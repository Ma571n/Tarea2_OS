/* Prueba del matchmaker: parejas, límite de mesas y apagado. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>
#include "../matchmaker/matchmaker.h"

static int fallas = 0;
#define VERIFICAR(cond, msg) do { if (!(cond)) { printf("FALLA: %s\n", msg); fallas++; } } while (0)

static volatile sig_atomic_t apagar = 0;

static void *pedir_apagado(void *arg) {
    (void)arg;
    struct timespec ts = { 0, 400 * 1000000L };
    nanosleep(&ts, NULL);
    apagar = 1;
    return NULL;
}

static void torneo(Matchmaker *mm, const Config *cfg, int N, int M, volatile sig_atomic_t *ap) {
    mm_iniciar(mm, cfg, N, M, ap);
    mm_ejecutar(mm);
}

int main(void) {
    Config rapida;
    config_defaults(&rapida);
    rapida.delay_min_ms = 1;
    rapida.delay_max_ms = 5;
    rapida.seed = 3;

    Matchmaker mm;

    /* 1. N par: todos se emparejan y nunca hay más de M mesas a la vez */
    torneo(&mm, &rapida, 10, 2, NULL);
    VERIFICAR(mm.partidas_jugadas == 5, "con N=10 debían jugarse 5 partidas");
    VERIFICAR(mm.max_activas >= 1 && mm.max_activas <= 2, "nunca más de M=2 mesas activas");
    VERIFICAR(mm.sin_pareja == 0, "con N par nadie queda sin pareja");
    VERIFICAR(mm.activas == 0, "al final no deben quedar mesas activas");
    mm_destruir(&mm);

    /* 2. N impar: uno queda sin pareja */
    torneo(&mm, &rapida, 7, 3, NULL);
    VERIFICAR(mm.partidas_jugadas == 3, "con N=7 debían jugarse 3 partidas");
    VERIFICAR(mm.sin_pareja == 1, "con N impar uno queda sin pareja");
    VERIFICAR(mm.max_activas <= 3, "nunca más de M=3 mesas activas");
    mm_destruir(&mm);

    /* 3. una sola mesa: las partidas se hacen una a la vez */
    torneo(&mm, &rapida, 6, 1, NULL);
    VERIFICAR(mm.partidas_jugadas == 3, "con N=6 debían jugarse 3 partidas");
    VERIFICAR(mm.max_activas == 1, "con M=1 solo una mesa a la vez");
    mm_destruir(&mm);

    /* 4. caso mínimo */
    torneo(&mm, &rapida, 2, 5, NULL);
    VERIFICAR(mm.partidas_jugadas == 1, "con N=2 se juega 1 partida");
    mm_destruir(&mm);

    /* 5. apagado en medio del torneo: termina rápido y sin mesas colgadas */
    Config lenta = rapida;
    lenta.delay_min_ms = 200;
    lenta.delay_max_ms = 300;
    pthread_t h;
    pthread_create(&h, NULL, pedir_apagado, NULL);
    time_t inicio = time(NULL);
    torneo(&mm, &lenta, 6, 3, &apagar);
    pthread_join(h, NULL);
    VERIFICAR(time(NULL) - inicio <= 3, "el apagado tardó demasiado");
    VERIFICAR(mm.partidas_jugadas == 0, "partidas interrumpidas no cuentan como jugadas");
    VERIFICAR(mm.activas == 0, "tras el apagado no deben quedar mesas activas");
    mm_destruir(&mm);

    printf(fallas ? "\n%d prueba(s) fallaron\n" : "\nTodas las pruebas de matchmaker pasaron\n", fallas);
    return fallas != 0;
}
