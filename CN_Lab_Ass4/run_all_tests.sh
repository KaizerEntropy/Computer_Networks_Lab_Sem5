#!/bin/bash
# ==============================================================================
#  run_all_tests.sh - Comprehensive Automated Verification Suite for Assignment 4
#  Tests:
#    1. Standalone Simulator Verification (Orthogonality, Bits, Strings, Noise)
#    2. Multi-Process Network Socket Communication (Channel + Receiver + 4 Senders)
#    3. 100% Bit-for-Bit Diff Comparison of Sample Transmissions
# ==============================================================================

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "======================================================================"
echo "    COMPILING CDMA PROJECT BINARIES VIA MAKE"
echo "======================================================================"
make clean
make

echo ""
echo "======================================================================"
echo "    PART 1: STANDALONE SIMULATOR BENCHMARK & MATHEMATICAL VERIFICATION"
echo "======================================================================"
./cdma_sim --auto

echo ""
echo "======================================================================"
echo "    PART 2: DISTRIBUTED MULTI-PROCESS UDP SOCKET NETWORK TEST"
echo "======================================================================"
PORT=8888
NUM_STATIONS=4

# Kill any lingering instances on this port
pkill -f "./channel $PORT" 2>/dev/null || true
pkill -f "./receiver $PORT" 2>/dev/null || true
sleep 0.5

# Step 1: Start Channel Coordinator in background
echo "[Test Harness] Starting Channel Coordinator on UDP port $PORT..."
./channel $PORT $NUM_STATIONS 0.0 > /tmp/cdma_channel_out.txt 2>&1 &
CHANNEL_PID=$!
sleep 0.8

# Step 2: Start Receiver in background (demuxing all stations)
echo "[Test Harness] Starting Multi-Station Receiver..."
./receiver $PORT -1 127.0.0.1 $NUM_STATIONS > /tmp/cdma_receiver_out.txt 2>&1 &
RECEIVER_PID=$!
sleep 0.8

# Step 3: Launch 4 Senders concurrently
echo "[Test Harness] Launching 4 Senders concurrently across shared channel..."
./sender 127.0.0.1 $PORT 0 sample_st0.txt $NUM_STATIONS > /tmp/cdma_st0.txt 2>&1 &
PID0=$!
./sender 127.0.0.1 $PORT 1 sample_st1.txt $NUM_STATIONS > /tmp/cdma_st1.txt 2>&1 &
PID1=$!
./sender 127.0.0.1 $PORT 2 sample_st2.txt $NUM_STATIONS > /tmp/cdma_st2.txt 2>&1 &
PID2=$!
./sender 127.0.0.1 $PORT 3 sample_st3.txt $NUM_STATIONS > /tmp/cdma_st3.txt 2>&1 &
PID3=$!

# Wait for senders to complete
wait $PID0 $PID1 $PID2 $PID3
echo "[Test Harness] All 4 senders have finished transmitting."

# Wait for receiver to decode and write files
wait $RECEIVER_PID 2>/dev/null || true

# Stop channel
kill $CHANNEL_PID 2>/dev/null || true

echo ""
echo "======================================================================"
echo "    PART 3: 100% BIT-FOR-BIT VERIFICATION (DIFF TEST)"
echo "======================================================================"

ALL_PASSED=true
for i in 0 1 2 3; do
    ORIGINAL="sample_st$i.txt"
    RECEIVED="received_station_$i.txt"
    if [ -f "$RECEIVED" ]; then
        if diff -u "$ORIGINAL" "$RECEIVED" > /dev/null; then
            echo -e "\e[32m[PASSED]\e[0m Station $i: $RECEIVED matches $ORIGINAL (100% Identical)"
        else
            echo -e "\e[31m[FAILED]\e[0m Station $i: Difference detected between $RECEIVED and $ORIGINAL"
            ALL_PASSED=false
        fi
    else
        echo -e "\e[31m[FAILED]\e[0m Station $i: Expected $RECEIVED not found!"
        ALL_PASSED=false
    fi
done

echo ""
if [ "$ALL_PASSED" = true ]; then
    echo "======================================================================"
    echo -e "   \e[32m\e[1mALL TESTS PASSED! CDMA WITH WALSH CODE FULLY VERIFIED!\e[0m"
    echo "======================================================================"
else
    echo -e "\e[31mSome tests failed. Check logs in /tmp/cdma_*.txt\e[0m"
    exit 1
fi
