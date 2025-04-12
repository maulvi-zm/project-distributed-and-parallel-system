#!/bin/bash

echo "Creating compiled code..."

gcc mp.c -o mp -fopenmp -lm

if [ $? -ne 0 ]; then
    echo "Error: Compilation failed."
    exit $?
fi

echo "Compiled code created successfully!"
