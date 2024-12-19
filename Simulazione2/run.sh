#!/bin/bash

if (( $# != 2 )); then
    echo ?ERROR wrong number of parameters >&2
else
    make
    ./app $1 $2
fi