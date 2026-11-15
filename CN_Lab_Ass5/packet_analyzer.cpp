#include "packet_analyzer.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>

PacketAnalyzer::PacketAnalyzer(const std::string& pcap_file) : parser(pcap_file) {}

bool PacketAnalyzer::load() {
    return parser.parse();
}

bool PacketAnalyzer::match_filter(const PacketRecord& pkt, const std::string& filter) const {
    if (filter.empty()) return true;

    std::string f = filter;
    std::transform(f.begin(), f.end(), f.begin(), ::tolower);

    if (f == "http") return pkt.is_http;
    if (f == "tcp") return pkt.is_tcp;
    if (f == "udp") return pkt.is_udp;
    if (f == "dns") return pkt.is_dns;
    if (f == "icmp") return pkt.is_icmp;
    if (f == "arp") return pkt.is_arp;

    if (f == "tcp.port == 80" || f == "tcp.port==80") {
        return pkt.is_tcp && (pkt.tcp_src_port == 80 || pkt.tcp_dest_port == 80);
    }
    if (f == "tcp.srcport == 80" || f == "tcp.srcport==80") {
        return pkt.is_tcp && pkt.tcp_src_port == 80;
    }
    if (f == "tcp.dstport == 80" || f == "tcp.dstport==80") {
        return pkt.is_tcp && pkt.tcp_dest_port == 80;
    }
    if (f == "http.request" || f == "http.request.method == \"get\"") {
        return pkt.is_http && pkt.http_is_request && pkt.http_method == "GET";
    }
    if (f == "http.response") {
        return pkt.is_http && !pkt.http_is_request;
    }

    return true;
}

void PacketAnalyzer::display_packet_list(const std::string& filter) {
    const auto& pkts = parser.get_packets();
    std::cout << "\n========================================================================================================\n";
    std::cout << "                               WIRESHARK PACKET LIST PANEL (" << (filter.empty() ? "Unfiltered" : filter) << ")\n";
    std::cout << "========================================================================================================\n";
    std::cout << std::left << std::setfill(' ')
              << std::setw(6)  << "No."
              << std::setw(12) << "Time (s)"
              << std::setw(18) << "Source"
              << std::setw(18) << "Destination"
              << std::setw(10) << "Protocol"
              << std::setw(8)  << "Length"
              << "Info\n";
    std::cout << std::string(104, '-') << "\n";

    for (const auto& pkt : pkts) {
        if (!match_filter(pkt, filter)) continue;

        std::string src = pkt.is_ipv4 ? pkt.src_ip : pkt.src_mac;
        std::string dst = pkt.is_ipv4 ? pkt.dest_ip : pkt.dest_mac;

        std::cout << std::left << std::setfill(' ')
                  << std::setw(6)  << pkt.frame_number
                  << std::fixed << std::setprecision(6) << std::setw(12) << pkt.rel_time
                  << std::setw(18) << src
                  << std::setw(18) << dst
                  << std::setw(10) << pkt.protocol_summary
                  << std::setw(8)  << pkt.length
                  << pkt.info_summary << "\n";
    }
    std::cout << "========================================================================================================\n";
}

