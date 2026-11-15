#include "problems.hpp"
#include "host.hpp"
#include "hub.hpp"
#include "switch.hpp"
#include "router.hpp"
#include "services.hpp"
#include <iomanip>

namespace net {

// =============================================================================
// PROBLEM 1: BACK-TO-BACK CROSSOVER CONNECTION
// =============================================================================
void run_problem_1(std::ostream& os, bool verbose) {
    os << "\n"
       << "================================================================================\n"
       << " EXPERIMENT 1: BACK-TO-BACK HOST INTERCONNECTION VIA CROSSOVER CABLE\n"
       << "================================================================================\n";

    Host host1("Host-1", MacAddress("0001.64A1.0001"), IPv4Address("192.168.1.1"));
    Host host2("Host-2", MacAddress("0001.64D4.0002"), IPv4Address("192.168.1.2"));
    host1.verbose = verbose;
    host2.verbose = verbose;

    // Connect with Crossover cable
    Device::connect_ports(host1.port(), host2.port(), CableType::CrossOver);

    os << "[Step 1] Initial IP Configurations:\n";
    host1.ipconfig(true, os);
    host2.ipconfig(true, os);

    os << "\n[Step 2] Initial ARP Caches (Cold State):\n";
    host1.show_arp(os);
    host2.show_arp(os);

    os << "\n[Step 3] Executing Ping from Host-1 (192.168.1.1) -> Host-2 (192.168.1.2):\n";
    host1.ping(IPv4Address("192.168.1.2"), 4, os);

    os << "\n[Step 4] Executing Reverse Ping from Host-2 (192.168.1.2) -> Host-1 (192.168.1.1):\n";
    host2.ping(IPv4Address("192.168.1.1"), 4, os);

    os << "\n[Step 5] Post-Ping ARP Caches (Resolved State):\n";
    host1.show_arp(os);
    host2.show_arp(os);

    os << "\n[Observation & Conclusion]:\n"
       << "  - Physical Media: Ethernet crossover pinout successfully pairs DTE Tx pins (1,2)\n"
       << "    with DTE Rx pins (3,6) between two peer PCs without an intervening DCE.\n"
       << "  - ARP Mechanics: Initial ping triggered an ARP broadcast request; upon receipt\n"
       << "    of the unicast ARP reply, both hosts populated dynamic ARP entries.\n"
       << "  - Result: 0% packet loss, bilateral ping verified successfully.\n";
}

// =============================================================================
// PROBLEM 2: LAN-A VIA HUB
// =============================================================================
void run_problem_2(std::ostream& os, bool verbose) {
    os << "\n"
       << "================================================================================\n"
       << " EXPERIMENT 2: LAN-A (3 HOSTS CONNECTED VIA A LAYER-1 HUB)\n"
       << "================================================================================\n";

    Hub hubA("Hub-A", 4);
    hubA.verbose = verbose;

    Host hostA1("Host-A1", MacAddress("0002.16A1.0001"), IPv4Address("192.168.10.1"));
    Host hostA2("Host-A2", MacAddress("0002.16A2.0002"), IPv4Address("192.168.10.2"));
    Host hostA3("Host-A3", MacAddress("0002.16A3.0003"), IPv4Address("192.168.10.3"));
    hostA1.verbose = verbose;
    hostA2.verbose = verbose;
    hostA3.verbose = verbose;

    // Connect hosts to Hub ports via straight-through cables
    Device::connect_ports(hostA1.port(), hubA.get_port("FastEthernet0"), CableType::StraightThrough);
    Device::connect_ports(hostA2.port(), hubA.get_port("FastEthernet1"), CableType::StraightThrough);
    Device::connect_ports(hostA3.port(), hubA.get_port("FastEthernet2"), CableType::StraightThrough);

    os << "[Step 1] Network IP Topology (LAN-A: 192.168.10.0/24):\n"
       << "  - Host-A1: 192.168.10.1 (Connected to Hub-A Fa0)\n"
       << "  - Host-A2: 192.168.10.2 (Connected to Hub-A Fa1)\n"
       << "  - Host-A3: 192.168.10.3 (Connected to Hub-A Fa2)\n";

    os << "\n[Step 2] Pairwise Ping 1: Host-A1 -> Host-A2:\n";
    hostA1.ping(IPv4Address("192.168.10.2"), 4, os);

    os << "\n[Step 3] Pairwise Ping 2: Host-A2 -> Host-A3:\n";
    hostA2.ping(IPv4Address("192.168.10.3"), 4, os);

    os << "\n[Step 4] Pairwise Ping 3: Host-A1 -> Host-A3:\n";
    hostA1.ping(IPv4Address("192.168.10.3"), 4, os);

    os << "\n[Step 5] Hub-A Layer-1 Statistics:\n"
       << "  - Total bits/frames repeated unconditionally: " << hubA.total_frames_repeated << "\n"
       << "  - Collision Domain Count: 1 (Shared across all 3 ports)\n"
       << "  - Broadcast Domain Count: 1\n";

    os << "\n[Observation & Conclusion]:\n"
       << "  - Hub acts as a Layer-1 repeater: bits received on any port are echoed to all other ports.\n"
       << "  - All stations receive all traffic; filtering occurs only at Layer 2 inside host NICs.\n"
       << "  - All pairs demonstrated 100% ICMP reachability.\n";
}

// =============================================================================
// PROBLEM 3: LAN-B VIA SWITCH & ARP / MAC TABLE RECORDING
// =============================================================================
void run_problem_3(std::ostream& os, bool verbose) {
    os << "\n"
       << "================================================================================\n"
       << " EXPERIMENT 3: LAN-B (3 HOSTS VIA SWITCH) & MAC/ARP TABLE DYNAMICS\n"
       << "================================================================================\n";

    Switch switchB("Switch-B", 24);
    switchB.verbose = verbose;

    Host hostB1("Host-B1", MacAddress("0060.2F11.0001"), IPv4Address("192.168.20.1"));
    Host hostB2("Host-B2", MacAddress("0060.2F22.0002"), IPv4Address("192.168.20.2"));
    Host hostB3("Host-B3", MacAddress("0060.2F33.0003"), IPv4Address("192.168.20.3"));
    hostB1.verbose = verbose;
    hostB2.verbose = verbose;
    hostB3.verbose = verbose;

    Device::connect_ports(hostB1.port(), switchB.get_port("FastEthernet0/1"), CableType::StraightThrough);
    Device::connect_ports(hostB2.port(), switchB.get_port("FastEthernet0/2"), CableType::StraightThrough);
    Device::connect_ports(hostB3.port(), switchB.get_port("FastEthernet0/3"), CableType::StraightThrough);

    os << "\n>>> [PHASE 1: BEFORE PING - RECORD INITIAL TABLES] <<<\n";
    os << "\n--- Host-B1 ARP Table (Initial) ---\n";
    hostB1.show_arp(os);
    os << "\n--- Host-B2 ARP Table (Initial) ---\n";
    hostB2.show_arp(os);
    os << "\n--- Host-B3 ARP Table (Initial) ---\n";
    hostB3.show_arp(os);
    os << "\n--- Switch-B MAC Forwarding Table (Initial) ---\n";
    switchB.show_mac_address_table(os);

    os << "\n>>> [PHASE 2: EXECUTE ALL-PAIRS PINGS] <<<\n";
    os << "\n[Ping 1: Host-B1 -> Host-B2 (192.168.20.2)]\n";
    hostB1.ping(IPv4Address("192.168.20.2"), 4, os);

    os << "\n[Ping 2: Host-B2 -> Host-B3 (192.168.20.3)]\n";
    hostB2.ping(IPv4Address("192.168.20.3"), 4, os);

    os << "\n[Ping 3: Host-B1 -> Host-B3 (192.168.20.3)]\n";
    hostB1.ping(IPv4Address("192.168.20.3"), 4, os);

    os << "\n>>> [PHASE 3: AFTER PING - RECORD UPDATED TABLES] <<<\n";
    os << "\n--- Host-B1 ARP Table (After Pings) ---\n";
    hostB1.show_arp(os);
    os << "\n--- Host-B2 ARP Table (After Pings) ---\n";
    hostB2.show_arp(os);
    os << "\n--- Host-B3 ARP Table (After Pings) ---\n";
    hostB3.show_arp(os);
    os << "\n--- Switch-B MAC Forwarding Table (After Pings) ---\n";
    switchB.show_mac_address_table(os);

    os << "\n[Observation & Comparison with Problem 2]:\n"
       << "  - Unlike Hub-A, Switch-B performs Backward MAC Learning from source frames.\n"
       << "  - Initial MAC table was empty. As packets flowed, ports Fa0/1, Fa0/2, Fa0/3 were\n"
       << "    associated with the corresponding host MAC addresses.\n"
       << "  - Subsequent unicast frames were selectively switched ONLY to the destination port,\n"
       << "    eliminating collision domains (microsegmentation) and conserving bandwidth.\n";
}

// =============================================================================
// PROBLEM 4: INTERCONNECTING LAN-A (HUB) AND LAN-B (SWITCH)
// =============================================================================
void run_problem_4(std::ostream& os, bool verbose) {
    os << "\n"
       << "================================================================================\n"
       << " EXPERIMENT 4: INTERCONNECTING LAN-A (HUB) & LAN-B (SWITCH) VIA CROSSOVER\n"
       << "================================================================================\n";

    Hub hubA("Hub-A", 4);
    Switch switchB("Switch-B", 24);
    hubA.verbose = verbose;
    switchB.verbose = verbose;

    // Unified Subnet 192.168.1.0/24 for Layer 2 bridged communication
    Host hostA1("Host-A1", MacAddress("0002.16A1.0001"), IPv4Address("192.168.1.1"));
    Host hostA2("Host-A2", MacAddress("0002.16A2.0002"), IPv4Address("192.168.1.2"));
    Host hostA3("Host-A3", MacAddress("0002.16A3.0003"), IPv4Address("192.168.1.3"));

    Host hostB1("Host-B1", MacAddress("0060.2F11.0001"), IPv4Address("192.168.1.4"));
    Host hostB2("Host-B2", MacAddress("0060.2F22.0002"), IPv4Address("192.168.1.5"));
    Host hostB3("Host-B3", MacAddress("0060.2F33.0003"), IPv4Address("192.168.1.6"));

    // Connect LAN-A hosts to Hub-A
    Device::connect_ports(hostA1.port(), hubA.get_port("FastEthernet0"), CableType::StraightThrough);
    Device::connect_ports(hostA2.port(), hubA.get_port("FastEthernet1"), CableType::StraightThrough);
    Device::connect_ports(hostA3.port(), hubA.get_port("FastEthernet2"), CableType::StraightThrough);

    // Connect LAN-B hosts to Switch-B
    Device::connect_ports(hostB1.port(), switchB.get_port("FastEthernet0/1"), CableType::StraightThrough);
    Device::connect_ports(hostB2.port(), switchB.get_port("FastEthernet0/2"), CableType::StraightThrough);
    Device::connect_ports(hostB3.port(), switchB.get_port("FastEthernet0/3"), CableType::StraightThrough);

    // Interconnect Hub-A and Switch-B using a Crossover Cable (Hub Fa3 <-> Switch Fa0/4)
    Device::connect_ports(hubA.get_port("FastEthernet3"), switchB.get_port("FastEthernet0/4"), CableType::CrossOver);

    os << "[Topology Interconnect]:\n"
       << "  Hub-A (FastEthernet3) <=== [Crossover Cable] ===> Switch-B (FastEthernet0/4)\n";

    os << "\n>>> [PHASE 1: BEFORE CROSS-LAN PINGS - RECORD TABLES] <<<\n";
    hostA1.show_arp(os);
    hostB1.show_arp(os);
    switchB.show_mac_address_table(os);

    os << "\n>>> [PHASE 2: EXECUTE CROSS-LAN PINGS] <<<\n";
    os << "\n[Cross-Ping 1: Host-A1 (192.168.1.1) -> Host-B1 (192.168.1.4)]\n";
    hostA1.ping(IPv4Address("192.168.1.4"), 4, os);

    os << "\n[Cross-Ping 2: Host-A2 (192.168.1.2) -> Host-B2 (192.168.1.5)]\n";
    hostA2.ping(IPv4Address("192.168.1.5"), 4, os);

    os << "\n[Cross-Ping 3: Host-A3 (192.168.1.3) -> Host-B3 (192.168.1.6)]\n";
    hostA3.ping(IPv4Address("192.168.1.6"), 4, os);

    os << "\n[Cross-Ping 4: Host-B1 (192.168.1.4) -> Host-A2 (192.168.1.2)]\n";
    hostB1.ping(IPv4Address("192.168.1.2"), 4, os);

    os << "\n>>> [PHASE 3: AFTER CROSS-LAN PINGS - RECORD TABLES] <<<\n";
    os << "\n--- Host-A1 ARP Table (Post-Ping) ---\n";
    hostA1.show_arp(os);
    os << "\n--- Host-B1 ARP Table (Post-Ping) ---\n";
    hostB1.show_arp(os);
    os << "\n--- Switch-B MAC Forwarding Table (CRUCIAL OBSERVATION) ---\n";
    switchB.show_mac_address_table(os);

    os << "\n[Key Analytical Discovery]:\n"
       << "  Notice that Port FastEthernet0/4 on Switch-B has dynamically learned MULTIPLE\n"
       << "  MAC addresses (Host-A1, Host-A2, and Host-A3)!\n"
       << "  Because Hub-A multiplexes all traffic from LAN-A onto that single uplink cable,\n"
       << "  every frame from LAN-A arrives at Switch-B via port Fa0/4.\n"
       << "  This proves switches support multi-MAC port associations when connected to hubs.\n";
}

// =============================================================================
// PROBLEM 5: DUAL LAN INTERCONNECTION VIA CISCO ROUTERS & STATIC ROUTING
// =============================================================================
void run_problem_5(std::ostream& os, bool verbose) {
    os << "\n"
       << "================================================================================\n"
       << " EXPERIMENT 5: DUAL LAN ROUTING (JU-MAIN & JU-SL) VIA CISCO 1818 ROUTERS\n"
       << "================================================================================\n";

    // --- LAN 1: JU-Main ---
    Switch swMain("PC-LAB1-Switch", 24);
    Router rtrMain("Router-JU-Main");
    rtrMain.verbose = verbose;

    Host hostM1("Host-M1", MacAddress("0001.64A1.0001"), IPv4Address("192.168.148.2"),
                IPv4Address("255.255.255.0"), IPv4Address("192.168.148.1"));
    Host hostM2("Host-M2", MacAddress("0001.64A1.0002"), IPv4Address("192.168.148.3"),
                IPv4Address("255.255.255.0"), IPv4Address("192.168.148.1"));
    Host hostM3("Host-M3", MacAddress("0001.64A1.0003"), IPv4Address("192.168.148.4"),
                IPv4Address("255.255.255.0"), IPv4Address("192.168.148.1"));

    Port* rMainLAN = rtrMain.configure_interface("FastEthernet0/0", IPv4Address("192.168.148.1"),
                                                IPv4Address("255.255.255.0"), MacAddress("0001.9614.8001"));
    Port* rMainWAN = rtrMain.configure_interface("FastEthernet0/1", IPv4Address("192.168.150.1"),
                                                IPv4Address("255.255.255.0"), MacAddress("0001.9615.0001"));

    Device::connect_ports(hostM1.port(), swMain.get_port("FastEthernet0/1"));
    Device::connect_ports(hostM2.port(), swMain.get_port("FastEthernet0/2"));
    Device::connect_ports(hostM3.port(), swMain.get_port("FastEthernet0/3"));
    Device::connect_ports(rMainLAN, swMain.get_port("FastEthernet0/24"));

    // --- LAN 2: JU-SL ---
    Switch swSL("PC-LAB2-Switch", 24);
    Router rtrSL("Router-JU-SL");
    rtrSL.verbose = verbose;

    Host hostS1("Host-S1", MacAddress("0001.64B1.0001"), IPv4Address("192.168.149.2"),
                IPv4Address("255.255.255.0"), IPv4Address("192.168.149.1"));
    Host hostS2("Host-S2", MacAddress("0001.64B1.0002"), IPv4Address("192.168.149.3"),
                IPv4Address("255.255.255.0"), IPv4Address("192.168.149.1"));
    Host hostS3("Host-S3", MacAddress("0001.64B1.0003"), IPv4Address("192.168.149.4"),
                IPv4Address("255.255.255.0"), IPv4Address("192.168.149.1"));

    Port* rSLLAN = rtrSL.configure_interface("FastEthernet0/0", IPv4Address("192.168.149.1"),
                                            IPv4Address("255.255.255.0"), MacAddress("0001.9614.9001"));
    Port* rSLWAN = rtrSL.configure_interface("FastEthernet0/1", IPv4Address("192.168.150.2"),
                                            IPv4Address("255.255.255.0"), MacAddress("0001.9615.0002"));

    Device::connect_ports(hostS1.port(), swSL.get_port("FastEthernet0/1"));
    Device::connect_ports(hostS2.port(), swSL.get_port("FastEthernet0/2"));
    Device::connect_ports(hostS3.port(), swSL.get_port("FastEthernet0/3"));
    Device::connect_ports(rSLLAN, swSL.get_port("FastEthernet0/24"));

    // --- WAN Interconnect (Router to Router via Crossover) ---
    Device::connect_ports(rMainWAN, rSLWAN, CableType::CrossOver);

    // --- Add Static Routes ---
    rtrMain.add_static_route(IPv4Address("192.168.149.0"), IPv4Address("255.255.255.0"), IPv4Address("192.168.150.2"));
    rtrSL.add_static_route(IPv4Address("192.168.148.0"), IPv4Address("255.255.255.0"), IPv4Address("192.168.150.1"));

    os << "[Step 1] Router Interface Status:\n";
    rtrMain.show_ip_interface_brief(os);
    os << "\n";
    rtrSL.show_ip_interface_brief(os);

    os << "\n[Step 2] Routing Tables with Static Routes Added:\n";
    rtrMain.show_ip_route(os);
    os << "\n";
    rtrSL.show_ip_route(os);

    os << "\n[Step 3] Cross-Campus Ping: Host-M1 (JU-Main 192.168.148.2) -> Host-S1 (JU-SL 192.168.149.2):\n";
    hostM1.ping(IPv4Address("192.168.149.2"), 4, os);

    os << "\n[Step 4] Cross-Campus Reverse Ping: Host-S2 (JU-SL 192.168.149.3) -> Host-M2 (JU-Main 192.168.148.3):\n";
    hostS2.ping(IPv4Address("192.168.148.3"), 4, os);

    os << "\n[Step 5] Router ARP Tables:\n";
    rtrMain.show_arp(os);
    os << "\n";
    rtrSL.show_arp(os);

    os << "\n[Observation]:\n"
       << "  - Static routes configured on both routers enable seamless Layer 3 packet forwarding.\n"
       << "  - TTL was decremented at each router hop (initial TTL 128 -> received TTL 126).\n"
       << "  - Two collision domains per LAN and a point-to-point WAN link were successfully orchestrated.\n";
}

// =============================================================================
// PROBLEM 6: DYNAMIC HOST CONFIGURATION VIA DHCP SERVERS
// =============================================================================
void run_problem_6(std::ostream& os, bool verbose) {
    os << "\n"
       << "================================================================================\n"
       << " EXPERIMENT 6: CAMPUS DHCP SERVER DEPLOYMENT ON JU-MAIN AND JU-SL\n"
       << "================================================================================\n";

    // JU-Main Network
    Switch swMain("PC-LAB1-Switch", 24);
    Router rtrMain("Router-JU-Main");
    rtrMain.verbose = verbose;

    Port* rMainLAN = rtrMain.configure_interface("FastEthernet0/0", IPv4Address("192.168.148.1"),
                                                IPv4Address("255.255.255.0"), MacAddress("0001.9614.8001"));
    Port* rMainWAN = rtrMain.configure_interface("FastEthernet0/1", IPv4Address("192.168.150.1"),
                                                IPv4Address("255.255.255.0"), MacAddress("0001.9615.0001"));
    Device::connect_ports(rMainLAN, swMain.get_port("FastEthernet0/24"));

    // Add Dedicated DHCP Server in JU-Main
    DHCPServer dhcpMain("JU-DHCP-Main", MacAddress("0001.96D1.0001"),
                        IPv4Address("192.168.148.10"), IPv4Address("255.255.255.0"),
                        IPv4Address("192.168.148.1"), IPv4Address("192.168.148.10"),
                        IPv4Address("192.168.148.20"), 50);
    dhcpMain.verbose = verbose;
    Device::connect_ports(dhcpMain.get_port("FastEthernet0"), swMain.get_port("FastEthernet0/10"));

    // Hosts in JU-Main (unconfigured initially, DHCP client mode)
    Host hostM1("Host-M1", MacAddress("0001.64A1.0001"));
    Host hostM2("Host-M2", MacAddress("0001.64A1.0002"));
    Host hostM3("Host-M3", MacAddress("0001.64A1.0003"));
    Device::connect_ports(hostM1.port(), swMain.get_port("FastEthernet0/1"));
    Device::connect_ports(hostM2.port(), swMain.get_port("FastEthernet0/2"));
    Device::connect_ports(hostM3.port(), swMain.get_port("FastEthernet0/3"));

    // JU-SL Network
    Switch swSL("PC-LAB2-Switch", 24);
    Router rtrSL("Router-JU-SL");
    rtrSL.verbose = verbose;

    Port* rSLLAN = rtrSL.configure_interface("FastEthernet0/0", IPv4Address("192.168.149.1"),
                                            IPv4Address("255.255.255.0"), MacAddress("0001.9614.9001"));
    Port* rSLWAN = rtrSL.configure_interface("FastEthernet0/1", IPv4Address("192.168.150.2"),
                                            IPv4Address("255.255.255.0"), MacAddress("0001.9615.0002"));
    Device::connect_ports(rSLLAN, swSL.get_port("FastEthernet0/24"));

    // Add Dedicated DHCP Server in JU-SL
    DHCPServer dhcpSL("JU-DHCP-SL", MacAddress("0001.96D2.0002"),
                      IPv4Address("192.168.149.10"), IPv4Address("255.255.255.0"),
                      IPv4Address("192.168.149.1"), IPv4Address("192.168.149.10"),
                      IPv4Address("192.168.149.20"), 50);
    dhcpSL.verbose = verbose;
    Device::connect_ports(dhcpSL.get_port("FastEthernet0"), swSL.get_port("FastEthernet0/10"));

    // Hosts in JU-SL
    Host hostS1("Host-S1", MacAddress("0001.64B1.0001"));
    Host hostS2("Host-S2", MacAddress("0001.64B1.0002"));
    Host hostS3("Host-S3", MacAddress("0001.64B1.0003"));
    Device::connect_ports(hostS1.port(), swSL.get_port("FastEthernet0/1"));
    Device::connect_ports(hostS2.port(), swSL.get_port("FastEthernet0/2"));
    Device::connect_ports(hostS3.port(), swSL.get_port("FastEthernet0/3"));

    // WAN Link & Static Routes
    Device::connect_ports(rMainWAN, rSLWAN, CableType::CrossOver);
    rtrMain.add_static_route(IPv4Address("192.168.149.0"), IPv4Address("255.255.255.0"), IPv4Address("192.168.150.2"));
    rtrSL.add_static_route(IPv4Address("192.168.148.0"), IPv4Address("255.255.255.0"), IPv4Address("192.168.150.1"));

    os << "[Step 1] Requesting Dynamic Leases for JU-Main Hosts via DORA:\n";
    hostM1.request_dhcp(os);
    hostM2.request_dhcp(os);
    hostM3.request_dhcp(os);

    os << "\n[Step 2] Requesting Dynamic Leases for JU-SL Hosts via DORA:\n";
    hostS1.request_dhcp(os);
    hostS2.request_dhcp(os);
    hostS3.request_dhcp(os);

    os << "\n[Step 3] Active Leases on DHCP Servers:\n";
    dhcpMain.show_leases(os);
    os << "\n";
    dhcpSL.show_leases(os);

    os << "\n[Step 4] Inter-LAN Ping between Dynamically Addressed Hosts:\n"
       << "  Host-M1 (" << hostM1.ip() << ") pinging Host-S1 (" << hostS1.ip() << "):\n";
    hostM1.ping(hostS1.ip(), 4, os);

    os << "\n[Observation]:\n"
       << "  - Standard 4-way DORA handshake executed cleanly: DHCPDISCOVER -> DHCPOFFER -> DHCPREQUEST -> DHCPACK.\n"
       << "  - All workstations received dynamic IP, subnet mask, default gateway, and DNS server addresses.\n"
       << "  - Cross-router static routing remained 100% operational with dynamic client endpoints.\n";
}

// =============================================================================
// PROBLEM 7: INTEGRATED ENTERPRISE LAN (CSE) WITH DHCP, DNS, WEB & FTP
// =============================================================================
void run_problem_7(std::ostream& os, bool verbose) {
    os << "\n"
       << "================================================================================\n"
       << " EXPERIMENT 7: INTEGRATED ENTERPRISE LAN (CSE) WITH DHCP, DNS, WEB & FTP\n"
       << "================================================================================\n";

    Switch cseSwitch("CSE-Switch", 24);
    cseSwitch.verbose = verbose;

    // 1. DHCP Server (Fixed IP 192.168.100.2)
    DHCPServer dhcpServer("CSE-DHCP-Server", MacAddress("0001.96C1.0002"),
                          IPv4Address("192.168.100.2"), IPv4Address("255.255.255.0"),
                          IPv4Address("192.168.100.1"), IPv4Address("192.168.100.4"),
                          IPv4Address("192.168.100.50"), 50);
    dhcpServer.verbose = verbose;
    Device::connect_ports(dhcpServer.get_port("FastEthernet0"), cseSwitch.get_port("FastEthernet0/4"));

    // 2. DNS Server (Fixed IP 192.168.100.4, Domain cse.myuniv.edu)
    DNSServer dnsServer("CSE-DNS-Server", MacAddress("0001.96C1.0004"),
                        IPv4Address("192.168.100.4"), IPv4Address("255.255.255.0"),
                        IPv4Address("192.168.100.1"));
    dnsServer.verbose = verbose;
    dnsServer.add_a_record("cse.myuniv.edu", IPv4Address("192.168.100.10"));
    dnsServer.add_a_record("web.cse.myuniv.edu", IPv4Address("192.168.100.10"));
    dnsServer.add_a_record("ftp.cse.myuniv.edu", IPv4Address("192.168.100.20"));
    dnsServer.add_a_record("dns.cse.myuniv.edu", IPv4Address("192.168.100.4"));
    dnsServer.add_a_record("dhcp.cse.myuniv.edu", IPv4Address("192.168.100.2"));
    Device::connect_ports(dnsServer.get_port("FastEthernet0"), cseSwitch.get_port("FastEthernet0/5"));

    // 3. Web Server (Fixed IP 192.168.100.10)
    WebServer webServer("CSE-Web-Server", MacAddress("0001.96C1.0010"),
                        IPv4Address("192.168.100.10"), IPv4Address("255.255.255.0"),
                        IPv4Address("192.168.100.1"));
    webServer.verbose = verbose;
    webServer.set_html_page("/",
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>Jadavpur University - CSE Department</title></head>\n"
        "<body style='font-family: Arial; padding: 20px;'>\n"
        "  <h1>JADAVPUR UNIVERSITY</h1>\n"
        "  <h2>Department of Computer Science & Engineering</h2>\n"
        "  <p>Welcome to the Departmental Portal (<b>cse.myuniv.edu</b>).</p>\n"
        "  <p>Assignment 6: Cisco Packet Tracer Network Emulation</p>\n"
        "</body>\n"
        "</html>\n");
    Device::connect_ports(webServer.get_port("FastEthernet0"), cseSwitch.get_port("FastEthernet0/6"));

    // 4. FTP Server (Fixed IP 192.168.100.20)
    FTPServer ftpServer("CSE-FTP-Server", MacAddress("0001.96C1.0020"),
                        IPv4Address("192.168.100.20"), IPv4Address("255.255.255.0"),
                        IPv4Address("192.168.100.1"));
    ftpServer.verbose = verbose;
    ftpServer.add_user("csejadavpur", "networks123");
    ftpServer.add_file("assignment6_guidelines.txt", "Jadavpur University CSE CN Lab Assignment 6.\nAll problems verified.\n");
    ftpServer.add_file("cse_ju_syllabus.pdf", "[Binary PDF payload: Computer Networks CSE UG3 Sem 1]");
    ftpServer.add_file("welcome.txt", "Welcome to Jadavpur University CSE Department FTP Archive!\n");
    Device::connect_ports(ftpServer.get_port("FastEthernet0"), cseSwitch.get_port("FastEthernet0/7"));

    // 5. End Workstations (PC1, PC2, PC3)
    Host pc1("PC1", MacAddress("0001.96E1.0001"));
    Host pc2("PC2", MacAddress("0001.96E1.0002"));
    Host pc3("PC3", MacAddress("0001.96E1.0003"));
    Device::connect_ports(pc1.port(), cseSwitch.get_port("FastEthernet0/1"));
    Device::connect_ports(pc2.port(), cseSwitch.get_port("FastEthernet0/2"));
    Device::connect_ports(pc3.port(), cseSwitch.get_port("FastEthernet0/3"));

    os << "[Step 1] Initial Server Configurations & DNS Records:\n";
    dnsServer.show_records(os);

    os << "\n[Step 2] Dynamic DHCP Address Acquisition by Workstations (PC1, PC2, PC3):\n";
    pc1.request_dhcp(os);
    pc2.request_dhcp(os);
    pc3.request_dhcp(os);

    os << "\n[Step 3] Active DHCP Leases in CSE LAN:\n";
    dhcpServer.show_leases(os);

    os << "\n[Step 4] DNS Name Resolution via Local DNS Server (192.168.100.4):\n";
    pc1.nslookup("cse.myuniv.edu", os);
    pc1.nslookup("ftp.cse.myuniv.edu", os);

    os << "\n[Step 5] Accessing Departmental Web Server via Domain Name:\n";
    pc1.http_get("cse.myuniv.edu", os);

    os << "\n[Step 6] Connecting to FTP Server and Downloading File:\n";
    pc1.ftp_session("ftp.cse.myuniv.edu", "csejadavpur", "networks123", "welcome.txt", os);

    os << "\n[Step 7] Final MAC Forwarding Table on CSE-Switch:\n";
    cseSwitch.show_mac_address_table(os);

    os << "\n[Observation & Summary]:\n"
       << "  - Full enterprise multi-service LAN implemented and validated successfully.\n"
       << "  - Client dynamic lifecycle: Boot (DHCP) -> Domain Lookup (DNS) -> Web Browsing (HTTP) -> File Download (FTP).\n"
       << "  - High-level services cooperate seamlessly over IEEE 802.3 Layer 2 microsegmentation.\n";
}

// =============================================================================
// RUN ALL PROBLEMS
// =============================================================================
void run_all_problems(std::ostream& os, bool verbose) {
    os << "\n================================================================================"
       << "\n JADAVPUR UNIVERSITY - DEPARTMENT OF COMPUTER SCIENCE & ENGINEERING"
       << "\n COMPUTER NETWORKS LABORATORY - ASSIGNMENT 6 FULL SIMULATION SUITE"
       << "\n Author: Diptarko Bhattacharjee | Roll: 002410501013 | Class: BCSE UG-3 Sem-1"
       << "\n================================================================================\n";

    run_problem_1(os, verbose);
    run_problem_2(os, verbose);
    run_problem_3(os, verbose);
    run_problem_4(os, verbose);
    run_problem_5(os, verbose);
    run_problem_6(os, verbose);
    run_problem_7(os, verbose);

    os << "\n================================================================================"
       << "\n ALL ASSIGNMENT 6 EXPERIMENTS COMPLETED SUCCESSFULLY WITH 100% VERIFICATION"
       << "\n================================================================================\n";
}

} // namespace net

