# Assignment 6 — Cisco Packet Tracer Network Design, Enterprise Services, and Discrete-Event Protocol Simulation

**CSE/PC/B/S/314 Computer Networks Laboratory**  
Department of Computer Science & Engineering, Jadavpur University  
**Author:** Diptarko Bhattacharjee | **Roll:** 002410501013 | **Class:** BCSE UG-3 Sem-1 A1  

---

## 1. Project Overview

This project provides an exhaustive, dual-faceted implementation of **Assignment 6: Cisco Packet Tracer Network Experiments**:
1. **Industry-Standard Cisco Packet Tracer Configurations:** Complete step-by-step device configurations, network topology layouts, and Cisco IOS configuration scripts (`.ios` / `.cfg`) for Cisco Catalyst 2950 switches, Cisco 1818/1841 routers, DHCP pools, DNS zone databases, HTTP web daemons, and FTP servers.
2. **Modular C++17 Discrete-Event Network Simulator:** A standalone, object-oriented networking engine modeling physical ports, Ethernet II framing, ARP resolution, CAM table backward learning, Layer-3 static routing, DHCP 4-way DORA state machines, DNS name resolution, HTTP request-response parsing, and interactive FTP sessions.
3. **Publication-Quality Visualizations:** Matplotlib and NetworkX topology diagrams, protocol state machine flows, and empirical latency comparison charts.
4. **26-Page Academic Lab Report:** Authored in LaTeX (`report.tex`) and compiled into `report.pdf` and `CN_Ass6_Report.pdf`, featuring Jadavpur University formatting, full theoretical foundations, topology matrices, configuration guides, experimental tables, and analytical observations.

---

## 2. Theoretical Foundations

### 2.1 Physical Layer & Twisted-Pair Cabling Pinouts
Ethernet over twisted-pair copper cabling (IEEE 802.3 100BASE-TX) uses four pairs of copper wires terminated in 8-pin RJ-45 connectors:
- **Pins 1 & 2:** Transmit pair ($Tx^+ / Tx^-$) on Data Terminal Equipment (DTE).
- **Pins 3 & 6:** Receive pair ($Rx^+ / Rx^-$) on Data Terminal Equipment (DTE).

When connecting like devices (DTE to DTE, such as PC to PC or Router to Router) without Auto-MDIX:
- **Straight-Through Cable:** Pins connect 1-to-1 ($Tx \rightarrow Tx$, $Rx \rightarrow Rx$). The electrical circuit remains open, and the physical link stays DOWN.
- **Copper Cross-Over Cable:** Pins are swapped according to T568A on one end and T568B on the opposite end ($1 \leftrightarrow 3$, $2 \leftrightarrow 6$). Transmitters align with receivers, establishing a valid physical link.

### 2.2 Collision Domains vs. Broadcast Domains
- **Layer 1 Hub (Repeater):** An unmanaged electrical repeater. Frames arriving on any port are unconditionally echoed out of all other ports. All connected hosts share a **single collision domain** governed by CSMA/CD, and bandwidth is partitioned.
- **Layer 2 Switch (Learning Bridge):** Inspects incoming 48-bit hardware MAC addresses. Populates its internal Content Addressable Memory (CAM) table via **backward learning**. Unicast traffic is selectively forwarded to the destination port only (microsegmentation). Each port constitutes an isolated, full-duplex collision domain.
- **Layer 3 Router:** Terminates broadcast domains. Forwards packets between logical subnets using IP routing tables, executing Longest Prefix Match (LPM) and decrementing the IP Time-To-Live (TTL).

---

## 3. Detailed Problem Specifications & Solutions

### Problem 1: Back-to-Back Host Interconnection via Crossover Cable
- **Setup:** Host-1 (`192.168.1.1/24`, MAC `0001.64A1.0001`) connected to Host-2 (`192.168.1.2/24`, MAC `0001.64D4.0002`) via a Copper Cross-Over Cable.
- **Verification:** Both hosts execute bilateral pings (`ping 192.168.1.2` and `ping 192.168.1.1`).
- **ARP Evolution:** Host ARP caches transition from empty (cold state) to populated with dynamic peer mappings (`192.168.1.2 -> 0001.64d4.0002`).

### Problem 2: Star Topology LAN-A (3 Hosts using Layer-1 Hub)
- **Setup:** Hub-A with 3 hosts on subnet `192.168.10.0/24`:
  - Host-A1 (`192.168.10.1`), Host-A2 (`192.168.10.2`), Host-A3 (`192.168.10.3`).
- **Verification:** Full pairwise pings (A1 $\leftrightarrow$ A2 $\leftrightarrow$ A3).
- **Traffic Analysis:** Hub unconditionally repeats frames to all ports. Non-target NICs receive the frame and discard it at Layer 2 based on destination MAC mismatch.

### Problem 3: Star Topology LAN-B (3 Hosts via Cisco 2950 Switch)
- **Setup:** Cisco Catalyst 2950 switch (`Switch-B`) with 3 hosts on subnet `192.168.20.0/24`:
  - Host-B1 (`192.168.20.1`, Fa0/1), Host-B2 (`192.168.20.2`, Fa0/2), Host-B3 (`192.168.20.3`, Fa0/3).
