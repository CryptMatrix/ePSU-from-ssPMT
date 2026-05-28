#!/usr/bin/env bash


# =================setting=================
BIN="$(cd "$(dirname "$0")" && pwd)/build/ssPMT"
LOG_DIR="./logs_ssPMT_$(date +%Y%m%d_%H%M%S)"
NN_LIST=(14 16 18 20 22)
NS_NAME="sspmt_bench_ns"

NET_CONFIGS=(
    "LAN|10gbit|100us"
    "WAN_100Mbps|100mbit|40ms|limit 20000"
    "WAN_10Mbps|10mbit|40ms|limit 20000"
)
# ===========================================

if [ "$EUID" -ne 0 ]; then
  echo "[ERR] sudo is required"
  exit 1
fi

REAL_USER=${SUDO_USER:-$(whoami)}
mkdir -p "$LOG_DIR"
chown $REAL_USER "$LOG_DIR"
echo "[INFO] logs saved to: $LOG_DIR"

trap "ip netns del $NS_NAME 2>/dev/null; exit" INT TERM EXIT

for config in "${NET_CONFIGS[@]}"; do
    IFS='|' read -r NET_NAME BANDWIDTH DELAY EXTRA_ARGS <<< "$config"

    echo "=========================================================="
    echo "[INFO] net setting: $NET_NAME ($BANDWIDTH, $DELAY)"
    echo "=========================================================="

    ip netns del $NS_NAME 2>/dev/null
    ip netns add $NS_NAME
    ip netns exec $NS_NAME ip link set dev lo up
    ip netns exec $NS_NAME tc qdisc add dev lo root netem delay $DELAY rate $BANDWIDTH $EXTRA_ARGS

    LOG="$LOG_DIR/${NET_NAME}.log"

    for NN in "${NN_LIST[@]}"; do
        echo -n "[INFO] NN=2^$NN ... "

        {
            echo "=============================================="
            echo "  setting: $NET_NAME ($BANDWIDTH, $DELAY)  NN=2^$NN"
            echo "=============================================="
        } >> "$LOG"

        # Role 0 starts first, listens for connection
        ip netns exec $NS_NAME sudo -u $REAL_USER $BIN -r 0 -nn "$NN" >> "$LOG" 2>&1 &
        P0_PID=$!

        sleep 2

        # Role 1 starts, connects to Role 0
        ip netns exec $NS_NAME sudo -u $REAL_USER $BIN -r 1 -nn "$NN" >> "$LOG" 2>&1 &
        P1_PID=$!

        wait "$P0_PID"
        wait "$P1_PID"

        echo "[SUCCESS] Finished." >> "$LOG"
        echo "✅"
    done

    ip netns del $NS_NAME
done

echo "=========================================================="
echo "[INFO] bench results saved to: $LOG_DIR"
