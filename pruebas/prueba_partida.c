/* Prueba de partidas completas, con retardos cortos para que sea rápido. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../partida/partida.h"

static int fallas = 0;
#define VERIFICAR(cond, msg) do { if (!(cond)) { printf("FALLA: %s\n", msg); fallas++; } } while (0)

static volatile sig_atomic_t apagar = 0;

static void *pedir_apagado(void *arg) {
    (void)arg;
    struct timespec ts = { 0, 300 * 1000000L };
    nanosleep(&ts, NULL);
    apagar = 1;
    return NULL;
}

int main(void) {
    Config cfg;
    config_defaults(&cfg);
    cfg.delay_min_ms = 1;
    cfg.delay_max_ms = 5;
    cfg.seed = 42;

    /* 1. varias partidas seguidas: siempre termina con un ganador coherente */
    for (int mesa = 1; mesa <= 5; mesa++) {
        Partida p = { .id_mesa = mesa, .cfg = &cfg };
        int g = partida_jugar(&p);
        VERIFICAR(g == GANA_PUMA || g == GANA_PERROS, "la partida debe tener ganador");
        VERIFICAR(p.tablero.perros_vivos == NUM_PERROS - p.tablero.capturas,
                  "perros vivos + capturas debe sumar 12");
        if (g == GANA_PUMA)
            VERIFICAR(p.tablero.capturas >= CAPTURAS_PARA_GANAR, "puma ganó sin 6 capturas");
    }

    /* 2. el semáforo de mesas se libera al terminar */
    sem_t mesas;
    sem_init(&mesas, 0, 0);
    Partida p2 = { .id_mesa = 6, .cfg = &cfg, .mesas_libres = &mesas };
    partida_jugar(&p2);
    int valor;
    sem_getvalue(&mesas, &valor);
    VERIFICAR(valor == 1, "la mesa debería liberarse con sem_post");
    sem_destroy(&mesas);

    /* 3. apagado: con retardos largos, la partida se corta rápido */
    Config lenta = cfg;
    lenta.delay_min_ms = 200;
    lenta.delay_max_ms = 300;
    Partida p3 = { .id_mesa = 7, .cfg = &lenta, .apagado = &apagar };
    pthread_t h;
    pthread_create(&h, NULL, pedir_apagado, NULL);
    time_t inicio = time(NULL);
    int g3 = partida_jugar(&p3);
    pthread_join(h, NULL);
    VERIFICAR(g3 == SIN_GANADOR, "una partida interrumpida no tiene ganador");
    VERIFICAR(time(NULL) - inicio <= 2, "el apagado tardó demasiado");

    /* 4. abandono: el jugador se cae con el mutex tomado y el oponente gana,
     *    sin que nadie quede bloqueado (se repite para probar distintas carreras) */
    for (int i = 0; i < 20; i++) {
        Partida pa = { .id_mesa = 10, .cfg = &cfg, .falla_puma_tras = 3 };
        time_t t0 = time(NULL);
        int ga = partida_jugar(&pa);
        VERIFICAR(ga == GANA_PERROS && pa.por_abandono, "si el Puma abandona, ganan los Perros");
        VERIFICAR(time(NULL) - t0 <= 2, "el abandono del Puma dejó bloqueada la mesa");

        Partida pb = { .id_mesa = 11, .cfg = &cfg, .falla_perros_tras = 4 };
        t0 = time(NULL);
        int gb = partida_jugar(&pb);
        VERIFICAR(gb == GANA_PUMA && pb.por_abandono, "si los Perros abandonan, gana el Puma");
        VERIFICAR(time(NULL) - t0 <= 2, "el abandono de los Perros dejó bloqueada la mesa");
    }

    /* 5. sin fallas no hay abandono */
    Partida pc = { .id_mesa = 12, .cfg = &cfg };
    partida_jugar(&pc);
    VERIFICAR(!pc.por_abandono, "sin fallas no debe haber abandono");

    printf(fallas ? "\n%d prueba(s) fallaron\n" : "\nTodas las pruebas de partida pasaron\n", fallas);
    return fallas != 0;
}
