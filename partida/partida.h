#ifndef PARTIDA_H
#define PARTIDA_H

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "../config/config.h"
#include "../tablero/tablero.h"

/* QUE COMIENCE EL JUEGOOOOOOO */
typedef struct {
    int id_mesa;
    const Config *cfg;

    /* Se pone en 1 desde el manejador de SIGINT. Podria ser NULL. */
    volatile sig_atomic_t *apagado;

    /* Semáforo de mesas libres del servidor. Se hace sem_post al terminar. */
    sem_t *mesas_libres;

    Tablero tablero;
    pthread_cond_t fin;   /* avisa al juez que la partida terminó */
} Partida;

/* QUIEN HA GANADO????? :speaking_head: (llamen a dios) */
int partida_jugar(Partida *p);

#endif
