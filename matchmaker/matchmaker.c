#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "matchmaker.h"
#include "../partida/partida.h"

// Utilidades

static void dormir_ms(int ms) {
    struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

// Instante (reloj real) dentro de 'ms' milisegundos, para los timedwait 
static struct timespec en_ms(int ms) {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    t.tv_nsec += (long)ms * 1000000L;
    t.tv_sec += t.tv_nsec / 1000000000L;
    t.tv_nsec %= 1000000000L;
    return t;
}

static bool hay_apagado(const Matchmaker *mm) {
    return mm->apagado != NULL && *mm->apagado;
}

// Inicio y muerte

int mm_iniciar(Matchmaker *mm, const Config *cfg, int N, int M, volatile sig_atomic_t *apagado) {
    mm->cfg = cfg;
    mm->N = N;
    mm->M = M;
    mm->apagado = apagado;

    mm->capacidad_cola = N / 2 + 1;
    mm->cola = malloc((size_t)mm->capacidad_cola * sizeof(Pareja));
    mm->mesa_ocupada = calloc((size_t)M, sizeof(bool));
    if (mm->cola == NULL || mm->mesa_ocupada == NULL) {
        free(mm->cola);
        free(mm->mesa_ocupada);
        return -1;
    }

    mm->cola_ini = 0;
    mm->cola_n = 0;
    mm->esperando = -1;
    mm->llegados = 0;
    mm->activas = 0;
    mm->max_activas = 0;
    mm->partidas_jugadas = 0;
    mm->sin_pareja = 0;
    mm->abandonos = 0;

    sem_init(&mm->mesas_libres, 0, (unsigned int)M);
    pthread_mutex_init(&mm->mutex, NULL);
    pthread_cond_init(&mm->hay_pareja, NULL);
    return 0;
}

void mm_destruir(Matchmaker *mm) {
    sem_destroy(&mm->mesas_libres);
    pthread_cond_destroy(&mm->hay_pareja);
    pthread_mutex_destroy(&mm->mutex);
    free(mm->cola);
    free(mm->mesa_ocupada);
}

// Jugadores

typedef struct {
    Matchmaker *mm;
    int id;
    unsigned int semilla;
} Jugador;

// El jugador se anota. Si ya había uno esperando, quedan emparejados.
static void llegar(Matchmaker *mm, int id) {
    pthread_mutex_lock(&mm->mutex);
    mm->llegados++;
    printf("[Servidor] Llega el jugador %d\n", id);

    if (mm->esperando < 0) {
        mm->esperando = id;
    } else {
        int pos = (mm->cola_ini + mm->cola_n) % mm->capacidad_cola;
        mm->cola[pos] = (Pareja){ mm->esperando, id };
        mm->cola_n++;
        mm->esperando = -1;
    }
    pthread_cond_signal(&mm->hay_pareja);
    pthread_mutex_unlock(&mm->mutex);
}

static void *hilo_jugador(void *arg) {
    Jugador *j = arg;
    // cada jugador llega en un momento aleatorio
    dormir_ms((int)(rand_r(&j->semilla) % (unsigned int)(j->mm->cfg->delay_max_ms + 1)));
    if (!hay_apagado(j->mm))
        llegar(j->mm, j->id);
    return NULL;
}

// LOS JUECES

typedef struct {
    Matchmaker *mm;
    Partida partida;
    Config cfg;        
    int mesa;
} DatosMesa;

static void *hilo_juez(void *arg) {
    DatosMesa *d = arg;
    Matchmaker *mm = d->mm;

    int resultado = partida_jugar(&d->partida);

    pthread_mutex_lock(&mm->mutex);
    mm->mesa_ocupada[d->mesa - 1] = false;
    mm->activas--;
    if (resultado == GANA_PUMA || resultado == GANA_PERROS)
        mm->partidas_jugadas++;
    if (d->partida.por_abandono)
        mm->abandonos++;
    pthread_mutex_unlock(&mm->mutex);

    printf("[Servidor] Mesa %d liberada\n", d->mesa);
    sem_post(&mm->mesas_libres);
    return NULL;
}

// EL SERVER 

void mm_ejecutar(Matchmaker *mm) {
    int max_partidas = mm->N / 2;

    Jugador   *jugadores = calloc((size_t)mm->N, sizeof(Jugador));
    pthread_t *h_jug     = malloc((size_t)mm->N * sizeof(pthread_t));
    DatosMesa *mesas     = calloc((size_t)max_partidas + 1, sizeof(DatosMesa));
    pthread_t *h_juez    = malloc(((size_t)max_partidas + 1) * sizeof(pthread_t));
    if (!jugadores || !h_jug || !mesas || !h_juez) {
        fprintf(stderr, "Sin memoria\n");
        goto limpiar;
    }

    // llegan los N jugadores, cada uno en su propio hilo
    int jug_creados = 0;
    for (int i = 0; i < mm->N; i++) {
        jugadores[i] = (Jugador){ mm, i + 1, mm->cfg->seed + (unsigned int)i };
        if (pthread_create(&h_jug[i], NULL, hilo_jugador, &jugadores[i]) != 0) {
            fprintf(stderr, "No se pudo crear el jugador %d\n", i + 1);
            break;
        }
        jug_creados++;
    }

    int lanzadas = 0;
    for (;;) {
        // espera pasiva a que haya una pareja lista
        pthread_mutex_lock(&mm->mutex);
        while (mm->cola_n == 0 && mm->llegados < jug_creados && !hay_apagado(mm)) {
            struct timespec limite = en_ms(100);
            pthread_cond_timedwait(&mm->hay_pareja, &mm->mutex, &limite);
        }
        if (hay_apagado(mm) || mm->cola_n == 0) {
            pthread_mutex_unlock(&mm->mutex);
            break;
        }
        Pareja par = mm->cola[mm->cola_ini];
        mm->cola_ini = (mm->cola_ini + 1) % mm->capacidad_cola;
        mm->cola_n--;
        pthread_mutex_unlock(&mm->mutex);

        printf("[Servidor] Pareja lista: jugador %d (Puma) vs jugador %d (Perros)\n",
               par.jugador_puma, par.jugador_perros);

        // si ya hay M mesas ocupadas, espera aquí sin gastar CPU
        int obtuvo_mesa = 0;
        while (!obtuvo_mesa && !hay_apagado(mm)) {
            struct timespec limite = en_ms(100);
            if (sem_timedwait(&mm->mesas_libres, &limite) == 0)
                obtuvo_mesa = 1;
            else if (errno != ETIMEDOUT && errno != EINTR)
                break;
        }
        if (!obtuvo_mesa)
            break;

        // elige la primera mesa libre (el semáforo asegura que existe una) 
        pthread_mutex_lock(&mm->mutex);
        int mesa = 0;
        for (int k = 0; k < mm->M; k++)
            if (!mm->mesa_ocupada[k]) { mesa = k + 1; break; }
        mm->mesa_ocupada[mesa - 1] = true;
        mm->activas++;
        if (mm->activas > mm->max_activas)
            mm->max_activas = mm->activas;
        pthread_mutex_unlock(&mm->mutex);

        printf("[Servidor] Jugador %d y jugador %d pasan a la mesa %d\n",
               par.jugador_puma, par.jugador_perros, mesa);

        DatosMesa *d = &mesas[lanzadas];
        d->mm = mm;
        d->mesa = mesa;
        d->cfg = *mm->cfg;
        d->cfg.seed += (unsigned int)(lanzadas * 1000);
        d->partida = (Partida){ .id_mesa = mesa, .cfg = &d->cfg, .apagado = mm->apagado };

        // con probabilidad prob_abandono, un jugador falla en esta partida 
        unsigned int s = d->cfg.seed;
        if ((int)(rand_r(&s) % 100u) < mm->cfg->prob_abandono) {
            int tras = 1 + (int)(rand_r(&s) % 5u);
            if (rand_r(&s) % 2u == 0) d->partida.falla_puma_tras = tras;
            else                      d->partida.falla_perros_tras = tras;
        }

        if (pthread_create(&h_juez[lanzadas], NULL, hilo_juez, d) != 0) {
            fprintf(stderr, "No se pudo crear el juez de la mesa %d\n", mesa);
            pthread_mutex_lock(&mm->mutex);
            mm->mesa_ocupada[mesa - 1] = false;
            mm->activas--;
            pthread_mutex_unlock(&mm->mutex);
            sem_post(&mm->mesas_libres);
            break;
        }
        lanzadas++;
    }

    if (hay_apagado(mm))
        printf("[Servidor] Ctrl+C recibido: no se aceptan más jugadores y se cierran las mesas activas\n");

    for (int i = 0; i < lanzadas; i++)
        pthread_join(h_juez[i], NULL);
    for (int i = 0; i < jug_creados; i++)
        pthread_join(h_jug[i], NULL);

    if (!hay_apagado(mm) && mm->esperando >= 0) {
        mm->sin_pareja = 1;
        printf("[Servidor] El jugador %d se queda sin pareja\n", mm->esperando);
    }

limpiar:
    free(jugadores);
    free(h_jug);
    free(mesas);
    free(h_juez);
}
