CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -pthread -g

torneo: main.c config/config.c
	$(CC) $(CFLAGS) -o torneo main.c config/config.c

clean:
	rm -f torneo