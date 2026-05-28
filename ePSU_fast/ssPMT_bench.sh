#!/usr/bin/env bash
set -e

NN=${1:-14}
BIN="./build/ssPMT"

echo "[INFO] Start Party 0"
$BIN -r 0 -nn "$NN" &
P0_PID=$!

sleep 2

echo "[INFO] Start Party 1"
$BIN -r 1 -nn "$NN" &
P1_PID=$!

wait "$P0_PID"
wait "$P1_PID"

echo "[SUCCESS] Finished."