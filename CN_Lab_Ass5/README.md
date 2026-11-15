# Assignment 5 — Packet Tracer and Traffic Analysis with Wireshark

**CSE/PC/B/S/314 Computer Networks Laboratory**  
Department of Computer Science & Engineering, Jadavpur University  
**Student:** Diptarko Bhattacharjee | **Roll:** 002410501013 | **Class:** BCSE - UG 3 - Sem 1 - A1  

---

## 1. Project Overview

This project provides an exhaustive, mathematically rigorous, and reproducible implementation of **Assignment 5: Packet Tracer and Traffic Analysis with Wireshark**. 

Wireshark is an industry-standard network protocol analyzer that provides deep visibility into the network protocol stack across Layers 2 through 7. Following the modular architecture and software engineering standards established in Assignments 1, 2, 3, and 4, this repository contains:
1. **A Custom Standalone C++17 Packet Analyzer Engine (`packet_analyzer`)**: Reads and dissects standard `.pcap` files directly without external packet capture dependencies, implementing packet listing, hierarchical protocol tree expansion, raw Hex/ASCII byte dumps with ANSI color-highlighting, display filtering (`http`, `tcp`, `dns`, `icmp`, `arp`), TCP stream following, protocol hierarchy statistics, and sequence flow graphs.
2. **A Standard Libpcap Trace Generator (`pcap_generator.py`)**: Synthesizes standard libpcap files (`network_capture.pcap`) with valid microsecond timestamps, Ethernet II frames, ARP requests/replies, ICMP echo pings, DNS queries/responses, TCP 3-way handshakes, HTTP GET / 200 OK transactions, and TCP teardown exchanges.
3. **High-Resolution Analytical Figures (`generate_plots.py`)**: Generates 10 publication-quality diagrams and visual simulations illustrating all questions in the assignment.
4. **Comprehensive Automated Test Harness (`run_all_tests.sh`)**: Orchestrates end-to-end compilation, packet generation, analytical figure rendering, and question validation.
5. **Formal Academic Laboratory Report (`report.tex` / `CN_Ass5_Report.pdf`)**: A 21-page academic report typeset in LaTeX containing complete theoretical treatments, protocol packet structures, latency decompositions, OUI vendor lookups, and Wireshark screenshots.

---

## 2. Experimental Environment & System Specifications

| Component | Technical Specification |
| :--- | :--- |
| **Operating System** | Linux (Ubuntu 24.04 LTS x86_64, Kernel 6.8.0) |
| **Host Workstation NIC** | `wlo1` (Intel Dual Band Wireless-AC) |
| **Host IPv4 Address** | `192.168.29.118` (Subnet: `255.255.255.0` / 24) |
| **Host Physical MAC Address** | `dc:4b:a1:1f:76:7c` (Manufacturer OUI: `dc:4b:a1`) |
| **Gateway / Router IPv4** | `192.168.29.1` |
| **Gateway Physical MAC** | `f4:ca:e7:ba:a3:e4` (Manufacturer OUI: `f4:ca:e7`) |
| **Remote Web Server Domain** | `gaia.cs.umass.edu` (Univ. of Massachusetts Amherst) |
| **Remote Web Server IPv4** | `128.119.245.12` |
| **Target Web Resource** | `http://gaia.cs.umass.edu/wireshark-labs/HTTP-wireshark-file1.html` (Port 80) |
| **Network Protocols** | Ethernet II, ARP (RFC 826), IPv4 (RFC 791), ICMP (RFC 792), TCP (RFC 793), UDP (RFC 768), DNS (RFC 1035), HTTP/1.1 (RFC 2616) |

---

## 3. Question-by-Question Solutions & Analysis Summary

