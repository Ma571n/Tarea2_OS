#ifndef PARTIDA_H
#define PARTIDA_H

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include "../config/config.h"
#include "../tablero/tablero.h"

typedef struct {
    int id_mesa;
    const Config *cfg;

    volatile sig_atomic_t *apagado;

    sem_t *mesas_libres;

    int falla_puma_tras;
    int falla_perros_tras;

    bool por_abandono;    
    int  mov_puma;        
    int  mov_perros;

    Tablero tablero;
    pthread_cond_t fin;   
} Partida;

int partida_jugar(Partida *p);

#endif
