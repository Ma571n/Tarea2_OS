#ifndef TABLERO_H
#define TABLERO_H

#include <pthread.h>
#include <stdbool.h>

#define NUM_NODOS    28   
#define MAX_VECINOS   4
#define NUM_PERROS   12
#define CAPTURAS_PARA_GANAR 6

#define ABANDONA_NADIE  (-1)
#define ABANDONA_PUMA     0
#define ABANDONA_PERROS   1

#define SIN_GANADOR (-1)
#define GANA_PUMA     0
#define GANA_PERROS   1


#define NODO(f, c) (3 + 5 * (f) + (c))

typedef enum { VACIA = 0, PUMA = 1, PERRO = 2 } Casilla;

typedef struct {
    int hacia;
    int sobre;
} Movimiento;

typedef struct {
    Casilla casilla[NUM_NODOS];   
    int     id_perro[NUM_NODOS];  

    int  pos_puma;
    int  pos_perro[NUM_PERROS];   
    int  perros_vivos;
    int  capturas;

    bool terminado;
    int  ganador;
    int  abandono;                

    pthread_mutex_t mutex;       
} Tablero;

extern const int POS_INICIAL_PUMA;
extern const int POS_INICIAL_PERROS[NUM_PERROS];

void tablero_iniciar (Tablero *t, int pos_puma, const int pos_perros[NUM_PERROS]);
void tablero_destruir(Tablero *t);
void tablero_imprimir(const Tablero *t, int id_mesa);

void tablero_tomar (Tablero *t);
void tablero_soltar(Tablero *t);

bool son_vecinos(int a, int b);
int  destino_salto(int desde, int sobre);   

int  movimientos_puma (const Tablero *t, Movimiento salida[]);
int  movimientos_perro(const Tablero *t, int id, int salida[]);

void mover_puma   (Tablero *t, int hacia);
void capturar_perro(Tablero *t, int sobre, int hacia);
void mover_perro  (Tablero *t, int id, int hacia);

int  revisar_ganador(const Tablero *t);

#endif
