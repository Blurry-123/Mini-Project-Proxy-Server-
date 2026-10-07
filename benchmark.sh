#!/bin/bash

PROXY="http://127.0.0.1:8080"
SERVER="http://127.0.0.1:9000/test.txt"

echo "====================================="
echo "       PROXY PERFORMANCE TEST"
echo "====================================="

echo ""
echo "1. DIRECT CONNECTION"
echo "-------------------------------------"

curl \
    --noproxy "*" \
    -o /dev/null \
    -s \
    -w "Time: %{time_total} seconds\n" \
    "$SERVER"


echo ""
echo "2. PROXY - CACHE MISS"
echo "-------------------------------------"

rm -f cache/*.cache

curl \
    --noproxy "" \
    -x "$PROXY" \
    -o /dev/null \
    -s \
    -w "Time: %{time_total} seconds\n" \
    "$SERVER"


echo ""
echo "3. PROXY - CACHE HIT"
echo "-------------------------------------"

curl \
    --noproxy "" \
    -x "$PROXY" \
    -o /dev/null \
    -s \
    -w "Time: %{time_total} seconds\n" \
    "$SERVER"


echo ""
echo "====================================="
echo "              DONE"
echo "====================================="