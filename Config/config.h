#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int delay_min_ms;   // retardo mínimo por movimiento
    int delay_max_ms;   // retardo máximo por movimiento
    unsigned int seed;  // semilla para reproducibilidad
} Config;

// Rellena cfg con valores por defecto
void config_defaults(Config *cfg);

// Lee un archivo "clave=valor". Retorna 0 si ok, -1 si error
int config_load(Config *cfg, const char *path);

#endif