#ifndef CONFIG_H
#define CONFIG_H

#include "../tablero/tablero.h"

typedef struct {
    int delay_min_ms;              
    int delay_max_ms;              
    unsigned int seed;             
    int pos_puma;                  
    int pos_perros[NUM_PERROS];    
    int prob_abandono;             
} Config;

void config_defaults(Config *cfg);

int config_validar(const Config *cfg);

int config_load(Config *cfg, const char *path);

#endif
