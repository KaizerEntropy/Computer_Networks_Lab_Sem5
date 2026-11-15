# Mini Group Project — Network, Transport & Application Layer Protocols (BOOTP, DHCP, TELNET)

**Course**: CSE/PC/B/S/314 Computer Networks Laboratory  
**Department**: Department of Computer Science & Engineering, Jadavpur University  
**Submission**: Mini Group Project (Basket 1: BOOTP, Basket 2: DHCP, Basket 3: TELNET)

---

## 1. Project Overview

This project provides an end-to-end, production-grade network protocol simulation implementing three foundational standards selected strictly across the assignment baskets:
1. **Basket 1 (Network Bootstrap)**: **BOOTP** (Bootstrap Protocol — RFC 951, RFC 1048, RFC 1542) over **UDP**
2. **Basket 2 (Dynamic Host Configuration)**: **DHCP** (Dynamic Host Configuration Protocol — RFC 2131, RFC 2132) over **UDP**
3. **Basket 3 (Application Layer Remote Shell)**: **TELNET** (Telecommunications Network Protocol — RFC 854, RFC 855) over **TCP**

The project seamlessly integrates all flow control, channel impairment, error detection, error injection, retransmission (ARQ), latency/jitter, and statistical analysis mechanisms developed across **Assignments 1, 2, 3, and 4**, elevating them into a unified, modular, and robust multi-protocol suite.

It is designed to run seamlessly either on a **single machine** (via loopback sockets) or across **separate physical computers** across a local area network (LAN) with configurable socket binding and network addressing.

---

## 2. Protocol Stack Architecture & Exchange Flow

The operational life-cycle of a client host proceeds through three sequential phases:

```
+-------------------------------------------------------------------------------+
|                       CLIENT INITIALIZATION & BOOTSTRAP                       |
+-------------------------------------------------------------------------------+
                                     |
                                     v
                        [Phase 1: BOOTP Bootstrap]
            Client boots diskless without IP -> Sends BOOTREQUEST (UDP:6767)
            Server maps client MAC -> Replies with BOOTREPLY (192.168.10.50,
            bootfile "pxelinux.0", server hostname, vendor options)
                                     |
                                     v
                        [Phase 2: DHCP DORA Handshake]
            Dynamic IP Lease Allocation via 4-Way Exchange:
              1. DHCPDISCOVER (UDP:6769, Client -> Broadcast)
              2. DHCPOFFER    (UDP:6769, Server -> Client: 192.168.10.100, Lease 7200s)
              3. DHCPREQUEST  (UDP:6769, Client -> Server: Requests offered IP)
              4. DHCPACK      (UDP:6769, Server -> Client: Commits lease & bindings)
                                     |
                                     v
                        [Phase 3: TELNET Remote Application]
            Application session over TCP Socket (TCP:2323):
              - TCP 3-Way Handshake
              - RFC 854 Option Negotiation (IAC DO TTYPE, WILL ECHO, WILL SGA)
              - Server Welcome Banner & Authentication
              - Transmission of Input File (commands / segmented data frames)
              - Stop-and-Wait ARQ & FCS Error Verification per Data Packet
              - Remote Shell Execution (help, status, network_info, calc, ping, upload)
                                     |
                                     v
                        [Phase 4: Session Termination]
            Client issues 'exit' -> TELNET closes -> DHCPRELEASE dispatched
```

---

## 3. Modular Code Organization

