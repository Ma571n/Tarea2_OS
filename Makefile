CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -pthread -g

FUENTES = main.c config/config.c tablero/tablero.c partida/partida.c matchmaker/matchmaker.c

torneo: $(FUENTES)
	$(CC) $(CFLAGS) -o torneo $(FUENTES)

prueba_tablero: pruebas/prueba_tablero.c tablero/tablero.c
	$(CC) $(CFLAGS) -o prueba_tablero pruebas/prueba_tablero.c tablero/tablero.c
	./prueba_tablero | tail -2

prueba_config: pruebas/prueba_config.c config/config.c tablero/tablero.c
	$(CC) $(CFLAGS) -o prueba_config pruebas/prueba_config.c config/config.c tablero/tablero.c
	./prueba_config 2>/dev/null | tail -2

prueba_partida: pruebas/prueba_partida.c config/config.c tablero/tablero.c partida/partida.c
	$(CC) $(CFLAGS) -o prueba_partida pruebas/prueba_partida.c config/config.c tablero/tablero.c partida/partida.c
	./prueba_partida | tail -2

prueba_matchmaker: pruebas/prueba_matchmaker.c config/config.c tablero/tablero.c partida/partida.c matchmaker/matchmaker.c
	$(CC) $(CFLAGS) -o prueba_matchmaker pruebas/prueba_matchmaker.c config/config.c tablero/tablero.c partida/partida.c matchmaker/matchmaker.c
	./prueba_matchmaker | tail -2

.PHONY: pruebas prueba_sigint clean
prueba_sigint:
	bash pruebas/prueba_sigint.sh

pruebas: prueba_tablero prueba_config prueba_partida prueba_matchmaker prueba_sigint

clean:
	rm -f torneo prueba_tablero prueba_config prueba_partida prueba_matchmaker