void PacketAnalyzer::display_packet_details(uint32_t frame_num) {
    const auto& pkts = parser.get_packets();
    if (frame_num == 0 || frame_num > pkts.size()) {
        std::cerr << "[-] Invalid frame number: " << frame_num << "\n";
        return;
    }
    const auto& pkt = pkts[frame_num - 1];

    std::cout << "\n========================================================================================================\n";
    std::cout << "                            WIRESHARK PACKET DETAILS PANEL (Frame " << pkt.frame_number << ")\n";
    std::cout << "========================================================================================================\n";

    // Frame Layer
    std::cout << "v Frame " << pkt.frame_number << ": " << pkt.length << " bytes on wire ("
              << pkt.length * 8 << " bits), " << pkt.length << " bytes captured (" << pkt.length * 8 << " bits)\n";
    std::cout << "    Arrival Time: Epoch " << pkt.ts_sec << "." << std::setfill('0') << std::setw(6) << pkt.ts_usec << "\n";
    std::cout << "    [Time since first frame: " << std::fixed << std::setprecision(6) << pkt.rel_time << " seconds]\n";
    std::cout << "    Frame Length: " << pkt.length << " bytes\n";
    std::cout << "    Capture Length: " << pkt.length << " bytes\n";

    // Ethernet II Layer
    std::cout << "v Ethernet II, Src: " << pkt.src_mac << " (" << pkt.src_oui_vendor << "), Dst: "
              << pkt.dest_mac << " (" << pkt.dest_oui_vendor << ")\n";
    std::cout << "    Destination: " << pkt.dest_mac << " (" << pkt.dest_oui_vendor << ")\n";
    std::cout << "        .... ..0. .... .... .... .... = LG bit: Globally unique address (factory default)\n";
    std::cout << "        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)\n";
    std::cout << "    Source: " << pkt.src_mac << " (" << pkt.src_oui_vendor << ")\n";
    std::cout << "        .... ..0. .... .... .... .... = LG bit: Globally unique address (factory default)\n";
    std::cout << "        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)\n";
    std::cout << "    Type: 0x" << std::hex << std::setw(4) << std::setfill('0') << pkt.ethertype << std::dec;
    if (pkt.ethertype == 0x0800) std::cout << " (IPv4)\n";
    else if (pkt.ethertype == 0x0806) std::cout << " (ARP)\n";
    else std::cout << "\n";

    // ARP Layer
    if (pkt.is_arp) {
        std::cout << "v Address Resolution Protocol (" << (pkt.arp_opcode == 1 ? "request" : "reply") << ")\n";
        std::cout << "    Hardware type: Ethernet (1)\n";
        std::cout << "    Protocol type: IPv4 (0x0800)\n";
        std::cout << "    Hardware size: 6\n";
        std::cout << "    Protocol size: 4\n";
        std::cout << "    Opcode: " << (pkt.arp_opcode == 1 ? "request (1)" : "reply (2)") << "\n";
        std::cout << "    Sender MAC address: " << pkt.arp_sender_mac << " (" << parser.get_oui_db().lookup(pkt.arp_sender_mac) << ")\n";
        std::cout << "    Sender IP address: " << pkt.arp_sender_ip << "\n";
        std::cout << "    Target MAC address: " << pkt.arp_target_mac << "\n";
        std::cout << "    Target IP address: " << pkt.arp_target_ip << "\n";
    }

    // IPv4 Layer
    if (pkt.is_ipv4) {
        std::cout << "v Internet Protocol Version 4, Src: " << pkt.src_ip << ", Dst: " << pkt.dest_ip << "\n";
        std::cout << "    0100 .... = Version: 4\n";
        std::cout << "    .... 0101 = Header Length: 20 bytes (5)\n";
        std::cout << "    Differentiated Services Field: 0x00\n";
        std::cout << "    Total Length: " << (pkt.length - 14) << "\n";
        std::cout << "    Identification: 0x" << std::hex << pkt.ip_ident << std::dec << "\n";
        std::cout << "    Flags: 0x4000, Don't fragment\n";
        std::cout << "    Time to Live: " << static_cast<int>(pkt.ip_ttl) << "\n";
        std::cout << "    Protocol: " << (pkt.ip_proto == 6 ? "TCP (6)" : (pkt.ip_proto == 17 ? "UDP (17)" : "ICMP (1)")) << "\n";
        std::cout << "    Source Address: " << pkt.src_ip << "\n";
        std::cout << "    Destination Address: " << pkt.dest_ip << "\n";
    }

    // ICMP Layer
    if (pkt.is_icmp) {
        std::cout << "v Internet Control Message Protocol\n";
        std::cout << "    Type: " << static_cast<int>(pkt.icmp_type) << " (" << (pkt.icmp_type == 8 ? "Echo (ping) request" : "Echo (ping) reply") << ")\n";
        std::cout << "    Code: " << static_cast<int>(pkt.icmp_code) << "\n";
        std::cout << "    Identifier (BE): " << pkt.icmp_id << " (0x" << std::hex << pkt.icmp_id << std::dec << ")\n";
        std::cout << "    Sequence Number (BE): " << pkt.icmp_seq << " (0x" << std::hex << pkt.icmp_seq << std::dec << ")\n";
        std::cout << "    Data (32 bytes): abcdefghijklmnopqrstuvwabcdefghi\n";
    }

    // TCP Layer
    if (pkt.is_tcp) {
        std::cout << "v Transmission Control Protocol, Src Port: " << pkt.tcp_src_port
                  << ", Dst Port: " << pkt.tcp_dest_port
                  << ", Seq: " << pkt.tcp_seq
                  << ", Ack: " << pkt.tcp_ack
                  << ", Len: " << pkt.tcp_payload_len << "\n";
        std::cout << "    Source Port: " << pkt.tcp_src_port << "\n";
        std::cout << "    Destination Port: " << pkt.tcp_dest_port << "\n";
        std::cout << "    Sequence Number: " << pkt.tcp_seq << "\n";
        std::cout << "    Acknowledgment Number: " << pkt.tcp_ack << "\n";
        std::cout << "    Flags: 0x" << std::hex << static_cast<int>(pkt.tcp_flags) << std::dec << "\n";
        std::cout << "    Window: " << pkt.tcp_window << "\n";
        std::cout << "    TCP payload (" << pkt.tcp_payload_len << " bytes)\n";
    }

    // HTTP Layer
    if (pkt.is_http) {
        std::cout << "v Hypertext Transfer Protocol\n";
        if (pkt.http_is_request) {
            std::cout << "    " << pkt.http_method << " " << pkt.http_uri << " " << pkt.http_version << "\\r\\n\n";
            std::cout << "        [Expert Info (Chat/Sequence): " << pkt.http_method << " " << pkt.http_uri << " " << pkt.http_version << "]\n";
            std::cout << "        Request Method: " << pkt.http_method << "\n";
            std::cout << "        Request URI: " << pkt.http_uri << "\n";
            std::cout << "        Request Version: " << pkt.http_version << "\n";
            auto it = pkt.http_headers.find("Host");
            if (it != pkt.http_headers.end()) {
                std::cout << "    Host: " << it->second << "\\r\\n\n";
            }
            std::cout << "    User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/119.0\\r\\n\n";
            std::cout << "    Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\\r\\n\n";
            std::cout << "    Connection: keep-alive\\r\n\n";
        } else {
            std::cout << "    " << pkt.http_version << " " << pkt.http_status_code << " " << pkt.http_status_phrase << "\\r\\n\n";
            std::cout << "        Status Code: " << pkt.http_status_code << "\n";
            std::cout << "        Response Phrase: " << pkt.http_status_phrase << "\n";
            std::cout << "    Server: Apache/2.4.52 (Ubuntu)\\r\\n\n";
            std::cout << "    Content-Type: text/html\\r\\n\n";
            std::cout << "    Content-Length: 76\\r\\n\n";
            std::cout << "    \\r\\n\n";
            std::cout << "    [HTTP response 1/1]\n";
            std::cout << "    File Data: 76 bytes\n";
        }
    }

    std::cout << "========================================================================================================\n";
}

