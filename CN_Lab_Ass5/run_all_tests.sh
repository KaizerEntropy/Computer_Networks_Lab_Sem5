#!/usr/bin/env bash
# run_all_tests.sh - Comprehensive automated verification test suite for Assignment 5

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "================================================================================"
echo "    COMPUTER NETWORKS LABORATORY - ASSIGNMENT 5 VERIFICATION SUITE              "
echo "================================================================================"

# 1. Clean previous build artifacts
echo "[*] Cleaning previous build artifacts..."
make clean > /dev/null 2>&1 || true

# 2. Build the C++ Packet Analyzer
echo "[*] Compiling Packet Analyzer (C++17)..."
make packet_analyzer

# 3. Generate standard PCAP capture file
echo "[*] Generating synthetic capture PCAP (network_capture.pcap)..."
python3 pcap_generator.py

# 4. Generate all high-resolution figures
echo "[*] Generating all 10 high-resolution analytical figures..."
python3 generate_plots.py

# 5. Run Analyzer Tests
echo ""
echo "================================================================================"
echo "[TEST 1] Unfiltered Packet List"
echo "================================================================================"
./packet_analyzer -l

echo ""
echo "================================================================================"
echo "[TEST 2] Display Filter: HTTP"
echo "================================================================================"
./packet_analyzer -Y http

echo ""
echo "================================================================================"
echo "[TEST 3] Display Filter: DNS"
echo "================================================================================"
./packet_analyzer -Y dns

echo ""
echo "================================================================================"
echo "[TEST 4] Packet Details Dissection: Frame 12 (HTTP GET)"
echo "================================================================================"
./packet_analyzer -d 12

echo ""
echo "================================================================================"
echo "[TEST 5] Packet Details Dissection: Frame 14 (HTTP 200 OK & Ethernet Layer)"
echo "================================================================================"
./packet_analyzer -d 14

echo ""
echo "================================================================================"
echo "[TEST 6] Packet Bytes Panel: Frame 12 (Hex & ASCII with Host Highlight)"
echo "================================================================================"
./packet_analyzer -b 12

echo ""
echo "================================================================================"
echo "[TEST 7] Follow TCP Conversation Stream"
echo "================================================================================"
./packet_analyzer --follow-stream

echo ""
echo "================================================================================"
echo "[TEST 8] Protocol Hierarchy Statistics"
echo "================================================================================"
./packet_analyzer -z hierarchy

echo ""
echo "================================================================================"
echo "[TEST 9] Sequence Flow Graph"
echo "================================================================================"
./packet_analyzer -z flow

echo ""
echo "================================================================================"
echo "[TEST 10] Complete Automated Assignment 5 Questions (Q1 - Q10)"
echo "================================================================================"
./packet_analyzer --analyze-all

echo ""
echo "================================================================================"
echo "[*] ALL TESTS PASSED SUCCESSFULLY!"
echo "================================================================================"

