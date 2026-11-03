#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "config.h"

void config_defaults(Config *cfg) {
    cfg->delay_min_ms = 50;
    cfg->delay_max_ms = 300;
    cfg->seed = (unsigned int)time(NULL);
    cfg->prob_abandono = 0;
    cfg->pos_puma = POS_INICIAL_PUMA;
    for (int i = 0; i < NUM_PERROS; i++)
        cfg->pos_perros[i] = POS_INICIAL_PERROS[i];
}

int config_validar(const Config *cfg) {
    if (cfg->delay_min_ms < 0 || cfg->delay_min_ms > cfg->delay_max_ms) {
        fprintf(stderr, "config: delay_min (%d) debe estar entre 0 y delay_max (%d)\n",
                cfg->delay_min_ms, cfg->delay_max_ms);
        return -1;
    }

    if (cfg->prob_abandono < 0 || cfg->prob_abandono > 100) {
        fprintf(stderr, "config: prob_abandono debe estar entre 0 y 100\n");
        return -1;
    }

    // ocupado[n] cuenta cuántas fichas hay en el nodo n
    int ocupado[NUM_NODOS] = {0};

    if (cfg->pos_puma < 0 || cfg->pos_puma >= NUM_NODOS) {
        fprintf(stderr, "config: el puma está en un nodo inválido (%d)\n", cfg->pos_puma);
        return -1;
    }
    ocupado[cfg->pos_puma]++;

    for (int i = 0; i < NUM_PERROS; i++) {
        int n = cfg->pos_perros[i];
        if (n < 0 || n >= NUM_NODOS) {
            fprintf(stderr, "config: el perro %d está en un nodo inválido (%d)\n", i, n);
            return -1;
        }
        if (++ocupado[n] > 1) {
            fprintf(stderr, "config: hay dos fichas en el nodo %d\n", n);
            return -1;
        }
    }
    return 0;
}

// Convierte un texto a entero. Retorna 0 si era un número completo
static int a_entero(const char *s, long *valor) {
    char *fin;
    errno = 0;
    long v = strtol(s, &fin, 10);
    if (fin == s || errno != 0) return -1;
    while (isspace((unsigned char)*fin)) fin++;
    if (*fin != '\0') return -1;
    *valor = v;
    return 0;
}

// Lee "12,13,14,..." y lo guarda en cfg->pos_perros. Retorna 0 si hay exactamente 12
static int leer_perros(Config *cfg, char *texto) {
    int cantidad = 0;
    char *resto = NULL;
    for (char *tok = strtok_r(texto, ",", &resto); tok != NULL; tok = strtok_r(NULL, ",", &resto)) {
        long v;
        if (a_entero(tok, &v) != 0 || cantidad >= NUM_PERROS) return -1;
        cfg->pos_perros[cantidad++] = (int)v;
    }
    return cantidad == NUM_PERROS ? 0 : -1;
}

int config_load(Config *cfg, const char *path) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror("fopen config");
        return -1;
    }

    char linea[256];
    int num_linea = 0;
    while (fgets(linea, sizeof(linea), f) != NULL) {
        num_linea++;

        // quita espacios y saltos de línea (también \r de archivos de Windows)
        size_t largo = strlen(linea);
        while (largo > 0 && isspace((unsigned char)linea[largo - 1])) linea[--largo] = '\0';
        if (linea[0] == '\0' || linea[0] == '#') continue;

        char *igual = strchr(linea, '=');
        if (igual == NULL) {
            fprintf(stderr, "config:%d: se esperaba clave=valor\n", num_linea);
            fclose(f);
            return -1;
        }
        *igual = '\0';
        char *clave = linea;
        char *valor = igual + 1;

        long v;
        int mal = 0;
        if (strcmp(clave, "delay_min") == 0)      { mal = a_entero(valor, &v); cfg->delay_min_ms = (int)v; }
        else if (strcmp(clave, "delay_max") == 0) { mal = a_entero(valor, &v); cfg->delay_max_ms = (int)v; }
        else if (strcmp(clave, "seed") == 0)      { mal = a_entero(valor, &v); cfg->seed = (unsigned int)v; }
        else if (strcmp(clave, "prob_abandono") == 0) { mal = a_entero(valor, &v); cfg->prob_abandono = (int)v; }
        else if (strcmp(clave, "puma") == 0)      { mal = a_entero(valor, &v); cfg->pos_puma = (int)v; }
        else if (strcmp(clave, "perros") == 0)    { mal = leer_perros(cfg, valor); }
        else {
            fprintf(stderr, "config:%d: clave desconocida '%s'\n", num_linea, clave);
            fclose(f);
            return -1;
        }

        if (mal) {
            fprintf(stderr, "config:%d: valor inválido para '%s'\n", num_linea, clave);
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return config_validar(cfg);
}