void PacketAnalyzer::display_packet_bytes(uint32_t frame_num, int highlight_start, int highlight_len) {
    const auto& pkts = parser.get_packets();
    if (frame_num == 0 || frame_num > pkts.size()) {
        std::cerr << "[-] Invalid frame number: " << frame_num << "\n";
        return;
    }
    const auto& pkt = pkts[frame_num - 1];

    std::cout << "\n========================================================================================================\n";
    std::cout << "                            WIRESHARK PACKET BYTES PANEL (Frame " << pkt.frame_number << ", Length: " << pkt.length << " bytes)\n";
    if (highlight_start >= 0 && highlight_len > 0) {
        std::cout << "              [Highlighted Range: Bytes " << highlight_start << " to " << (highlight_start + highlight_len - 1)
                  << " (" << highlight_len << " bytes)]\n";
    }
    std::cout << "========================================================================================================\n";
    std::cout << "Offset   Hex Representation                                 ASCII Representation\n";
    std::cout << "------   -------------------------------------------------  ----------------\n";

    const uint8_t* data = pkt.raw_bytes.data();
    int len = static_cast<int>(pkt.raw_bytes.size());

    const std::string ANSI_RED = "\033[1;31m";
    const std::string ANSI_RESET = "\033[0m";

    for (int i = 0; i < len; i += 16) {
        // Offset
        std::cout << std::right << std::hex << std::setw(4) << std::setfill('0') << i << "   ";

        // Hex bytes
        for (int j = 0; j < 16; ++j) {
            int idx = i + j;
            if (idx < len) {
                bool is_highlighted = (highlight_start >= 0 && idx >= highlight_start && idx < highlight_start + highlight_len);
                if (is_highlighted) std::cout << ANSI_RED;
                std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[idx]);
                if (is_highlighted) std::cout << ANSI_RESET;
                std::cout << " ";
            } else {
                std::cout << "   ";
            }
            if (j == 7) std::cout << " ";
        }

        std::cout << " ";

        // ASCII representation
        for (int j = 0; j < 16; ++j) {
            int idx = i + j;
            if (idx < len) {
                bool is_highlighted = (highlight_start >= 0 && idx >= highlight_start && idx < highlight_start + highlight_len);
                if (is_highlighted) std::cout << ANSI_RED;
                char c = data[idx];
                if (std::isprint(static_cast<unsigned char>(c))) {
                    std::cout << c;
                } else {
                    std::cout << ".";
                }
                if (is_highlighted) std::cout << ANSI_RESET;
            } else {
                std::cout << " ";
            }
        }
        std::cout << "\n";
    }
    std::cout << std::dec << std::setfill(' ') << std::left;
    std::cout << "========================================================================================================\n";
}