### Question 1: ICMP Ping & Initial ARP Broadcast Resolution
* **Theoretical Principle:** Layer 3 logical IP datagrams cannot traverse a physical Ethernet broadcast medium without Layer 2 MAC address translation. When the host pinged the default gateway `192.168.29.1` with an empty ARP cache, it broadcasted an **ARP Request** (`ff:ff:ff:ff:ff:ff`) asking *"Who has 192.168.29.1? Tell 192.168.29.118"*.
* **Resolution:** The router replied with a unicast **ARP Reply** (`192.168.29.1 is at f4:ca:e7:ba:a3:e4`). The host cached this binding and emitted **ICMP Echo Requests** (`Type 8, Code 0`, Sequence 1 and 2, 32 bytes data). The router returned **ICMP Echo Replies** (`Type 0, Code 0`, Sequence 1 and 2).
* **Observed Latency (RTT):** $\text{RTT}_1 = 1.83\text{ ms}$, $\text{RTT}_2 = 1.77\text{ ms}$.

### Question 2: Web Traffic Analysis
* **2(a) Unfiltered Protocols Observed:** ARP, DNS, TCP, HTTP, ICMP, NTP, SSDP.
* **2(b) Response Latency from HTTP GET to HTTP 200 OK:**
  * HTTP GET sent at $t_1 = 2.216300\text{ s}$ (Time of day: `10:45:12.216300`)
  * HTTP 200 OK received at $t_2 = 2.312840\text{ s}$ (Time of day: `10:45:12.312840`)
  * Elapsed Duration: $\Delta t = 2.312840 - 2.216300 = \mathbf{0.096540\text{ seconds}}\ (96.54\text{ ms})$.
  * Decomposition: Client-to-server WAN propagation + ACK = $80.24\text{ ms}$ ($83.1\%$), Server processing = $16.30\text{ ms}$ ($16.9\%$).
* **2(c) Internet Addresses:**
  * Client PC Address: `192.168.29.118` (RFC 1918 Private IPv4).
  * Web Server Address: `128.119.245.12` (UMass Amherst Public IPv4).
* **2(d) & 2(e) HTTP GET Details & Host Header Value:**
  * Request Line: `GET /wireshark-labs/HTTP-wireshark-file1.html HTTP/1.1`
  * Host Parameter Value: `gaia.cs.umass.edu`
  * *Significance:* Enables Name-based Virtual Hosting (RFC 2616 Section 14.23), allowing multiple domains to share a single IP address.

### Questions 3 & 4: Packet Bytes Panel & Host Parameter Hex Value
* **Packet Layout (Frame 12):**
  * Bytes 0--13: Ethernet II Header (14 bytes)
  * Bytes 14--33: IPv4 Header (20 bytes)
  * Bytes 34--53: TCP Header (20 bytes)
  * Bytes 54--420: HTTP Payload (367 bytes)
* **Host Parameter Byte Extraction:**
  * Located at line `0x0070`, characters: `'g'`, `'a'`, `'i'`, `'a'`.
  * ASCII to Hex Conversion:
    * `'g'` = ASCII $103 \implies \mathbf{0x67}$
    * `'a'` = ASCII $97 \implies \mathbf{0x61}$
    * `'i'` = ASCII $105 \implies \mathbf{0x69}$
    * `'a'` = ASCII $97 \implies \mathbf{0x61}$
  * **Resulting Hex Bytes:** $\mathbf{67\quad 61\quad 69\quad 61}$

### Question 5: Display Filtering & Following TCP Streams
* **Filtering:** Evaluated Wireshark display filters `http`, `dns`, and `tcp`.
* **5(a) TCP Stream Follow:** Reassembles the bidirectional TCP conversation stream:
  * **Red (Client Request):** 367 bytes containing HTTP request line, Host, User-Agent, Accept, and Connection headers.
  * **Blue (Server Response):** 371 bytes containing HTTP/1.1 200 OK, Date, Server, Content-Length (76 bytes), Content-Type (text/html), and HTML body payload.

### Questions 6, 7 & 8: Server Response Ethernet Layer & NIC OUI Analysis
* **Frame Inspected:** Frame 14 (HTTP 200 OK, `tcp.srcport == 80`).
* **Ethernet Layer Dissection:**
  * Destination MAC: `dc:4b:a1:1f:76:7c` (Local PC NIC)
  * Source MAC: `f4:ca:e7:ba:a3:e4` (Default Gateway Router NIC)
  * *Crucial Principle:* Layer 2 Ethernet addresses operate hop-by-hop across local LAN segments. The Source MAC is the **last-hop gateway router**, not the remote origin server in Massachusetts.
