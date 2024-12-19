#!/bin/bash

if (( $# != 2 )) ;then
    echo "?ERRORE, numero errato di parametri" >&2
else
    rm -f $1
    make
    ./program $1 $2
    cat $1
fi