```
CN_Lab_Grp_Project/
├── include/
│   ├── common.hpp            # ANSI styling, hex dump, FrameHeader envelope, Statistics engine
│   ├── error_detection.hpp   # Checksum-16, CRC-8/10/16/32, Hamming SEC-DED, 2D Parity
│   ├── error_injection.hpp   # Single-bit, two-isolated, odd, burst, checksum/crc-blind
│   ├── channel.hpp           # Simulated channel: packet loss, corruption, delay, jitter
│   ├── bootp.hpp             # RFC 951/1048/1542 BOOTP packet header, serialization, vendor options
│   ├── dhcp.hpp              # RFC 2131/2132 DHCP packet layout, TLV options, DORA lease pool
│   └── telnet.hpp            # RFC 854/855 Telnet commands, NVT parser, remote command engine
├── src/
│   ├── error_detection.cpp   # Error detection mathematical implementations
│   ├── error_injection.cpp   # Algorithmic error injection & blind spot generators
│   ├── channel.cpp           # Channel impairment & latency engine
│   ├── bootp.cpp             # BOOTP packet builder, parser, and pretty-printer
│   ├── dhcp.cpp              # DHCP DORA state machine & lease management pool
│   └── telnet.cpp            # Telnet NVT engine & remote command interpreter
├── server.cpp                # Unified multi-protocol server daemon (0.0.0.0 bind)
├── client.cpp                # Multi-protocol client node (BOOTP -> DHCP -> TELNET)
├── simulator.cpp             # Standalone test bench (matrix benchmark & efficiency sweep)
├── generate_plots.py         # Matplotlib publication figures generator
├── run_all_tests.sh          # Comprehensive automated test runner
├── input_commands.txt        # Interactive input command script
├── sample_payload.txt        # Structured data transmission payload
├── Makefile                  # Optimized build automation
└── plots/                    # Generated high-resolution visualization charts
```

---

## 4. Error Detection & Checking Schemes

All error detection schemes from previous assignments are supported and selectable:

| Scheme Code | Scheme Name | Generator Polynomial / Method | Primary Applications |
|---|---|---|---|
| **1** | **16-bit Checksum** | One's complement sum of 16-bit words (RFC 1071) | IP, UDP, TCP headers |
| **2** | **CRC-8** | $x^8 + x^7 + x^6 + x^4 + x^2 + 1$ (`0xD5`) | ATM, Bluetooth, Sensors |
| **3** | **CRC-10** | $x^{10} + x^9 + x^5 + x^4 + x + 1$ (`0x233`) | Telecommunications, ATM AAL2 |
| **4** | **CRC-16** | $x^{16} + x^{15} + x^2 + 1$ (`0x8005`) | USB, Modbus, ANSI/IBM |
| **5** | **CRC-32** | IEEE 802.3 Ethernet Standard (`0x04C11DB7`) | Ethernet, Ethernet frames, ZIP, PNG |
| **6** | **Hamming SEC-DED** | Extended Multi-Block Syndrome Code | Error correction & detection |
| **7** | **2D Parity** | Longitudinal + Vertical Redundancy Check (LRC+VRC) | Matrix parity check |

---

## 5. Error Injection & Mathematical Blind-Spot Demonstrations

The channel simulator supports targeted or round-robin error injection:
1. **Single-Bit Error**: Flips exactly one bit in the frame.
2. **Two Isolated Bit Errors**: Flips 2 bits with distance $\ge 2$.
3. **Odd Number of Bit Errors**: Flips 3, 5, or 7 bits (guaranteed detection by parity/CRC).
4. **Burst Error (8-bit span)**: Corrupts an 8-bit contiguous sequence.
5. **Checksum-Blind Error**: Applies balanced $+1 / -1$ word modifications. Checksum remains invariant ($0\%$ detection), whereas CRC-32 detects it with $100\%$ accuracy!
6. **CRC-Blind Error**: Injects an error polynomial $E(x)$ matching a multiple of the generator polynomial $G(x)$. The CRC remainder is 0 ($0\%$ detection), whereas Checksum detects it with $100\%$ accuracy!
7. **Random Byte Corruption**: Injects multi-byte noise.

---

## 6. Channel Impairments & Flow Control (ARQ)

- **Packet Loss Simulation (`--prob-loss <p>`)**: Simulates packet drop in transit.
- **Bit Corruption (`--prob-error <p>`)**: Applies bit flips before socket delivery.
- **Random Delay & Jitter (`--prob-delay <p>`)**: Introduces $60\text{--}180\text{ ms}$ latency exceeding socket timeout to trigger premature timeouts.
- **ACK Loss (`--prob-ack-loss <p>`)**: Drops reverse acknowledgments.
- **Stop-and-Wait ARQ**: Automatically retransmits unacknowledged or NACKed frames with configurable timeout and retry limits, ensuring bit-exact data reconstruction.

---

## 7. How to Build & Run

### Step 1: Compilation
```bash
make clean && make all
```
This builds three standalone executables:
- `server`: Multi-protocol server daemon
- `client`: Multi-protocol client node
- `simulator`: Mathematical proof and channel benchmark

---

### Step 2: Running on a Single Machine (Localhost)

