#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include "../config/config.h"

/* Dos jugadores emparejados: el que llegó primero juega con el Puma. */
typedef struct {
    int jugador_puma;
    int jugador_perros;
} Pareja;

typedef struct {
    const Config *cfg;
    int N;                              /* jugadores totales          */
    int M;                              /* máximo de mesas simultáneas */
    volatile sig_atomic_t *apagado;     /* se pone en 1 con SIGINT     */

    sem_t mesas_libres;                 /* cuenta las mesas disponibles */

    pthread_mutex_t mutex;              /* protege todo lo de abajo    */
    pthread_cond_t  hay_pareja;         /* avisa al servidor           */

    Pareja *cola;                       /* parejas listas, en orden    */
    int capacidad_cola;
    int cola_ini;
    int cola_n;
    int esperando;                      /* jugador sin pareja, o -1    */
    int llegados;                       /* jugadores que ya llegaron   */
    bool *mesa_ocupada;                 /* una por mesa                */

    /* estadísticas */
    int activas;
    int max_activas;
    int partidas_jugadas;
    int sin_pareja;
    int abandonos;
} Matchmaker;

int  mm_iniciar (Matchmaker *mm, const Config *cfg, int N, int M, volatile sig_atomic_t *apagado);
void mm_ejecutar(Matchmaker *mm);   /* corre el torneo completo */
void mm_destruir(Matchmaker *mm);

#endif
