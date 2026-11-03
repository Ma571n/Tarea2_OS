#ifndef PARTIDA_H
#define PARTIDA_H

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include "../config/config.h"
#include "../tablero/tablero.h"

/*
 * Una partida = una mesa. El hilo que llama a partida_jugar() hace de Juez:
 * crea el tablero, lanza 13 hilos (1 puma + 12 perros), espera a que alguien
 * gane (o a que el servidor pida apagar) y libera todo al terminar.
 */
typedef struct {
    int id_mesa;
    const Config *cfg;

    /* Se pone en 1 desde el manejador de SIGINT. Puede ser NULL. */
    volatile sig_atomic_t *apagado;

    /* Semáforo de mesas libres del servidor. Se hace sem_post al terminar.
     * Puede ser NULL (por ejemplo en las pruebas). */
    sem_t *mesas_libres;

    /* Fallas simuladas: el jugador falla justo después de su movimiento
     * número N (0 = nunca falla). Sirven para probar el abandono. */
    int falla_puma_tras;
    int falla_perros_tras;

    bool por_abandono;    /* resultado: la partida terminó por abandono */
    int  mov_puma;        /* movimientos hechos (protegidos por el mutex) */
    int  mov_perros;

    Tablero tablero;
    pthread_cond_t fin;   /* avisa al juez que la partida terminó */
} Partida;

/* Juega la partida completa. Devuelve GANA_PUMA, GANA_PERROS (también cuando
 * el oponente abandona; ver por_abandono),
 * SIN_GANADOR (si se interrumpió) o -1 si falló la creación de hilos. */
int partida_jugar(Partida *p);

#endif
