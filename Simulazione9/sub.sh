#!/bin/bash

if (( $# != 1 )); then
    echo "?ERROR wrong number of parameters"
elif (( $1 > 10 || $1 <1 )); then
    echo "?ERROR number must between 1 and 10"
else
    for i in $(seq 1 $1); do
        (echo $BASHPID)
    done
fi
    