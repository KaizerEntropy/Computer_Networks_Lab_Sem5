#ifndef TYPES_HPP
#define TYPES_HPP

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <map>
#include <memory>
#include <algorithm>

namespace net {

// -----------------------------------------------------------------------------
// 1. MAC ADDRESS (48-bit Hardware Address)
// -----------------------------------------------------------------------------
struct MacAddress {
    std::array<uint8_t, 6> bytes{};

    MacAddress() {
        bytes.fill(0);
    }

    MacAddress(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5) {
        bytes = {b0, b1, b2, b3, b4, b5};
    }

    explicit MacAddress(const std::string& str) {
        bytes.fill(0);
        // Supports "00:11:22:33:44:55" or "0011.2233.4455" or "00-11-22-33-44-55"
        std::string clean;
        for (char c : str) {
            if (isxdigit(c)) clean += c;
        }
        if (clean.size() >= 12) {
            for (int i = 0; i < 6; ++i) {
                std::string byteStr = clean.substr(i * 2, 2);
                bytes[i] = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
            }
        }
    }

    static MacAddress broadcast() {
        return MacAddress(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    }

    static MacAddress zero() {
        return MacAddress(0, 0, 0, 0, 0, 0);
    }

    bool is_broadcast() const {
        for (uint8_t b : bytes) if (b != 0xFF) return false;
        return true;
    }

    bool is_zero() const {
        for (uint8_t b : bytes) if (b != 0) return false;
        return true;
    }

    bool operator==(const MacAddress& other) const {
        return bytes == other.bytes;
    }

    bool operator!=(const MacAddress& other) const {
        return bytes != other.bytes;
    }

    bool operator<(const MacAddress& other) const {
        return bytes < other.bytes;
    }

    // Standard Cisco notation: xxxx.xxxx.xxxx
    std::string to_cisco_string() const {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        oss << std::setw(2) << (int)bytes[0] << std::setw(2) << (int)bytes[1] << "."
            << std::setw(2) << (int)bytes[2] << std::setw(2) << (int)bytes[3] << "."
            << std::setw(2) << (int)bytes[4] << std::setw(2) << (int)bytes[5];
        return oss.str();
    }

    // Standard IEEE notation: xx:xx:xx:xx:xx:xx
    std::string to_string() const {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (size_t i = 0; i < 6; ++i) {
            oss << std::setw(2) << (int)bytes[i];
            if (i < 5) oss << ":";
        }
        return oss.str();
    }
};

inline std::ostream& operator<<(std::ostream& os, const MacAddress& mac) {
    return os << mac.to_cisco_string();
}

// -----------------------------------------------------------------------------
// 2. IPv4 ADDRESS (32-bit Logical Address)
// -----------------------------------------------------------------------------
struct IPv4Address {
    uint32_t val{0}; // Host byte order

    IPv4Address() = default;

    IPv4Address(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
        val = (static_cast<uint32_t>(a) << 24) |
              (static_cast<uint32_t>(b) << 16) |
              (static_cast<uint32_t>(c) << 8)  |
              (static_cast<uint32_t>(d));
    }

    explicit IPv4Address(uint32_t v) : val(v) {}

    explicit IPv4Address(const std::string& ipStr) {
        val = 0;
        std::stringstream ss(ipStr);
        std::string seg;
        uint32_t parts[4] = {0, 0, 0, 0};
        int idx = 0;
        while (std::getline(ss, seg, '.') && idx < 4) {
            parts[idx++] = static_cast<uint32_t>(std::stoul(seg));
        }
        val = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];
    }

    static IPv4Address any() { return IPv4Address(0, 0, 0, 0); }
    static IPv4Address broadcast() { return IPv4Address(255, 255, 255, 255); }

    bool is_broadcast() const { return val == 0xFFFFFFFF; }
    bool is_zero() const { return val == 0; }

    bool operator==(const IPv4Address& o) const { return val == o.val; }
    bool operator!=(const IPv4Address& o) const { return val != o.val; }
    bool operator<(const IPv4Address& o) const { return val < o.val; }

    uint8_t octet(int i) const {
        return static_cast<uint8_t>((val >> (24 - i * 8)) & 0xFF);
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << (int)octet(0) << "." << (int)octet(1) << "."
            << (int)octet(2) << "." << (int)octet(3);
        return oss.str();
    }

    bool in_same_subnet(const IPv4Address& other, const IPv4Address& mask) const {
        return (val & mask.val) == (other.val & mask.val);
    }

    IPv4Address network_address(const IPv4Address& mask) const {
        return IPv4Address(val & mask.val);
    }