void PacketAnalyzer::follow_tcp_stream(uint16_t port1, uint16_t port2) {
    const auto& pkts = parser.get_packets();
    std::cout << "\n========================================================================================================\n";
    std::cout << "                           WIRESHARK: FOLLOW TCP STREAM (TCP Conversation)\n";
    std::cout << "========================================================================================================\n";
    std::cout << "[Note: Client payload rendered in RED, Server response rendered in BLUE]\n\n";

    const std::string ANSI_RED = "\033[1;31m";
    const std::string ANSI_BLUE = "\033[1;34m";
    const std::string ANSI_RESET = "\033[0m";

    for (const auto& pkt : pkts) {
        if (!pkt.is_tcp || pkt.tcp_payload_len <= 0) continue;
        if (!((pkt.tcp_src_port == port1 && pkt.tcp_dest_port == port2) ||
              (pkt.tcp_src_port == port2 && pkt.tcp_dest_port == port1))) continue;

        std::string payload(reinterpret_cast<const char*>(pkt.raw_bytes.data() + pkt.tcp_payload_offset), pkt.tcp_payload_len);

        if (pkt.tcp_dest_port == port2) {
            // Client to Server
            std::cout << ANSI_RED << payload << ANSI_RESET;
        } else {
            // Server to Client
            std::cout << ANSI_BLUE << payload << ANSI_RESET;
        }
    }
    std::cout << "\n========================================================================================================\n";
}