- **Recorded Initial State:**
  - Host ARP caches: *No ARP Entries Found.*
  - Switch-B MAC Table: *Total Mac Addresses for this module: 0.*
- **Recorded Post-Ping State:**
  - Host ARP caches contain dynamic entries for all communicating peers.
  - Switch-B MAC Table records:
    * `Fa0/1 -> 0060.2F11.0001` (Host-B1)
    * `Fa0/2 -> 0060.2F22.0002` (Host-B2)
    * `Fa0/3 -> 0060.2F33.0003` (Host-B3)

### Problem 4: Hybrid Interconnection of LAN-A (Hub) and LAN-B (Switch)
- **Setup:** Hub-A (FastEthernet3) connected to Switch-B (FastEthernet0/4) via a Copper Cross-Over Cable under unified subnet `192.168.1.0/24`.
- **Key Pedagogical Discovery:**
  Switch-B's port **`FastEthernet0/4`** dynamically learns **MULTIPLE MAC addresses** (`Host-A1`, `Host-A2`, and `Host-A3`) because all frames originating behind the hub traverse that single uplink interface.

### Problem 5: Campus Dual-LAN Routing (JU-Main & JU-SL) via Cisco Routers
- **Setup:**
  - **JU-Main Campus LAN (`192.168.148.0/24`):** 3 hosts (PC1-3: `.2`, `.3`, `.4`), switch `PC-LAB1-Switch`, default gateway `192.168.148.1` on router `Router-JU-Main` (`Fa0/0`).
  - **JU-SL Salt Lake LAN (`192.168.149.0/24`):** 3 hosts (PC4-6: `.2`, `.3`, `.4`), switch `PC-LAB2-Switch`, default gateway `192.168.149.1` on router `Router-JU-SL` (`Fa0/0`).
  - **WAN Interconnect (`192.168.150.0/24`):** Router-JU-Main `Fa0/1` (`192.168.150.1`) $\longleftrightarrow$ Router-JU-SL `Fa0/1` (`192.168.150.2`).
- **Static Route Configurations:**
  - `Router-JU-Main`: `ip route 192.168.149.0 255.255.255.0 192.168.150.2`
  - `Router-JU-SL`: `ip route 192.168.148.0 255.255.255.0 192.168.150.1`
- **Verification:** Cross-campus pings succeed with TTL decrement (128 $\rightarrow$ 126).

### Problem 6: Dynamic Host Configuration Protocol (DHCP) Deployment
- **Setup:** Dedicated DHCP Servers deployed on JU-Main (`JU-DHCP-Main: 192.168.148.10`) and JU-SL (`JU-DHCP-SL: 192.168.149.10`).
- **DORA Execution:** Workstations dynamically acquire IP, subnet mask, default gateway, and DNS server via 4-way DORA handshake (`DISCOVER -> OFFER -> REQUEST -> ACK`).
- **Verification:** Active lease pool tables verified; cross-campus ping between dynamic clients succeeds.

### Problem 7: Integrated Departmental CSE LAN with DHCP, DNS, Web, and FTP
- **Setup:** Departmental switch `CSE-Switch` with domain `cse.myuniv.edu`:
  - Three dynamic workstations (`PC1`, `PC2`, `PC3`).
  - **DHCP Server (`192.168.100.2`):** Pool `192.168.100.50 - .100`, Gateway `100.1`, DNS `100.4`.
  - **DNS Server (`192.168.100.4`):** A-records for `cse.myuniv.edu` and `web.cse.myuniv.edu` $\rightarrow$ `192.168.100.10`, `ftp.cse.myuniv.edu` $\rightarrow$ `192.168.100.20`, `dns.cse.myuniv.edu` $\rightarrow$ `192.168.100.4`.
  - **Web Server (`192.168.100.10`):** HTTP daemon serving custom HTML portal.
  - **FTP Server (`192.168.100.20`):** FTP service with user `csejadavpur`, password `networks123`.
- **Verification:**
  1. PC1 acquires dynamic IP `192.168.100.50` via DHCP.
  2. PC1 executes `nslookup cse.myuniv.edu` $\rightarrow$ resolves `192.168.100.10`.
  3. PC1 accesses `http://cse.myuniv.edu/` $\rightarrow$ fetches `200 OK` HTML portal.
  4. PC1 connects to `ftp ftp.cse.myuniv.edu` $\rightarrow$ authenticates, lists directory, and downloads `welcome.txt`.

---

## 4. Directory Structure

