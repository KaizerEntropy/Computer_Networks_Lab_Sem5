#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>

#pragma pack(push, 1)

// Standard Libpcap Global Header (24 bytes)
struct PcapGlobalHeader {
    uint32_t magic_number;   // 0xa1b2c3d4 or 0xd4c3b2a1
    uint16_t version_major;  // 2
    uint16_t version_minor;  // 4
    int32_t  thiszone;       // GMT to local correction
    uint32_t sigfigs;        // accuracy of timestamps
    uint32_t snaplen;        // max length of captured packets
    uint32_t network;        // data link type (LINKTYPE_ETHERNET = 1)
};

// Standard Libpcap Packet Header (16 bytes)
struct PcapPacketHeader {
    uint32_t ts_sec;         // timestamp seconds
    uint32_t ts_usec;        // timestamp microseconds
    uint32_t incl_len;       // number of octets of packet saved in file
    uint32_t orig_len;       // actual length of packet
};

// Ethernet II Header (14 bytes)
struct EthernetHeader {
    uint8_t  dest_mac[6];
    uint8_t  src_mac[6];
    uint16_t ethertype;      // Big-endian: 0x0800 = IPv4, 0x0806 = ARP
};

// ARP Packet Header (28 bytes)
struct ArpHeader {
    uint16_t hw_type;        // 1 = Ethernet
    uint16_t proto_type;     // 0x0800 = IPv4
    uint8_t  hw_size;        // 6
    uint8_t  proto_size;     // 4
    uint16_t opcode;         // 1 = Request, 2 = Reply
    uint8_t  sender_mac[6];
    uint8_t  sender_ip[4];
    uint8_t  target_mac[6];
    uint8_t  target_ip[4];
};

// IPv4 Header (Minimum 20 bytes)
struct Ipv4Header {
    uint8_t  ver_ihl;        // Version (4 bits) + Internet Header Length (4 bits)
    uint8_t  tos;            // Type of service
    uint16_t total_length;   // Total length in octets
    uint16_t identification; // Identification
    uint16_t flags_fo;       // Flags (3 bits) + Fragment Offset (13 bits)
    uint8_t  ttl;            // Time to Live
    uint8_t  protocol;       // 1 = ICMP, 6 = TCP, 17 = UDP
    uint16_t checksum;       // Header checksum
    uint8_t  src_ip[4];      // Source IPv4 address
    uint8_t  dest_ip[4];     // Destination IPv4 address
};

// ICMP Header (8 bytes base)
struct IcmpHeader {
    uint8_t  type;           // 8 = Echo Request, 0 = Echo Reply
    uint8_t  code;           // 0
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
};

// UDP Header (8 bytes)
struct UdpHeader {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
};

// TCP Header (Minimum 20 bytes)
struct TcpHeader {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t  data_offset_res; // Data offset (4 bits) + Reserved (4 bits)
    uint8_t  flags;           // CWR, ECE, URG, ACK, PSH, RST, SYN, FIN
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urg_ptr;
};

#pragma pack(pop)

// Dissected Protocol Data Structure
struct PacketRecord {
    uint32_t frame_number;
    uint32_t ts_sec;
    uint32_t ts_usec;
    double   rel_time;       // relative to first packet in seconds
    uint32_t length;
    
    // Raw packet bytes
    std::vector<uint8_t> raw_bytes;

    // Layer 2 - Ethernet
    std::string src_mac;
    std::string dest_mac;
    std::string src_oui_vendor;
    std::string dest_oui_vendor;
    uint16_t    ethertype;
    int         eth_offset = 0;
    int         eth_len = 14;

    // Layer 2.5 - ARP
    bool        is_arp = false;
    uint16_t    arp_opcode;
    std::string arp_sender_mac;
    std::string arp_sender_ip;
    std::string arp_target_mac;
    std::string arp_target_ip;
    int         arp_offset = 14;

    // Layer 3 - IPv4
    bool        is_ipv4 = false;
    std::string src_ip;
    std::string dest_ip;
    uint8_t     ip_proto;
    uint8_t     ip_ttl;
    uint16_t    ip_ident;
    int         ip_offset = 14;
    int         ip_len = 20;

    // Layer 4 - ICMP
    bool        is_icmp = false;
    uint8_t     icmp_type;
    uint8_t     icmp_code;
    uint16_t    icmp_id;
    uint16_t    icmp_seq;
    int         icmp_offset = 34;

    // Layer 4 - UDP
    bool        is_udp = false;
    uint16_t    udp_src_port;
    uint16_t    udp_dest_port;
    int         udp_offset = 34;
    int         udp_len = 8;

    // Layer 4 - TCP
    bool        is_tcp = false;
    uint16_t    tcp_src_port;
    uint16_t    tcp_dest_port;
    uint32_t    tcp_seq;
    uint32_t    tcp_ack;
    uint8_t     tcp_flags;
    uint16_t    tcp_window;
    int         tcp_offset = 34;
    int         tcp_len = 20;
    int         tcp_payload_offset = 54;
    int         tcp_payload_len = 0;

    // Layer 7 - DNS
    bool        is_dns = false;
    uint16_t    dns_tx_id;
    bool        dns_is_response;
    std::string dns_query_name;
    std::string dns_answer_ip;

    // Layer 7 - HTTP
    bool        is_http = false;
    bool        http_is_request = false;
    std::string http_method;
    std::string http_uri;
    std::string http_version;
    int         http_status_code = 0;
    std::string http_status_phrase;
    std::map<std::string, std::string> http_headers;
    std::string http_body;
    int         http_offset = 0;
    int         http_len = 0;
    int         host_header_offset = -1;
    int         host_header_val_offset = -1;
    int         host_header_val_len = 0;

    // Higher-level summary for Packet List Panel
    std::string protocol_summary;
    std::string info_summary;
};

#endif // PROTOCOL_HPP

