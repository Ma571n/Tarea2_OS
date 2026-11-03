CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -pthread -g

FUENTES = main.c config/config.c tablero/tablero.c partida/partida.c

torneo: $(FUENTES)
	$(CC) $(CFLAGS) -o torneo $(FUENTES)

prueba_tablero: pruebas/prueba_tablero.c tablero/tablero.c
	$(CC) $(CFLAGS) -o prueba_tablero pruebas/prueba_tablero.c tablero/tablero.c
	./prueba_tablero

prueba_partida: pruebas/prueba_partida.c tablero/tablero.c partida/partida.c
	$(CC) $(CFLAGS) -o prueba_partida pruebas/prueba_partida.c tablero/tablero.c partida/partida.c
	./prueba_partida | tail -3

clean:
	rm -f torneo prueba_tablero prueba_partida
