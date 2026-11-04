#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include "../config/config.h"

// Dos jugadores emparejados: el que llegó primero juega con el Puma 
typedef struct {
    int jugador_puma;
    int jugador_perros;
} Pareja;

typedef struct {
    const Config *cfg;
    int N;                              
    int M;                              
    volatile sig_atomic_t *apagado;     

    sem_t mesas_libres;                 

    pthread_mutex_t mutex;              
    pthread_cond_t  hay_pareja;         

    Pareja *cola;                       
    int capacidad_cola;
    int cola_ini;
    int cola_n;
    int esperando;                      
    int llegados;                       
    bool *mesa_ocupada;                 

    // Estadísticas
    int activas;
    int max_activas;
    int partidas_jugadas;
    int sin_pareja;
    int abandonos;
} Matchmaker;

int  mm_iniciar (Matchmaker *mm, const Config *cfg, int N, int M, volatile sig_atomic_t *apagado);
void mm_ejecutar(Matchmaker *mm);   
void mm_destruir(Matchmaker *mm);

#endif
