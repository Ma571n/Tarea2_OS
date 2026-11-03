#include <stdio.h>
#include "tablero.h"

const int POS_INICIAL_PUMA = 0;
const int POS_INICIAL_PERROS[NUM_PERROS] = {
    NODO(2,3), NODO(2,4),
    NODO(3,0), NODO(3,1), NODO(3,2), NODO(3,3), NODO(3,4),
    NODO(4,0), NODO(4,1), NODO(4,2), NODO(4,3), NODO(4,4),
};

/* Los vecinos y saltos */

static int  vecinos[NUM_NODOS][MAX_VECINOS];
static int  num_vecinos[NUM_NODOS];
static int  salto[NUM_NODOS][NUM_NODOS];   /* salto[a][b] = c, o -1 */
static pthread_once_t una_vez = PTHREAD_ONCE_INIT;

static void unir(int a, int b) {
    vecinos[a][num_vecinos[a]++] = b;
    vecinos[b][num_vecinos[b]++] = a;
}

/* Saltos en línea recta que salen o pasan por el triángulo.
 * Los de la grilla se calculan aparte. */
static const int saltos_especiales[][3] = {
    /* desde, sobre, hacia */
    {0, 1, 3},  {3, 1, 0},     /* diagonal de la izquierda */
    {0, 2, 7},  {7, 2, 0},     /* diagonal de la derecha   */
    {0, 5, 10}, {10, 5, 0},    /* vertical central   */
    {1, 4, 9},  {9, 4, 1},     /* vertical de la izquierda */
    {2, 6, 11}, {11, 6, 2},    /* vertical de la derecha   */
};

static void construir_tablas(void) {
    /* Vecinos horizontales y verticales */
    for (int f = 0; f < 5; f++)
        for (int c = 0; c < 5; c++) {
            if (c < 4) unir(NODO(f, c), NODO(f, c + 1));
            if (f < 4) unir(NODO(f, c), NODO(f + 1, c));
        }

    /* Cabeza y triángulo */
    unir(0, 1);  unir(0, 2);  unir(1, 2);
    unir(0, 5);                    /* Cabeza con el centro de la fila 0 */
    unir(1, 3);  unir(1, 4);       /* nodo izquierdo */
    unir(2, 6);  unir(2, 7);       /* nodo derecho */

    for (int a = 0; a < NUM_NODOS; a++)
        for (int b = 0; b < NUM_NODOS; b++)
            salto[a][b] = -1;

    /* se sigue hacia donde mismo */
    for (int a = 3; a < NUM_NODOS; a++)
        for (int i = 0; i < num_vecinos[a]; i++) {
            int b = vecinos[a][i];
            if (b < 3) continue;
            int fa = (a - 3) / 5, ca = (a - 3) % 5;
            int fb = (b - 3) / 5, cb = (b - 3) % 5;
            int fc = 2 * fb - fa, cc = 2 * cb - ca;
            if (fc >= 0 && fc < 5 && cc >= 0 && cc < 5)
                salto[a][b] = NODO(fc, cc);
        }

    int n = (int)(sizeof saltos_especiales / sizeof saltos_especiales[0]);
    for (int i = 0; i < n; i++)
        salto[saltos_especiales[i][0]][saltos_especiales[i][1]] = saltos_especiales[i][2];

}

static void preparar_tablas(void) {
    pthread_once(&una_vez, construir_tablas);
}

bool son_vecinos(int a, int b) {
    preparar_tablas();
    if (a < 0 || a >= NUM_NODOS) return false;
    for (int i = 0; i < num_vecinos[a]; i++)
        if (vecinos[a][i] == b) return true;
    return false;
}

int destino_salto(int desde, int sobre) {
    preparar_tablas();
    if (desde < 0 || desde >= NUM_NODOS || sobre < 0 || sobre >= NUM_NODOS)
        return -1;
    return salto[desde][sobre];
}

/* Inicio y muerte....*/

