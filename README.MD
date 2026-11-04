Tarea 2 "Torneo de Komikan Concurrente"

Integrantes: -Matías Ignacio Neira Guzmán -Martín Alejandro Mondaca Labarca

Descripcion: Programa en C que simula un torneo de Komikan: un Puma contra 12 Perros en un tablero de 5x5 con trono. Cada ficha es un hilo (pthread) que se mueve con retardos aleatorios. Un servidor recibe N jugadores, los empareja de a dos (Puma y Perros) y les asigna una mesa, donde un juez valida los movimientos y detecta al ganador. Nunca hay más de M mesas jugando a la vez y el programa reacciona a Ctrl+C.

Estructura: -main.c: valida los argumentos, instala el manejador de Ctrl+C y llama al resto.

-config/: archivos config.c y config.h (leen y validan el archivo de configuración), más basico.txt y rapido.txt de ejemplo.

-tablero/: archivos tablero.c y tablero.h (nodos, vecinos, saltos, movimientos y capturas).

-partida/: archivos partida.c y partida.h (una mesa: hilos de las fichas y juez).

-matchmaker/: archivos matchmaker.c y matchmaker.h (servidor: jugadores, parejas y límite de mesas).

-pruebas/: pruebas de cada módulo y del Ctrl+C.

Compilar: Desde la carpeta del proyecto: "make" Compila con gcc -Wall -Wextra -std=c17 -pthread y deja el ejecutable torneo. Para borrar lo compilado: make clean. El -pthread del Makefile es necesario porque usamos hilos.

Ejecutar ./torneo N M [config.txt]

N: cantidad de jugadores (entero mayor a 0). M: máximo de mesas a la vez (entero mayor a 0). config.txt (opcional): archivo de configuración; si no se entrega se usan valores por defecto. Ejemplo:

./torneo 8 2 config/rapido.txt

Código de salida: 0 si el torneo terminó o se cerró con Ctrl+C, 1 si los argumentos o el config son inválidos.

Formato de config.txt Una clave por línea: clave=valor

Claves: delay_min y delay_max (retardo por movimiento en ms, por defecto 50 y 300), seed (semilla, por defecto la hora), puma (nodo inicial, por defecto 0), perros (los 12 nodos iniciales separados por coma) y prob_abandono (de 0 a 100, probabilidad de que un jugador falle a propósito en una partida, por defecto 0).

Ejemplo: perros=16,17,18,19,20,21,22,23,24,25,26,27

Los nodos van así: 0 es el trono, 1 y 2 el triángulo y la grilla del 3 al 27 (3 + 5 * fila + columna). Las líneas vacías o que parten con # se ignoran y lo que no aparece usa su valor por defecto. Si hay un delay mínimo mayor al máximo, un nodo fuera del tablero, dos fichas en el mismo nodo o una clave desconocida, avisa con el número de línea y no parte.

Funciones principales: -config_defaults / config_load / config_validar: valores por defecto, lectura del archivo y validación.

-tablero_iniciar / tablero_destruir: crean el tablero con su mutex y lo destruyen.

-movimientos_puma / movimientos_perro: calculan los pasos y saltos válidos de cada ficha.

-mover_puma / capturar_perro / mover_perro: modifican el tablero, siempre con el mutex tomado.

-revisar_ganador: gana el Puma con 6 capturas, ganan los Perros si el Puma queda sin movimientos.

-tablero_tomar / tablero_soltar: toman y sueltan el mutex, recuperándolo si su dueño murió.

-partida_jugar: el juez de la mesa. Lanza los 13 hilos, espera el fin de la partida, un abandono o Ctrl+C, y libera todo.

-hilo_puma / hilo_perro: el ciclo de cada ficha (dormir, tomar el mutex, mover, soltar).

-mm_iniciar / mm_ejecutar / mm_destruir: el servidor. Recibe jugadores, arma parejas y abre mesas.

Diseño: -Concurrencia: un hilo por ficha (13 por mesa), uno por jugador y un juez por mesa. Nunca hay más de M mesas activas.

-Exclusión mutua: un mutex por tablero. La ficha duerme fuera del mutex, y después lo toma, decide, mueve y lo suelta, así dos fichas nunca ocupan la misma casilla.

-Sin busy-waiting: el juez espera con una variable de condición y el servidor con una variable de condición y un semáforo. Despiertan cada 100 ms solo para revisar si hubo Ctrl+C, así no gastan CPU.

-Mesas: un semáforo parte en M. El servidor hace sem_wait antes de abrir una mesa y la mesa hace sem_post al terminar.

-Parejas: el primero que llega juega con el Puma y el segundo con los Perros. Si N es impar, el último queda sin pareja y se avisa.

-Abandonos: si un jugador falla, muere con el mutex del tablero tomado. El mutex es robusto: el siguiente hilo recibe EOWNERDEAD y lo deja consistente, y el juez da la victoria al oponente.

-Ctrl+C: el manejador solo levanta una bandera. El servidor deja de aceptar jugadores, las mesas terminan, se espera a todos los hilos y se destruyen mutex, condiciones, semáforo y memoria. Un segundo Ctrl+C cierra de inmediato. Los semáforos son anónimos, así que no quedan recursos huérfanos.

-Deadlock: no hay espera circular porque los mutex siempre se toman en el mismo orden.

-Config mala: cualquier error en config.txt se informa antes de empezar a jugar.

Pruebas: Con el programa compilado: make pruebas Corre las pruebas del tablero, del config, de las partidas (con abandonos), del servidor y del Ctrl+C. La de Ctrl+C usa AddressSanitizer, así que puede que no este instalado (me sucedio) en Fedora se tuvo que instalar antes libasan y libubsan sino tira error.