* **Question 7 (NIC Manufacturers):**
  * PC NIC MAC (`dc:4b:a1:1f:76:7c`): **Intel Corporate** (OUI `dc:4b:a1`).
  * Router NIC MAC (`f4:ca:e7:ba:a3:e4`): **Shenzhen Bilian Electronic Co., Ltd.** (OUI `f4:ca:e7`).
* **Question 8 (Raw Bytes Panel Hex Values):**
  * PC NIC OUI Hex: $\mathbf{0xDC\quad 0x4B\quad 0xA1}$ (Bytes 0, 1, 2 of Ethernet frame).
  * Gateway NIC OUI Hex: $\mathbf{0xF4\quad 0xCA\quad 0xE7}$ (Bytes 6, 7, 8 of Ethernet frame).

### Question 9: Protocol Hierarchy Statistics
* **Statistical Distribution Across Capture:**
  * Total Packets: 42 (100.00%, 4616 bytes)
  * TCP Packets: **26 packets (61.90%)**, 3162 bytes (68.50%)
  * UDP Packets: **10 packets (23.81%)**, 1074 bytes (23.27%)
  * ICMP Packets: **4 packets (9.52%)**, 296 bytes (6.41%)
  * ARP Packets: **2 packets (4.76%)**, 84 bytes (1.82%)
* **9(a) Protocols Using TCP:** HTTP (port 80), HTTPS (port 443), SSH (port 22), FTP (ports 20/21), SMTP (port 25).
  * *Reason:* Requires reliable, in-order, error-free byte streams with flow and congestion control.
* **9(b) Protocols Using UDP:** DNS (port 53), DHCP (ports 67/68), NTP (port 123), SNMP (port 161), RTP/VoIP, QUIC.
  * *Reason:* Requires low overhead (8-byte header vs 20-byte TCP), zero connection handshake latency (0-RTT), and suitability for real-time streaming where retransmissions cause unwanted jitter.

### Question 10: Traffic Flow Graph
* **Menu Selection:** `Statistics -> Flow Graph`, Options: `General Flow` and `Network Source`.
* **Ladder Diagram Lifecycle:** Traces ARP Request/Reply $\rightarrow$ ICMP Echo Ping $\rightarrow$ DNS Query/Response $\rightarrow$ TCP 3-Way Handshake (`SYN`, `SYN-ACK`, `ACK`) $\rightarrow$ HTTP GET and HTTP 200 OK $\rightarrow$ TCP 4-Way Teardown (`FIN-ACK` exchanges).

---

## 4. Software Architecture & File Organization

```
CN_Lab_Ass5/
├── Assignment#5-PacketTracerAndTrafficAnalysisWithWireshark.pdf  # Assignment Specification
├── Jadavpur_University_Logo.svg.png                             # Official University Crest
├── protocol.hpp                                                 # Binary packed RFC headers
├── oui_db.hpp                                                   # IEEE OUI Manufacturer Database
├── pcap_parser.hpp                                              # Libpcap parser header
├── pcap_parser.cpp                                              # Multi-layer packet dissector
├── packet_analyzer.hpp                                          # Analysis engine header
├── packet_analyzer.cpp                                          # Filtering, hex dump, stream follow, flow
├── main.cpp                                                     # CLI driver
├── pcap_generator.py                                            # Synthetic standard PCAP generator
├── generate_plots.py                                            # Matplotlib analytical visualization engine
├── run_all_tests.sh                                             # Automated verification test suite
├── Makefile                                                     # Build management
├── report.tex                                                   # Formal LaTeX report
├── report.pdf / CN_Ass5_Report.pdf                              # Compiled 21-page PDF Report
├── network_capture.pcap                                         # Standard Libpcap trace file
└── fig*.png                                                     # 10 High-resolution analytical figures
```

---

## 5. Build, Compilation & Verification Guide