Open two terminal windows:

#### Terminal 1 — Server:
```bash
./server --ip 0.0.0.0 --bootp-port 6767 --dhcp-port 6769 --telnet-port 2323 --scheme 5
```

#### Terminal 2 — Client:
```bash
# Clean baseline test:
./client --server-ip 127.0.0.1 --bootp-port 6767 --dhcp-port 6769 --telnet-port 2323 \
         --scheme 5 --input input_commands.txt

# Test with 15% channel loss and 10% bit corruption:
./client --server-ip 127.0.0.1 --bootp-port 6767 --dhcp-port 6769 --telnet-port 2323 \
         --scheme 5 --prob-loss 0.15 --prob-error 0.10 --input input_commands.txt

# Interactive menu mode:
./client --interactive
```

---

### Step 3: Running on Separate Physical Devices (LAN)

Assume two laptops on the same Wi-Fi/Ethernet network:
- **Server Device IP**: `192.168.1.50`
- **Client Device IP**: `192.168.1.60`

#### On Server Device (`192.168.1.50`):
```bash
./server --ip 0.0.0.0 --bootp-port 6767 --dhcp-port 6769 --telnet-port 2323 --scheme 5
```
*(Binding to `0.0.0.0` allows incoming connections from all physical network interfaces)*.

#### On Client Device (`192.168.1.60`):
```bash
./client --server-ip 192.168.1.50 --bootp-port 6767 --dhcp-port 6769 --telnet-port 2323 \
         --scheme 5 --input input_commands.txt
```

---

### Step 4: Automated Test Harness & Plot Generation
```bash
bash run_all_tests.sh
```
This runs the full test matrix, executes socket tests under varying loss/error rates, validates blind-spot conditions, verifies bit-exact payload delivery, and generates plots in `plots/`:
- `plots/fig1_protocol_stack_flow.png`: Complete protocol sequence diagram
- `plots/fig2_error_detection_heatmap.png`: 7x7 Detection rate heatmap
- `plots/fig3_efficiency_vs_loss_rate.png`: Stop-and-Wait ARQ throughput efficiency curve
- `plots/fig4_rtt_latency_and_jitter.png`: Latency and network jitter distribution

---

## 8. Sample Terminal Output Excerpt

```
================================================================================
        CLIENT COMPLETE END-OF-RUN REPORT
================================================================================
 Execution Role          : Client Node
 Total Elapsed Time      : 421.36 ms
 Total Data Transferred  : 5145 bytes
--------------------------------------------------------------------------------
 PROTOCOL BREAKDOWN:
  * BOOTP Packets        : 2
  * DHCP DORA Packets    : 5
  * TELNET Application   : 34
--------------------------------------------------------------------------------
 PACKET TRANSMISSION METRICS:
  * Total Transmissions  : 24 (includes retransmissions)
  * Packets Received     : 22
  * Retransmissions      : 3
  * Channel Packets Lost : 2
  * Corrupted in Channel : 1
  * Delayed Packets      : 1
--------------------------------------------------------------------------------
 ERROR DETECTION VERIFICATION:
  * Errors Injected      : 1
  * Errors Detected (FCS): 1
  * Undetected (Blind)   : 0
  * Detection Success Rate: 100.00 %
--------------------------------------------------------------------------------
 ROUND TRIP TIME (RTT) & JITTER:
  * Minimum RTT          : 0.12 ms
  * Maximum RTT          : 122.45 ms
  * Average RTT          : 18.25 ms
  * Network Jitter       : 14.80 ms
--------------------------------------------------------------------------------
 Throughput Efficiency : 87.50 %
 Efficiency Bar Graph:
 [###########################################       ] 87.50 %
 Packet Allocation:
  - Unique Packets : [===========================================       ] 87%
  - Retransmitted  : [******                                            ] 13%
================================================================================
```

---

## 9. Verification & Bit-Exact Reconstruction

When transmitting the input payload through the Telnet data channel:
- Every data chunk is encapsulated in a `FrameHeader` tagged with `PROTO_TELNET_DATA`, sequence number, and FCS.
- The server writes received chunks to `server_received_payload.txt`.
- Verification using `diff -u` demonstrates $100\%$ bit-exact data delivery even in the presence of packet loss and corruption due to ARQ retransmissions.

