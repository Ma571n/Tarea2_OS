/* Prueba del tablero: simetría de vecinos, saltos válidos y un caso de captura. */
#include <stdio.h>
#include "../tablero/tablero.h"

static int fallas = 0;
#define VERIFICAR(cond, msg) do { if (!(cond)) { printf("FALLA: %s\n", msg); fallas++; } } while (0)

int main(void) {
    /* 1. la relación de vecindad es simétrica y tiene las aristas del diagrama */
    int aristas = 0;
    for (int a = 0; a < NUM_NODOS; a++)
        for (int b = 0; b < NUM_NODOS; b++)
            if (son_vecinos(a, b)) {
                VERIFICAR(son_vecinos(b, a), "vecindad no simétrica");
                aristas++;
            }
    VERIFICAR(aristas / 2 == 40 + 8, "cantidad de aristas incorrecta (40 de la grilla + 8 del triángulo)");
    VERIFICAR(son_vecinos(0, 5) && son_vecinos(1, 3) && son_vecinos(2, 7), "faltan aristas del trono");
    VERIFICAR(!son_vecinos(3, 9), "no hay diagonales en la grilla");

    /* 2. todo salto pasa por un vecino y aterriza en un vecino de ese nodo */
    for (int a = 0; a < NUM_NODOS; a++)
        for (int b = 0; b < NUM_NODOS; b++) {
            int c = destino_salto(a, b);
            if (c >= 0) {
                VERIFICAR(son_vecinos(a, b) && son_vecinos(b, c), "salto inconsistente");
                VERIFICAR(destino_salto(c, b) == a, "el salto inverso no existe");
            }
        }
    VERIFICAR(destino_salto(0, 5) == 10 && destino_salto(1, 4) == 9, "saltos desde el triángulo");
    VERIFICAR(destino_salto(1, 2) == -1, "no se salta por el borde del triángulo");

    /* 3. partida inicial: el puma en el trono no tiene capturas */
    Tablero t;
    tablero_iniciar(&t, POS_INICIAL_PUMA, POS_INICIAL_PERROS);
    Movimiento m[MAX_VECINOS];
    int n = movimientos_puma(&t, m);
    VERIFICAR(n == 3, "el puma en el trono debería tener 3 pasos");
    VERIFICAR(t.casilla[POS_INICIAL_PERROS[0]] == PERRO, "perro mal ubicado");

    /* 4. captura: puma en 10, perro en 15, 20 vacío */
    tablero_destruir(&t);
    int perros[NUM_PERROS] = { 15, 3, 7, 8, 12, 13, 17, 18, 22, 23, 27, 26 };
    tablero_iniciar(&t, 10, perros);
    n = movimientos_puma(&t, m);
    int hay_captura = 0;
    for (int i = 0; i < n; i++)
        if (m[i].sobre == 15 && m[i].hacia == 20) hay_captura = 1;
    VERIFICAR(hay_captura, "debería poder capturar al perro en 15");
    capturar_perro(&t, 15, 20);
    VERIFICAR(t.capturas == 1 && t.perros_vivos == 11 && t.pos_puma == 20 && t.pos_perro[0] == -1,
              "estado después de capturar");
    VERIFICAR(revisar_ganador(&t) == SIN_GANADOR, "no debería haber ganador todavía");

    /* 5. puma acorralado en la esquina 27 */
    tablero_destruir(&t);
    int cerco[NUM_PERROS] = { 22, 26, 17, 25, 3, 4, 5, 6, 7, 8, 9, 10 };
    tablero_iniciar(&t, 27, cerco);
    VERIFICAR(revisar_ganador(&t) == GANA_PERROS, "el puma debería estar acorralado");
    tablero_imprimir(&t, 1);
    tablero_destruir(&t);

    printf(fallas ? "\n%d prueba(s) fallaron\n" : "\nTodas las pruebas pasaron\n", fallas);
    return fallas != 0;
}