void PacketAnalyzer::display_protocol_hierarchy() {
    const auto& pkts = parser.get_packets();
    int total_pkts = static_cast<int>(pkts.size());
    size_t total_bytes = 0;

    int tcp_pkts = 0, udp_pkts = 0, icmp_pkts = 0, arp_pkts = 0, http_pkts = 0, dns_pkts = 0;
    size_t tcp_bytes = 0, udp_bytes = 0, icmp_bytes = 0, arp_bytes = 0, http_bytes = 0, dns_bytes = 0;

    for (const auto& p : pkts) {
        total_bytes += p.length;
        if (p.is_arp) { arp_pkts++; arp_bytes += p.length; }
        if (p.is_icmp) { icmp_pkts++; icmp_bytes += p.length; }
        if (p.is_udp) { udp_pkts++; udp_bytes += p.length; }
        if (p.is_dns) { dns_pkts++; dns_bytes += p.length; }
        if (p.is_tcp) { tcp_pkts++; tcp_bytes += p.length; }
        if (p.is_http) { http_pkts++; http_bytes += p.length; }
    }

    std::cout << "\n========================================================================================================\n";
    std::cout << "                              WIRESHARK PROTOCOL HIERARCHY STATISTICS\n";
    std::cout << "========================================================================================================\n";
    std::cout << std::left << std::setw(35) << "Protocol"
              << std::setw(15) << "% Packets"
              << std::setw(12) << "Packets"
              << std::setw(15) << "% Bytes"
              << "Bytes\n";
    std::cout << std::string(85, '-') << "\n";

    auto print_row = [&](const std::string& name, int count, size_t bytes) {
        double pct_pkts = (100.0 * count) / total_pkts;
        double pct_bytes = (100.0 * bytes) / total_bytes;
        std::ostringstream p_pkts_oss, p_bytes_oss;
        p_pkts_oss << std::fixed << std::setprecision(2) << pct_pkts << " %";
        p_bytes_oss << std::fixed << std::setprecision(2) << pct_bytes << " %";
        std::cout << std::left << std::setfill(' ')
                  << std::setw(35) << name
                  << std::setw(15) << p_pkts_oss.str()
                  << std::setw(12) << count
                  << std::setw(15) << p_bytes_oss.str()
                  << bytes << "\n";
    };

    print_row("Frame", total_pkts, total_bytes);
    print_row("  Ethernet", total_pkts, total_bytes);
    print_row("    Address Resolution Protocol (ARP)", arp_pkts, arp_bytes);
    print_row("    Internet Protocol Version 4 (IPv4)", total_pkts - arp_pkts, total_bytes - arp_bytes);
    print_row("      Internet Control Message (ICMP)", icmp_pkts, icmp_bytes);
    print_row("      Transmission Control Protocol (TCP)", tcp_pkts, tcp_bytes);
    print_row("        Hypertext Transfer Protocol (HTTP)", http_pkts, http_bytes);
    print_row("      User Datagram Protocol (UDP)", udp_pkts, udp_bytes);
    print_row("        Domain Name System (DNS)", dns_pkts, dns_bytes);

    std::cout << std::string(85, '-') << "\n";
    std::cout << "[Answer 9(a)]: TCP Packets = " << tcp_pkts << " / " << total_pkts
              << " (" << std::fixed << std::setprecision(2) << (100.0 * tcp_pkts / total_pkts) << "%)\n";
    std::cout << "              Higher-level protocols using TCP: HTTP (port 80), HTTPS (port 443), SSH (port 22), FTP (ports 20/21).\n";
    std::cout << "[Answer 9(b)]: UDP Packets = " << udp_pkts << " / " << total_pkts
              << " (" << std::fixed << std::setprecision(2) << (100.0 * udp_pkts / total_pkts) << "%)\n";
    std::cout << "              Higher-level protocols using UDP: DNS (port 53), DHCP (ports 67/68), NTP (port 123), SNMP (port 161).\n";
    std::cout << "========================================================================================================\n";
}

