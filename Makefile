CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -pthread -g

FUENTES = main.c config/config.c tablero/tablero.c

torneo: $(FUENTES)
	$(CC) $(CFLAGS) -o torneo $(FUENTES)

prueba_tablero: pruebas/prueba_tablero.c tablero/tablero.c
	$(CC) $(CFLAGS) -o prueba_tablero pruebas/prueba_tablero.c tablero/tablero.c
	./prueba_tablero

clean:
	rm -f torneo prueba_tablero