```
CN_Lab_Ass6/
├── Assignment#6-CISCO_PacketTracerNetwork.pdf   # Original Assignment Specification
├── Jadavpur_University_Logo.svg.png            # Institutional Emblem for Report
├── Makefile                                    # Automated C++ Build Engine
├── run_all_tests.sh                            # Test Pipeline & Verification Suite
├── generate_plots.py                           # Matplotlib Visualization Generator
├── report.tex                                  # Comprehensive 26-Page LaTeX Report
├── report.pdf                                  # Compiled Academic Report
├── CN_Ass6_Report.pdf                          # Formatted Assignment Submission Report
├── README.md                                   # Project Documentation & Architecture
│
├── cisco_configs/                              # Cisco IOS CLI Scripts & Guides
│   ├── problem1_crossover.txt                  # Problem 1 Host & Cable Instructions
│   ├── problem2_lan_a_hub.txt                  # Problem 2 Hub & Host Instructions
│   ├── problem3_lan_b_switch.ios               # Problem 3 Cisco 2950 Switch IOS Config
│   ├── problem4_interconnected.ios             # Problem 4 Bridged Uplink IOS Config
│   ├── problem5_switches.ios                   # Problem 5 Campus Switches IOS Config
│   ├── problem5_ju_main_router.ios             # Problem 5 JU-Main Router IOS Config
│   ├── problem5_ju_sl_router.ios               # Problem 5 JU-SL Router IOS Config
│   ├── problem5_dual_lan_routers.txt           # Problem 5 Campus Routing Guide
│   ├── problem6_dhcp_services.ios              # Problem 6 DHCP Server & Pool Config
│   └── problem7_cse_enterprise_network.ios     # Problem 7 Complete CSE Services Config
│
├── simulator/                                  # C++17 Discrete-Event Network Simulator
│   ├── types.hpp                               # Protocols, Frame & Packet Structures
│   ├── arp_table.hpp / .cpp                    # Dynamic Host & Router ARP Cache
│   ├── mac_table.hpp / .cpp                    # Switch CAM Forwarding Table
│   ├── routing_table.hpp / .cpp                # Router LPM Routing Table
│   ├── network.hpp / .cpp                      # Physical Port & Link Layer Delivery Bus
│   ├── hub.hpp / .cpp                          # Layer-1 Hub Multiport Repeater
│   ├── switch.hpp / .cpp                       # Layer-2 Cisco 2950 Learning Switch
│   ├── router.hpp / .cpp                       # Layer-3 Cisco 1818 Router
│   ├── services.hpp / .cpp                     # DHCP, DNS, Web, and FTP Servers
│   ├── host.hpp / .cpp                         # Workstation Node & Application Clients
│   ├── problems.hpp / .cpp                     # Problem 1-7 Simulation Implementations
│   └── main.cpp                                # Command-Line Interface Runner
│
├── plots/                                      # Publication-Quality PNG Diagrams
│   ├── topology_prob1_crossover.png
│   ├── topology_prob2_hub.png
│   ├── topology_prob3_switch.png
│   ├── topology_prob4_interconnected.png
│   ├── topology_prob5_routers.png
│   ├── dhcp_dora_sequence.png
│   ├── topology_prob7_integrated_services.png
│   └── latency_comparison.png
│
└── logs/                                       # Simulation Verification Traces
    ├── problem_1.log
    ├── problem_2.log
    ├── problem_3.log
    ├── problem_4.log
    ├── problem_5.log
    ├── problem_6.log
    ├── problem_7.log
    └── full_simulation.log
```

---

## 5. Build, Test, and Execution Instructions

### 5.1 Compiling the C++ Simulation Engine
```bash
make clean
make all
```

### 5.2 Running the Simulation Suite
```bash
# Run all 7 problems sequentially
./sim_network all

# Run individual problems
./sim_network 1    # Problem 1 (Crossover)
./sim_network 3    # Problem 3 (Switch ARP/MAC)
./sim_network 5    # Problem 5 (Dual-LAN Routers)
./sim_network 7    # Problem 7 (Integrated CSE Services)

# Enable detailed packet tracing
./sim_network all --verbose
```

### 5.3 Generating Publication Visualizations
```bash
python3 generate_plots.py
```

### 5.4 Automated End-to-End Test Suite & Report Generation
```bash
./run_all_tests.sh
```

---

## 6. Verification and Empirical Results Summary

| Experiment | Focus Area | Observed Behavior | Verification Status |
|:---|:---|:---|:---:|
| **Problem 1** | Back-to-Back Crossover | Direct DTE-DTE link UP; cold ARP $\rightarrow$ ICMP ping 0% loss | **PASS (100%)** |
| **Problem 2** | LAN-A (Hub) | Layer-1 repetition; shared collision domain; NIC filtering | **PASS (100%)** |
| **Problem 3** | LAN-B (Switch) | Cold ARP/MAC empty $\rightarrow$ Backward learning populates CAM table | **PASS (100%)** |
| **Problem 4** | Hub + Switch Bridge | Port Fa0/4 learns multiple MAC addresses behind hub | **PASS (100%)** |
| **Problem 5** | Campus Dual-LAN Routing | Static routes route packets across WAN; TTL decrements 128 $\rightarrow$ 126 | **PASS (100%)** |
| **Problem 6** | DHCP Deployment | 4-way DORA handshake assigns dynamic leases; cross-LAN ping OK | **PASS (100%)** |
| **Problem 7** | Enterprise CSE LAN | Dynamic IP $\rightarrow$ DNS lookup (`cse.myuniv.edu`) $\rightarrow$ HTTP GET $\rightarrow$ FTP get | **PASS (100%)** |

All deliverables adhere strictly to Jadavpur University Computer Networks Laboratory guidelines.

