#!/bin/bash

TESTCASE=$1

if [ -z "$TESTCASE" ]; then
    echo "Usage: ./run.sh <test_num>"
    exit 1
fi

echo "Running test case $TESTCASE..."

if [ ! -d "output" ]; then
    mkdir "output"
fi

if [ ! -f "output/out-$TESTCASE.txt" ]; then
    touch "output/out-$TESTCASE.txt"
fi

if [ ! -f "test_case/case$TESTCASE.txt" ]; then
    echo "Error: Test case file test_case/case$TESTCASE.txt does not exist."
    exit 1
fi

./avx2 <"test_case/case$TESTCASE.txt" >"output/out-$TESTCASE.txt"

if [ $? -ne 0 ]; then
    echo "Error: Test case $TESTCASE failed."
    exit $?
fi

echo "Test case $TESTCASE completed. Output saved to output/out-$TESTCASE.txt."
