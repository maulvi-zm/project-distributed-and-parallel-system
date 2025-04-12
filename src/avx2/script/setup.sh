#!/bin/bash

echo "Creating compiled code..."

gcc -mavx2 -O2 avx.c -o avx2 -lm -mfma

if [ $? -ne 0 ]; then
    echo "Error: Compilation failed."
    exit $?
fi

echo "Compiled code created successfully!"
