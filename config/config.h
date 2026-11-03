#ifndef CONFIG_H
#define CONFIG_H

#include "../tablero/tablero.h"

typedef struct {
    int delay_min_ms;              // retardo mínimo por movimiento
    int delay_max_ms;              // retardo máximo por movimiento
    unsigned int seed;             // semilla para reproducibilidad
    int pos_puma;                  // nodo inicial del puma
    int pos_perros[NUM_PERROS];    // nodo inicial de cada perro
    int prob_abandono;             // % de partidas en que un jugador falla (0 a 100)
} Config;

// Rellena cfg con valores por defecto
void config_defaults(Config *cfg);

// Revisa que la configuración sea coherente. Retorna 0 si ok, -1 si no
int config_validar(const Config *cfg);

// Lee un archivo "clave=valor". Retorna 0 si ok, -1 si error.
// Claves: delay_min, delay_max, seed, puma, perros (12 nodos separados por coma),
// prob_abandono
int config_load(Config *cfg, const char *path);

#endif
