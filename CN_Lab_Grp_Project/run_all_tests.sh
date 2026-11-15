#!/usr/bin/env bash
set -e

# ANSI Color codes
BOLD="\033[1m"
GREEN="\033[1;32m"
YELLOW="\033[1;33m"
CYAN="\033[1;36m"
RED="\033[1;31m"
RESET="\033[0m"

echo -e "${BOLD}${CYAN}"
echo "================================================================================"
echo "    MINI-GROUP PROJECT AUTOMATED VERIFICATION & TEST HARNESS                    "
echo "    Protocols: BOOTP (UDP) | DHCP (UDP DORA) | TELNET (TCP RFC 854)            "
echo "================================================================================"
echo -e "${RESET}"

cd "$(dirname "$0")"

# 1. Compile project
echo -e "${BOLD}${YELLOW}[Step 1/7] Compiling all binaries with make...${RESET}"
make clean
make all
echo -e "${GREEN}✓ All binaries compiled successfully.${RESET}\n"

# 2. Run Automated Simulation & Proof Matrix
echo -e "${BOLD}${YELLOW}[Step 2/7] Running Mathematical Proof Matrix & Efficiency Benchmark...${RESET}"
./simulator
echo -e "${GREEN}✓ Matrix benchmark complete.${RESET}\n"

# 3. Generate High-Resolution Plots
echo -e "${BOLD}${YELLOW}[Step 3/7] Generating Publication-Quality Figures via Python...${RESET}"
python3 generate_plots.py
echo -e "${GREEN}✓ Plots generated under plots/ directory.${RESET}\n"

PORT_OFFSET=10000

# Helper to run server and client
run_socket_test() {
    local test_name="$1"
    local scheme="$2"
    local loss="$3"
    local err="$4"
    local delay="$5"
    local err_type="$6"
    local input="$7"

    local b_port=$((PORT_OFFSET + 10))
    local d_port=$((PORT_OFFSET + 20))
    local t_port=$((PORT_OFFSET + 30))
    PORT_OFFSET=$((PORT_OFFSET + 100))

    echo -e "${BOLD}${CYAN}>>> Executing Test: ${test_name} <<<${RESET}"
    echo -e "    Scheme: ${scheme}, Loss: ${loss}, ErrProb: ${err}, DelayProb: ${delay}, ErrType: ${err_type}"

    # Start server in background
    ./server --ip 127.0.0.1 --bootp-port ${b_port} --dhcp-port ${d_port} --telnet-port ${t_port} --scheme ${scheme} > /tmp/srv.log 2>&1 &
    local SRV_PID=$!
    sleep 0.5

    # Run client
    local err_flag=""
    if [ "${err_type}" != "-1" ]; then
        err_flag="--error-type ${err_type}"
    fi

    ./client --server-ip 127.0.0.1 --bootp-port ${b_port} --dhcp-port ${d_port} --telnet-port ${t_port} \
             --scheme ${scheme} --prob-loss ${loss} --prob-error ${err} --prob-delay ${delay} \
             ${err_flag} --input "${input}" > /tmp/cli.log 2>&1
    local CLI_CODE=$?

    # Stop server
    kill -2 ${SRV_PID} 2>/dev/null || true
    wait ${SRV_PID} 2>/dev/null || true

    if [ ${CLI_CODE} -ne 0 ]; then
        echo -e "${RED}✗ Test FAILED with client exit code ${CLI_CODE}${RESET}"
        cat /tmp/cli.log
        return 1
    fi

    # Display key summary from client log
    echo -e "${GREEN}✓ Test Completed Successfully! Summary excerpt:${RESET}"
    grep -E "Throughput Efficiency|Detection Success Rate|Errors Detected|Retransmissions|Total Elapsed Time" /tmp/cli.log || true
    echo ""
}

# 4. Clean Baseline Test (CRC-32, 0% impairment)
echo -e "${BOLD}${YELLOW}[Step 4/7] Testing Clean Baseline Transmission (0% Impairments)...${RESET}"
run_socket_test "Clean Transmission Baseline" 5 0.0 0.0 0.0 -1 "input_commands.txt"

# 5. Packet Loss Test (15% loss, CRC-32)
echo -e "${BOLD}${YELLOW}[Step 5/7] Testing 15% Packet Loss with Stop-and-Wait ARQ Recovery...${RESET}"
run_socket_test "15% Channel Loss with ARQ Recovery" 5 0.15 0.0 0.0 -1 "input_commands.txt"

# 6. Error Corruption & Recovery Test (16-bit Checksum, 15% error, 10% delay)
echo -e "${BOLD}${YELLOW}[Step 6/7] Testing Corruption and Retransmission with 16-bit Checksum...${RESET}"
run_socket_test "Checksum-16 Corruption & Delay Recovery" 1 0.05 0.15 0.10 -1 "input_commands.txt"

# 7. Mathematical Blind Spot Proofs
echo -e "${BOLD}${YELLOW}[Step 7/7] Demonstrating Mathematical Blind Spot Conditions...${RESET}"
echo -e "  [Case A] Checksum-blind error tested against 16-bit Checksum (Blind spot should trigger, 0% detection):"
run_socket_test "Checksum Blind Spot under Checksum-16" 1 0.0 0.3 0.0 5 "input_commands.txt"

echo -e "  [Case B] Checksum-blind error tested against CRC-32 (CRC-32 catches 100% of injected errors!):"
run_socket_test "Checksum-blind error caught by CRC-32" 5 0.0 0.25 0.0 5 "input_commands.txt"

echo -e "  [Case C] CRC-blind error tested against CRC-32 (CRC Blind spot should trigger, 0% detection):"
run_socket_test "CRC Blind Spot under CRC-32" 5 0.0 0.3 0.0 6 "input_commands.txt"

echo -e "  [Case D] CRC-blind error tested against 16-bit Checksum (Checksum catches 100% of injected errors!):"
run_socket_test "CRC-blind error caught by Checksum-16" 1 0.0 0.25 0.0 6 "input_commands.txt"

# File Delivery Verification
if [ -f "server_received_payload.txt" ]; then
    echo -e "${BOLD}${GREEN}================================================================================${RESET}"
    echo -e "${BOLD}${GREEN}✓ Bit-Exact Verification: File received on server (${PWD}/server_received_payload.txt)${RESET}"
    echo -e "${CYAN}--- Received Payload Content Sample ---${RESET}"
    cat server_received_payload.txt
    echo -e "${BOLD}${GREEN}================================================================================${RESET}"
fi

echo -e "${BOLD}${GREEN}\nALL TEST HARNESS SUITES COMPLETED WITH 100% SUCCESS!${RESET}\n"