void tablero_iniciar(Tablero *t, int pos_puma, const int pos_perros[NUM_PERROS]) {
    preparar_tablas();

    for (int i = 0; i < NUM_NODOS; i++) {
        t->casilla[i]  = VACIA;
        t->id_perro[i] = -1;
    }
    t->pos_puma = pos_puma;
    t->casilla[pos_puma] = PUMA;

    for (int d = 0; d < NUM_PERROS; d++) {
        t->pos_perro[d] = pos_perros[d];
        t->casilla[pos_perros[d]]  = PERRO;
        t->id_perro[pos_perros[d]] = d;
    }

    t->perros_vivos = NUM_PERROS;
    t->capturas     = 0;
    t->terminado    = false;
    t->ganador      = SIN_GANADOR;

    pthread_mutex_init(&t->mutex, NULL);
}

void tablero_destruir(Tablero *t) {
    pthread_mutex_destroy(&t->mutex);
}

/* Impresión del tablero a pantallita */

static pthread_mutex_t mutex_impresion = PTHREAD_MUTEX_INITIALIZER;

static void etiqueta(const Tablero *t, int nodo, char s[4]) {
    if (t->casilla[nodo] == VACIA)      snprintf(s, 4, " . ");
    else if (t->casilla[nodo] == PUMA)  snprintf(s, 4, " P ");
    else                                snprintf(s, 4, "D%-2d", t->id_perro[nodo]);
}

void tablero_imprimir(const Tablero *t, int id_mesa) {
    char e[NUM_NODOS][4];
    for (int i = 0; i < NUM_NODOS; i++) etiqueta(t, i, e[i]);

    pthread_mutex_lock(&mutex_impresion);
    printf("\n--- Mesa %d ---\n", id_mesa);
    printf("            [%s]\n", e[0]);
    printf("         [%s]-[%s]\n", e[1], e[2]);
    for (int f = 0; f < 5; f++) {
        printf(" ");
        for (int c = 0; c < 5; c++)
            printf("[%s]%s", e[NODO(f, c)], c < 4 ? "-" : "\n");
    }
    printf(" Capturas: %d | Perros vivos: %d\n", t->capturas, t->perros_vivos);
    fflush(stdout);
    pthread_mutex_unlock(&mutex_impresion);
}

/* Movimientos posibles */

int movimientos_puma(const Tablero *t, Movimiento salida[]) {
    int n = 0;
    int desde = t->pos_puma;

    for (int i = 0; i < num_vecinos[desde]; i++) {
        int v = vecinos[desde][i];
        if (t->casilla[v] == VACIA) {
            salida[n++] = (Movimiento){ v, -1 };
        } else if (t->casilla[v] == PERRO) {
            int c = salto[desde][v];
            if (c >= 0 && t->casilla[c] == VACIA)
                salida[n++] = (Movimiento){ c, v };
        }
    }
    return n;
}

int movimientos_perro(const Tablero *t, int id, int salida[]) {
    int n = 0;
    int desde = t->pos_perro[id];
    if (desde < 0) return 0;

    for (int i = 0; i < num_vecinos[desde]; i++)
        if (t->casilla[vecinos[desde][i]] == VACIA)
            salida[n++] = vecinos[desde][i];
    return n;
}

/* Cambios en el tablero */

void mover_puma(Tablero *t, int hacia) {
    t->casilla[t->pos_puma] = VACIA;
    t->pos_puma = hacia;
    t->casilla[hacia] = PUMA;
}

void capturar_perro(Tablero *t, int sobre, int hacia) {
    int id = t->id_perro[sobre];
    t->casilla[sobre]  = VACIA;
    t->id_perro[sobre] = -1;
    t->pos_perro[id]   = -1;
    t->perros_vivos--;
    t->capturas++;
    mover_puma(t, hacia);
}

void mover_perro(Tablero *t, int id, int hacia) {
    int desde = t->pos_perro[id];
    t->casilla[desde]  = VACIA;
    t->id_perro[desde] = -1;
    t->pos_perro[id]   = hacia;
    t->casilla[hacia]  = PERRO;
    t->id_perro[hacia] = id;
}

int revisar_ganador(const Tablero *t) {
    if (t->capturas >= CAPTURAS_PARA_GANAR)
        return GANA_PUMA;

    Movimiento m[MAX_VECINOS];
    if (movimientos_puma(t, m) == 0)
        return GANA_PERROS;

    return SIN_GANADOR;
}
