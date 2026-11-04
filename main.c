#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "config/config.h"
#include "matchmaker/matchmaker.h"

static volatile sig_atomic_t apagado = 0;

static void manejar_sigint(int sig) {
    (void)sig;
    apagado = 1;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Uso: %s N M [config.txt]\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    int M = atoi(argv[2]);
    if (N <= 0 || M <= 0) {
        fprintf(stderr, "N y M deben ser positivos\n");
        return 1;
    }

    Config cfg;
    config_defaults(&cfg);
    if (argc == 4 && config_load(&cfg, argv[3]) != 0) {
        return 1;
    }

    // con un segundo ctrl+c se cierra de inmediato
    struct sigaction sa = { 0 };
    sa.sa_handler = manejar_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &sa, NULL);

    Matchmaker mm;
    if (mm_iniciar(&mm, &cfg, N, M, &apagado) != 0) {
        fprintf(stderr, "No se pudo iniciar el servidor\n");
        return 1;
    }

    mm_ejecutar(&mm);

    if (apagado)
        printf("\n[Servidor] Interrumpido con Ctrl+C: mesas cerradas y recursos liberados\n");

    printf("\n[Servidor] Torneo terminado: %d partidas jugadas (%d por abandono), máximo %d mesas a la vez\n",
           mm.partidas_jugadas, mm.abandonos, mm.max_activas);
    mm_destruir(&mm);
    return 0;
}