    IPv4Address broadcast_address(const IPv4Address& mask) const {
        return IPv4Address(val | (~mask.val));
    }
};

inline std::ostream& operator<<(std::ostream& os, const IPv4Address& ip) {
    return os << ip.to_string();
}

// -----------------------------------------------------------------------------
// 3. PROTOCOL ENUMS & STRUCTURES
// -----------------------------------------------------------------------------
enum class EtherType : uint16_t {
    IPv4 = 0x0800,
    ARP  = 0x0806
};

enum class ARPOpcode : uint16_t {
    Request = 1,
    Reply   = 2
};

struct ARPPacket {
    ARPOpcode   opcode{ARPOpcode::Request};
    MacAddress  sender_mac;
    IPv4Address sender_ip;
    MacAddress  target_mac;
    IPv4Address target_ip;
};

enum class IPProtocol : uint8_t {
    ICMP = 1,
    TCP  = 6,
    UDP  = 17
};

enum class ICMPType : uint8_t {
    EchoReply              = 0,
    DestinationUnreachable = 3,
    EchoRequest            = 8,
    TimeExceeded           = 11
};

struct ICMPPacket {
    ICMPType    type{ICMPType::EchoRequest};
    uint8_t     code{0};
    uint16_t    identifier{0};
    uint16_t    sequence_num{0};
    std::string payload;
};

struct UDPPacket {
    uint16_t    src_port{0};
    uint16_t    dst_port{0};
    std::vector<uint8_t> payload;
};

// --- DHCP ---
enum class DHCPType : uint8_t {
    Discover = 1,
    Offer    = 2,
    Request  = 3,
    Ack      = 5,
    Nak      = 6
};

struct DHCPMessage {
    uint8_t     op{1}; // 1 = BootRequest, 2 = BootReply
    uint32_t    xid{0}; // Transaction ID
    IPv4Address ciaddr; // Client IP
    IPv4Address yiaddr; // Your (Client) IP
    IPv4Address siaddr; // Server IP
    MacAddress  chaddr; // Client hardware MAC
    DHCPType    msg_type{DHCPType::Discover};
    IPv4Address subnet_mask{255, 255, 255, 0};
    IPv4Address default_gateway;
    IPv4Address dns_server;
    uint32_t    lease_time{86400};
};

// --- DNS ---
struct DNSRecord {
    std::string name;
    std::string type{"A"};
    uint32_t    ttl{300};
    IPv4Address address;
};

struct DNSMessage {
    uint16_t    id{0};
    bool        is_response{false};
    std::string query_name;
    std::string query_type{"A"};
    std::vector<DNSRecord> answers;
};

// --- HTTP ---
struct HTTPMessage {
    bool        is_request{true};
    std::string method{"GET"};
    std::string uri{"/"};
    int         status_code{200};
    std::string status_text{"OK"};
    std::map<std::string, std::string> headers;
    std::string body;
};

// --- FTP ---
struct FTPMessage {
    bool        is_command{true};
    std::string cmd;
    std::string argument;
    int         reply_code{220};
    std::string reply_text;
    std::string data_payload;
};

// --- IPv4 Packet Encapsulation ---
struct IPv4Packet {
    IPv4Address src_ip;
    IPv4Address dst_ip;
    IPProtocol  protocol{IPProtocol::ICMP};
    uint8_t     ttl{64};
    uint16_t    identification{0};

    // Sub-layer payloads
    ICMPPacket  icmp;
    UDPPacket   udp;
    DHCPMessage dhcp;
    DNSMessage  dns;
    HTTPMessage http;
    FTPMessage  ftp;
};

// --- Layer 2 Ethernet Frame ---
struct EthernetFrame {
    MacAddress  dst_mac;
    MacAddress  src_mac;
    EtherType   ethertype{EtherType::IPv4};
    
    ARPPacket   arp;
    IPv4Packet  ip;

    std::string summary() const {
        std::ostringstream oss;
        oss << "[" << src_mac.to_cisco_string() << " -> " << dst_mac.to_cisco_string() << "] ";
        if (ethertype == EtherType::ARP) {
            oss << "ARP " << (arp.opcode == ARPOpcode::Request ? "Who-has " : "Reply ")
                << arp.target_ip << " (" << arp.sender_ip << ")";
        } else if (ethertype == EtherType::IPv4) {
            oss << "IPv4 " << ip.src_ip << " -> " << ip.dst_ip << " ";
            if (ip.protocol == IPProtocol::ICMP) {
                oss << "ICMP " << (ip.icmp.type == ICMPType::EchoRequest ? "Echo Req" : "Echo Reply")
                    << " seq=" << ip.icmp.sequence_num;
            } else if (ip.protocol == IPProtocol::UDP) {
                oss << "UDP " << ip.udp.src_port << "->" << ip.udp.dst_port;
                if (ip.udp.dst_port == 67 || ip.udp.dst_port == 68) {
                    oss << " (DHCP " << (int)ip.dhcp.msg_type << ")";
                } else if (ip.udp.dst_port == 53 || ip.udp.src_port == 53) {
                    oss << " (DNS " << ip.dns.query_name << ")";
                }
            } else if (ip.protocol == IPProtocol::TCP) {
                oss << "TCP";
            }
        }
        return oss.str();
    }
};

} // namespace net

#endif // TYPES_HPP
