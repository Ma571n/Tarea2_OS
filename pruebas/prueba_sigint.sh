#!/bin/bash

cd "$(dirname "$0")/.." || exit 1
FUENTES="main.c config/config.c tablero/tablero.c partida/partida.c matchmaker/matchmaker.c"

gcc -Wall -Wextra -std=c17 -pthread -fsanitize=address,undefined -g -o /tmp/torneo_asan $FUENTES || exit 1
printf 'delay_min=200\ndelay_max=300\nseed=1\n' > /tmp/config_lenta.txt

fallas=0
fallar() { echo "FALLA: $1"; fallas=$((fallas + 1)); }

for espera in 0.1 1.5 3; do
    shm_antes=$(ls /dev/shm 2>/dev/null | wc -l)
    /tmp/torneo_asan 12 2 /tmp/config_lenta.txt > /tmp/salida_sigint.txt 2>&1 &
    pid=$!
    sleep "$espera"
    kill -INT "$pid" 2>/dev/null
    inicio=$(date +%s)
    wait "$pid"
    codigo=$?
    duracion=$(( $(date +%s) - inicio ))
    shm_despues=$(ls /dev/shm 2>/dev/null | wc -l)

    [ "$codigo" -eq 0 ]            || fallar "(${espera}s) código de salida $codigo (23 = fugas de memoria)"
    [ "$duracion" -le 3 ]          || fallar "(${espera}s) tardó ${duracion}s en cerrar"
    grep -q "Interrumpido con Ctrl+C" /tmp/salida_sigint.txt || fallar "(${espera}s) no avisó que lo interrumpieron"
    grep -q "ERROR: " /tmp/salida_sigint.txt && fallar "(${espera}s) AddressSanitizer reportó errores"
    [ "$shm_antes" -eq "$shm_despues" ] || fallar "(${espera}s) quedaron cosas en /dev/shm"
done

if [ "$fallas" -eq 0 ]; then
    echo "Todas las pruebas de SIGINT pasaron"
else
    echo "$fallas prueba(s) fallaron"
    exit 1
fi
