#!/bin/bash

if [ -z "$TRAVIS_TAG" ]; then
    echo "Not a tagged build, skipping building extra GCC"
    exit 0
fi

make extra-gcc -j5 >./extra-gcc.log 2>&1 &
PID=$!

while [ -d /proc/$PID ]
do
    echo "Building GCC..."
    sleep 10s
done

wait $PID
CODE=$?

echo "GCC extra build status code: $CODE"

if [ $CODE -ne 0 ]; then
    echo "GCC EXTRA BUILD ERROR:"
    cat ./extra-gcc.log
fi

exit $CODE