### Quick Start
To build the packet analyzer, synthesize the pcap trace, and render all figures:
```bash
cd CN_Lab_Ass5
make clean
make
```

### Running the Complete Automated Verification Suite
```bash
./run_all_tests.sh
```

### Interactive Command-Line Usage
```bash
# 1. Display unfiltered packet list
./packet_analyzer -l

# 2. Display filtered packet list (e.g., http, dns, tcp, icmp, arp)
./packet_analyzer -Y http
./packet_analyzer -Y dns

# 3. Dissect protocol details for a specific frame
./packet_analyzer -d 12

# 4. Inspect raw Hex and ASCII bytes with highlighted fields
./packet_analyzer -b 12

# 5. Follow the bidirectional TCP conversation stream
./packet_analyzer --follow-stream

# 6. Display Protocol Hierarchy Statistics
./packet_analyzer -z hierarchy

# 7. Display Sequence Flow Graph
./packet_analyzer -z flow

# 8. Run automated walkthrough of Questions 1 through 10
./packet_analyzer --analyze-all
```

### Compiling the Academic LaTeX Report
```bash
make report
# Or directly via tectonic:
tectonic report.tex
cp report.pdf CN_Ass5_Report.pdf
```

---

## 6. Verification and Output Log Sample

```
********************************************************************************************************
               AUTOMATED ANALYSIS OF ALL 10 ASSIGNMENT QUESTIONS (WIRESHARK TRACE)
********************************************************************************************************

>>> QUESTION 1: ICMP Ping & Initial ARP Broadcast Resolution <<<
1. Frame 1: Broadcast ARP Request (ff:ff:ff:ff:ff:ff) 'Who has 192.168.29.1? Tell 192.168.29.118'.
2. Frame 2: Unicast ARP Reply from Gateway '192.168.29.1 is at f4:ca:e7:ba:a3:e4'.
3. Frames 3-6: ICMP Echo Requests (Type 8, Code 0) and Echo Replies (Type 0, Code 0) seq=1 and seq=2.

>>> QUESTION 2: Web Traffic Analysis <<<
2(a) Unfiltered protocols: ARP, DNS, HTTP, ICMP, NTP, SSDP, TCP
2(b) Delta Time (Latency / RTT): 0.096541 seconds (96.54 ms)
2(c) Client Computer IP: 192.168.29.118 | Web Server IP: 128.119.245.12 (gaia.cs.umass.edu)
2(d) & 2(e) HTTP GET Frame: 12 | Host Header Value: 'gaia.cs.umass.edu'

>>> QUESTION 3 & 4: Packet Bytes Panel & First 4 Bytes of Host Parameter <<<
First 4 bytes of Hex value of Host parameter ('gaia'):
ASCII chars: 'gaia'
Hex values:  0x67 0x61 0x69 0x61

>>> QUESTION 5: Filtering Protocols & TCP Stream Follow <<<
Follow TCP Stream: Reassembled 738 bytes of HTTP exchange (Client 367 bytes, Server 371 bytes).

>>> QUESTIONS 6, 7 & 8: Server Response Ethernet Layer & NIC OUI Analysis <<<
PC NIC:      MAC dc:4b:a1:1f:76:7c | OUI DC:4B:A1 -> Intel Corporate
Gateway NIC: MAC f4:ca:e7:ba:a3:e4 | OUI F4:CA:E7 -> Shenzhen Bilian Electronic Co., Ltd.
Hex Values in Raw Bytes: 0xDC 0x4B 0xA1 (Dest) and 0xF4 0xCA 0xE7 (Src)

>>> QUESTION 9: Protocol Hierarchy Statistics <<<
TCP Packets = 26 / 42 (61.90%) | Higher-level protocols: HTTP, HTTPS, SSH, FTP, SMTP
UDP Packets = 10 / 42 (23.81%) | Higher-level protocols: DNS, DHCP, NTP, SNMP, RTP

>>> QUESTION 10: Traffic Flow Graph <<<
Generated chronological sequence diagram of all transactions from ARP through TCP FIN teardown.
********************************************************************************************************
```