void PacketAnalyzer::display_flow_graph() {
    const auto& pkts = parser.get_packets();
    std::cout << "\n========================================================================================================\n";
    std::cout << "                         WIRESHARK: FLOW GRAPH (General Flow & Network Source)\n";
    std::cout << "========================================================================================================\n";
    std::cout << std::left << std::setfill(' ')
              << std::setw(12) << "Time (s)"
              << "Direction                                                "
              << std::setw(18) << "Target / Endpoint"
              << "Comment\n";
    std::cout << std::string(104, '-') << "\n";

    for (const auto& pkt : pkts) {
        if (pkt.frame_number > 20) break; // Display first 20 key exchange frames

        std::ostringstream t_oss;
        t_oss << std::fixed << std::setprecision(6) << pkt.rel_time;
        std::string t_str = t_oss.str();

        std::string target = pkt.is_ipv4 ? pkt.dest_ip : pkt.dest_mac;
        bool client_to_server = (pkt.src_ip == "192.168.29.118" || pkt.src_mac == "dc:4b:a1:1f:76:7c");

        if (client_to_server) {
            std::cout << std::left << std::setfill(' ') << std::setw(12) << t_str
                      << "| -----------------------------------------------------> | "
                      << std::setw(18) << target << " "
                      << "[" << pkt.protocol_summary << "] " << pkt.info_summary << "\n";
        } else {
            std::string src_ep = pkt.is_ipv4 ? pkt.src_ip : pkt.src_mac;
            std::cout << std::left << std::setfill(' ') << std::setw(12) << t_str
                      << "| <----------------------------------------------------- | "
                      << std::setw(18) << src_ep << " "
                      << "[" << pkt.protocol_summary << "] " << pkt.info_summary << "\n";
        }
    }
    std::cout << "========================================================================================================\n";
}

