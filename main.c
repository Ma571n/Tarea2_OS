#include <stdio.h>
#include <stdlib.h>
#include "config.h"

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

    printf("N=%d M=%d delay=[%d,%d] seed=%u\n",
           N, M, cfg.delay_min_ms, cfg.delay_max_ms, cfg.seed);
    return 0;
}