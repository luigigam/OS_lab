#!/usr/bin/env bash

line_number=0
while read -r line; do
    echo "$line" >&$((line_number%2+1))
    ((line_number++))
done <$1
echo "$line" >&$((line_number%2+1))
