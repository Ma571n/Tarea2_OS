#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "partida.h"

/* Datos que recibe cada hilo ficha. id = -1 es el puma, 0..11 son los perros. */
typedef struct {
    Partida *p;
    int id;
    unsigned int semilla;
} Ficha;

static void dormir_ms(int ms) {
    struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

static void dormir_aleatorio(const Config *cfg, unsigned int *semilla) {
    int rango = cfg->delay_max_ms - cfg->delay_min_ms + 1;
    dormir_ms(cfg->delay_min_ms + (int)(rand_r(semilla) % (unsigned int)rango));
}

/* Se llama con el mutex del tablero tomado. */
static void revisar_fin(Partida *p) {
    Tablero *t = &p->tablero;
    int g = revisar_ganador(t);
    if (g != SIN_GANADOR) {
        t->terminado = true;
        t->ganador = g;
        pthread_cond_signal(&p->fin);
    }
}

/* Simula que el jugador se cae: anota quién falló, avisa al juez y muere
 * SIN soltar el mutex del tablero (el peor caso posible).
 * Se llama con el mutex tomado y no vuelve. */
static void caerse(Partida *p, int rol) {
    p->tablero.abandono = rol;
    printf("[Mesa %d] El jugador %s falló y dejó el mutex tomado\n",
           p->id_mesa, rol == ABANDONA_PUMA ? "del Puma" : "de los Perros");
    pthread_cond_signal(&p->fin);
    pthread_exit(NULL);
}

/* Threads de las fichas */

static void *hilo_puma(void *arg) {
    Ficha *f = arg;
    Partida *p = f->p;
    Tablero *t = &p->tablero;

    for (;;) {
        dormir_aleatorio(p->cfg, &f->semilla);

        tablero_tomar(t);
        if (t->terminado || t->abandono != ABANDONA_NADIE) {
            tablero_soltar(t);
            break;
        }

        Movimiento opciones[MAX_VECINOS];
        int n = movimientos_puma(t, opciones);
        if (n > 0) {
            /* si puede capturar, lo prefiere; si no, elige al azar */
            int elegido = (int)(rand_r(&f->semilla) % (unsigned int)n);
            for (int i = 0; i < n; i++)
                if (opciones[i].sobre >= 0) { elegido = i; break; }

            Movimiento m = opciones[elegido];
            int desde = t->pos_puma;
            if (m.sobre >= 0) {
                int id_perro = t->id_perro[m.sobre];
                capturar_perro(t, m.sobre, m.hacia);
                printf("[Mesa %d] Puma %d -> %d captura a D%d (capturas: %d)\n",
                       p->id_mesa, desde, m.hacia, id_perro, t->capturas);
                tablero_imprimir(t, p->id_mesa);
            } else {
                mover_puma(t, m.hacia);
                printf("[Mesa %d] Puma %d -> %d\n", p->id_mesa, desde, m.hacia);
            }
            revisar_fin(p);

            p->mov_puma++;
            if (!t->terminado && p->falla_puma_tras > 0 && p->mov_puma >= p->falla_puma_tras)
                caerse(p, ABANDONA_PUMA);
        }
        tablero_soltar(t);
    }
    return NULL;
}

static void *hilo_perro(void *arg) {
    Ficha *f = arg;
    Partida *p = f->p;
    Tablero *t = &p->tablero;

    for (;;) {
        dormir_aleatorio(p->cfg, &f->semilla);

        tablero_tomar(t);
        if (t->terminado || t->abandono != ABANDONA_NADIE || t->pos_perro[f->id] < 0) {
            tablero_soltar(t);   /* fin, abandono o perro capturado */
            break;
        }

        int destinos[MAX_VECINOS];
        int n = movimientos_perro(t, f->id, destinos);
        if (n > 0) {
            int desde = t->pos_perro[f->id];
            int hacia = destinos[rand_r(&f->semilla) % (unsigned int)n];
            mover_perro(t, f->id, hacia);
            printf("[Mesa %d] D%d %d -> %d\n", p->id_mesa, f->id, desde, hacia);
            revisar_fin(p);

            p->mov_perros++;
            if (!t->terminado && p->falla_perros_tras > 0 && p->mov_perros >= p->falla_perros_tras)
                caerse(p, ABANDONA_PERROS);
        }
        tablero_soltar(t);
    }
    return NULL;
}

/* JUEZ YO MANDO */

int partida_jugar(Partida *p) {
    Tablero *t = &p->tablero;
    tablero_iniciar(t, p->cfg->pos_puma, p->cfg->pos_perros);
    pthread_cond_init(&p->fin, NULL);
    p->por_abandono = false;
    p->mov_puma = 0;
    p->mov_perros = 0;

    printf("[Mesa %d] Partida iniciada\n", p->id_mesa);
    tablero_imprimir(t, p->id_mesa);

    pthread_t hilos[NUM_PERROS + 1];
    Ficha fichas[NUM_PERROS + 1];
    int creados = 0;
    int error = 0;

    for (int i = 0; i <= NUM_PERROS; i++) {
        fichas[i].p = p;
        fichas[i].id = i - 1;    /* la posición 0 es el puma */
        fichas[i].semilla = p->cfg->seed + (unsigned int)(p->id_mesa * 100 + i);
        void *(*rutina)(void *) = (i == 0) ? hilo_puma : hilo_perro;
        if (pthread_create(&hilos[i], NULL, rutina, &fichas[i]) != 0) {
            error = 1;
            break;
        }
        creados++;
    }

    /* El juez espera el fin de la partida, un abandono o un apagado, despierta cada 100 ms por si acaso se perdió algún aviso. */
    tablero_tomar(t);
    if (error)
        t->terminado = true;
    while (!t->terminado) {
        if (t->abandono != ABANDONA_NADIE) {
            /* un jugador falló: gana el oponente por abandono */
            t->terminado = true;
            t->ganador = (t->abandono == ABANDONA_PUMA) ? GANA_PERROS : GANA_PUMA;
            p->por_abandono = true;
            break;
        }
        if (p->apagado != NULL && *p->apagado) {
            t->terminado = true;
            break;
        }
        struct timespec limite;
        clock_gettime(CLOCK_REALTIME, &limite);
        limite.tv_nsec += 100 * 1000000L;
        if (limite.tv_nsec >= 1000000000L) {
            limite.tv_sec++;
            limite.tv_nsec -= 1000000000L;
        }
        /* si el que murió tenía el mutex, aquí volvemos con EOWNERDEAD */
        if (pthread_cond_timedwait(&p->fin, &t->mutex, &limite) == EOWNERDEAD)
            pthread_mutex_consistent(&t->mutex);
    }
    tablero_soltar(t);

    for (int i = 0; i < creados; i++)
        pthread_join(hilos[i], NULL);

    int resultado = error ? -1 : t->ganador;
    if (p->por_abandono)
        printf("[Mesa %d] Gana %s por abandono del oponente\n", p->id_mesa,
               resultado == GANA_PUMA ? "el Puma" : "los Perros");
    else if (resultado == GANA_PUMA)
        printf("[Mesa %d] Gana el Puma con %d capturas\n", p->id_mesa, t->capturas);
    else if (resultado == GANA_PERROS)
        printf("[Mesa %d] Ganan los Perros: el Puma quedó acorralado\n", p->id_mesa);
    else
        printf("[Mesa %d] Partida interrumpida\n", p->id_mesa);
    tablero_imprimir(t, p->id_mesa);

    pthread_cond_destroy(&p->fin);
    tablero_destruir(t);
    if (p->mesas_libres != NULL)
        sem_post(p->mesas_libres);
    return resultado;
}
