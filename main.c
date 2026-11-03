#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "config/config.h"
#include "matchmaker/matchmaker.h"

static volatile sig_atomic_t apagado = 0;

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

    Matchmaker mm;
    if (mm_iniciar(&mm, &cfg, N, M, &apagado) != 0) {
        fprintf(stderr, "No se pudo iniciar el servidor\n");
        return 1;
    }

    mm_ejecutar(&mm);

    printf("\n[Servidor] Torneo terminado: %d partidas jugadas, máximo %d mesas a la vez\n",
           mm.partidas_jugadas, mm.max_activas);
    mm_destruir(&mm);
    return 0;
}
