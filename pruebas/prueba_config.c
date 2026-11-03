/* Prueba del config: archivos válidos, inválidos y con finales de línea de Windows. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../config/config.h"

static int fallas = 0;
#define VERIFICAR(cond, msg) do { if (!(cond)) { printf("FALLA: %s\n", msg); fallas++; } } while (0)

/* Escribe 'contenido' en un archivo temporal y lo carga sobre unos valores por defecto. */
static int cargar_texto(Config *cfg, const char *contenido) {
    char ruta[] = "/tmp/config_prueba_XXXXXX";
    int fd = mkstemp(ruta);
    if (fd < 0) { perror("mkstemp"); exit(1); }
    FILE *f = fdopen(fd, "w");
    fputs(contenido, f);
    fclose(f);

    config_defaults(cfg);
    int r = config_load(cfg, ruta);
    unlink(ruta);
    return r;
}

int main(void) {
    Config c;

    /* los valores por defecto son válidos */
    config_defaults(&c);
    VERIFICAR(config_validar(&c) == 0, "los valores por defecto deben ser válidos");

    VERIFICAR(c.prob_abandono == 0, "por defecto nadie abandona");

    /* archivo completo */
    VERIFICAR(cargar_texto(&c,
        "# comentario\n"
        "delay_min=10\n"
        "delay_max=20\n"
        "seed=99\n"
        "puma=0\n"
        "perros=3,4,5,6,7,8,9,10,11,12,13,14\n") == 0, "archivo completo debería cargar");
    VERIFICAR(c.delay_min_ms == 10 && c.delay_max_ms == 20 && c.seed == 99, "delays o seed mal leídos");
    VERIFICAR(c.pos_perros[0] == 3 && c.pos_perros[11] == 14, "perros mal leídos");

    /* solo algunas claves: el resto queda con el valor por defecto */
    VERIFICAR(cargar_texto(&c, "delay_max=100\n") == 0, "archivo parcial debería cargar");
    VERIFICAR(c.delay_min_ms == 50 && c.delay_max_ms == 100, "clave faltante debe quedar por defecto");

    /* finales de línea de Windows */
    VERIFICAR(cargar_texto(&c, "delay_min=5\r\ndelay_max=8\r\n") == 0, "CRLF debería aceptarse");
    VERIFICAR(c.delay_min_ms == 5 && c.delay_max_ms == 8, "CRLF mal leído");

    VERIFICAR(cargar_texto(&c, "prob_abandono=30\n") == 0 && c.prob_abandono == 30, "prob_abandono mal leída");

    /* casos inválidos */
    VERIFICAR(cargar_texto(&c, "prob_abandono=101\n") == -1, "prob_abandono > 100 debe fallar");
    VERIFICAR(cargar_texto(&c, "prob_abandono=-5\n") == -1, "prob_abandono negativa debe fallar");
    VERIFICAR(cargar_texto(&c, "delay_min=500\ndelay_max=100\n") == -1, "min > max debe fallar");
    VERIFICAR(cargar_texto(&c, "delay_min=-1\n") == -1, "delay negativo debe fallar");
    VERIFICAR(cargar_texto(&c, "puma=99\n") == -1, "puma fuera del tablero debe fallar");
    VERIFICAR(cargar_texto(&c, "puma=16\n") == -1, "puma sobre un perro debe fallar");
    VERIFICAR(cargar_texto(&c, "perros=3,4,5\n") == -1, "pocos perros debe fallar");
    VERIFICAR(cargar_texto(&c, "perros=3,4,5,6,7,8,9,10,11,12,13,14,15\n") == -1, "demasiados perros debe fallar");
    VERIFICAR(cargar_texto(&c, "perros=3,3,5,6,7,8,9,10,11,12,13,14\n") == -1, "perros repetidos debe fallar");
    VERIFICAR(cargar_texto(&c, "perros=3,4,5,6,7,8,9,10,11,12,13,100\n") == -1, "perro fuera del tablero debe fallar");
    VERIFICAR(cargar_texto(&c, "delay_min=abc\n") == -1, "número inválido debe fallar");
    VERIFICAR(cargar_texto(&c, "colores=rojo\n") == -1, "clave desconocida debe fallar");
    VERIFICAR(cargar_texto(&c, "esto no tiene igual\n") == -1, "línea sin '=' debe fallar");
    VERIFICAR(config_load(&c, "/tmp/no_existe_este_archivo.txt") == -1, "archivo inexistente debe fallar");

    printf(fallas ? "\n%d prueba(s) fallaron\n" : "\nTodas las pruebas de config pasaron\n", fallas);
    return fallas != 0;
}