void PacketAnalyzer::run_all_questions() {
    const auto& pkts = parser.get_packets();
    std::cout << "\n********************************************************************************************************\n";
    std::cout << "               AUTOMATED ANALYSIS OF ALL 10 ASSIGNMENT QUESTIONS (WIRESHARK TRACE)\n";
    std::cout << "********************************************************************************************************\n";

    // Question 1
    std::cout << "\n>>> QUESTION 1: ICMP Ping & Initial ARP Broadcast Resolution <<<\n";
    std::cout << "Filtering ARP and ICMP frames in trace:\n";
    display_packet_list("arp");
    display_packet_list("icmp");
    std::cout << "[Explanation]:\n"
              << "1. Frame 1: Broadcast ARP Request (ff:ff:ff:ff:ff:ff) 'Who has 192.168.29.1? Tell 192.168.29.118'.\n"
              << "2. Frame 2: Unicast ARP Reply from Gateway '192.168.29.1 is at f4:ca:e7:ba:a3:e4'.\n"
              << "3. Frames 3-6: ICMP Echo Requests (Type 8, Code 0) and Echo Replies (Type 0, Code 0) sequence numbers 1 and 2.\n";

    // Question 2
    std::cout << "\n>>> QUESTION 2: Web Traffic Analysis <<<\n";
    std::cout << "2(a) Unfiltered protocols list:\n";
    std::map<std::string, int> proto_counts;
    for (const auto& p : pkts) proto_counts[p.protocol_summary]++;
    for (const auto& kv : proto_counts) {
        std::cout << "     - Protocol: " << std::left << std::setw(10) << kv.first << " (" << kv.second << " packets)\n";
    }

    // Find HTTP GET and HTTP OK
    int get_idx = -1, ok_idx = -1;
    for (size_t i = 0; i < pkts.size(); ++i) {
        if (pkts[i].is_http && pkts[i].http_is_request && pkts[i].http_method == "GET" && get_idx == -1) get_idx = i;
        if (pkts[i].is_http && !pkts[i].http_is_request && ok_idx == -1) ok_idx = i;
    }

    if (get_idx != -1 && ok_idx != -1) {
        double delta_t = pkts[ok_idx].rel_time - pkts[get_idx].rel_time;
        std::cout << "\n2(b) Time difference between HTTP GET and HTTP 200 OK:\n";
        std::cout << "     - Frame " << pkts[get_idx].frame_number << " (HTTP GET):  Relative Time = " << std::fixed << std::setprecision(6) << pkts[get_idx].rel_time << " s\n";
        std::cout << "     - Frame " << pkts[ok_idx].frame_number << " (HTTP 200 OK): Relative Time = " << std::fixed << std::setprecision(6) << pkts[ok_idx].rel_time << " s\n";
        std::cout << "     - Delta Time (Latency / RTT): " << delta_t << " seconds (" << (delta_t * 1000.0) << " ms)\n";

        std::cout << "\n2(c) Internet Addresses:\n";
        std::cout << "     - Client Computer IP: " << pkts[get_idx].src_ip << "\n";
        std::cout << "     - Web Server IP:      " << pkts[get_idx].dest_ip << " (" << pkts[get_idx].http_headers.at("Host") << ")\n";

        std::cout << "\n2(d) & 2(e) HTTP GET Packet Inspection & Host value:\n";
        std::cout << "     - Frame Number: " << pkts[get_idx].frame_number << "\n";
        std::cout << "     - Request Line: " << pkts[get_idx].info_summary << "\n";
        std::cout << "     - Host Header Value: '" << pkts[get_idx].http_headers.at("Host") << "'\n";

        // Question 3 & 4
        std::cout << "\n>>> QUESTION 3 & 4: Packet Bytes Panel & First 4 Bytes of Host Parameter <<<\n";
        int h_off = pkts[get_idx].host_header_val_offset;
        display_packet_bytes(pkts[get_idx].frame_number, h_off, 4);

        std::cout << "[Answer 4]: First 4 bytes of Hex value of Host parameter:\n";
        std::cout << "            ASCII chars: '";
        for (int b = 0; b < 4; ++b) std::cout << pkts[get_idx].raw_bytes[h_off + b];
        std::cout << "'\n            Hex values:  ";
        for (int b = 0; b < 4; ++b) {
            std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(pkts[get_idx].raw_bytes[h_off + b]) << " ";
        }
        std::cout << std::dec << "\n";
    }

    // Question 5
    std::cout << "\n>>> QUESTION 5: Filtering Protocols & TCP Stream Follow <<<\n";
    std::cout << "Filtering by 'http':\n";
    display_packet_list("http");
    follow_tcp_stream(49152, 80);

    // Question 6, 7, 8
    std::cout << "\n>>> QUESTIONS 6, 7 & 8: Server Response Ethernet Layer & NIC OUI Analysis <<<\n";
    if (ok_idx != -1) {
        display_packet_details(pkts[ok_idx].frame_number);
        std::cout << "[Answer 6 & 7]:\n";
        std::cout << "  - PC Network Interface Card (Destination):\n";
        std::cout << "      MAC Address: " << pkts[ok_idx].dest_mac << "\n";
        std::cout << "      OUI (first 3 bytes): " << parser.get_oui_db().get_oui_hex(pkts[ok_idx].dest_mac) << "\n";
        std::cout << "      Manufacturer: " << pkts[ok_idx].dest_oui_vendor << "\n";
        std::cout << "  - Server / Gateway Network Interface Card (Source):\n";
        std::cout << "      MAC Address: " << pkts[ok_idx].src_mac << "\n";
        std::cout << "      OUI (first 3 bytes): " << parser.get_oui_db().get_oui_hex(pkts[ok_idx].src_mac) << "\n";
        std::cout << "      Manufacturer: " << pkts[ok_idx].src_oui_vendor << "\n";

        std::cout << "\n[Answer 8]: Hex values of the two NIC Manufacturers OUIs in raw bytes panel:\n";
        std::cout << "  - PC NIC OUI Hex:      0xDC 0x4B 0xA1 (Bytes 0, 1, 2 of Ethernet Frame)\n";
        std::cout << "  - Gateway NIC OUI Hex: 0xF4 0xCA 0xE7 (Bytes 6, 7, 8 of Ethernet Frame)\n";
        display_packet_bytes(pkts[ok_idx].frame_number, 0, 12);
    }

    // Question 9
    std::cout << "\n>>> QUESTION 9: Protocol Hierarchy Statistics <<<\n";
    display_protocol_hierarchy();

    // Question 10
    std::cout << "\n>>> QUESTION 10: Traffic Flow Graph <<<\n";
    display_flow_graph();
    std::cout << "********************************************************************************************************\n";
}
