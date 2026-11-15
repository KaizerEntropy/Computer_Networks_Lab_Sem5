#pragma once

#include "common.hpp"
#include <string>
#include <vector>

namespace bdtp {

constexpr uint8_t BOOTREQUEST = 1;
constexpr uint8_t BOOTREPLY   = 2;

constexpr uint8_t HTYPE_ETHER = 1;
constexpr uint8_t HLEN_ETHER  = 6;

// RFC 1048 Magic Cookie (99.130.83.99)
constexpr uint8_t BOOTP_MAGIC_COOKIE[4] = {99, 130, 83, 99};

// Standard BOOTP Vendor Options
constexpr uint8_t BOOTP_TAG_PAD         = 0;
constexpr uint8_t BOOTP_TAG_SUBNET_MASK = 1;
constexpr uint8_t BOOTP_TAG_TIME_OFFSET = 2;
constexpr uint8_t BOOTP_TAG_GATEWAY     = 3;
constexpr uint8_t BOOTP_TAG_DNS         = 6;
constexpr uint8_t BOOTP_TAG_HOSTNAME    = 12;
constexpr uint8_t BOOTP_TAG_END         = 255;

#pragma pack(push, 1)
struct BootpRawPacket {
    uint8_t  op;              // 1=BOOTREQUEST, 2=BOOTREPLY
    uint8_t  htype;           // 1=Ethernet
    uint8_t  hlen;            // 6 for MAC
    uint8_t  hops;            // 0
    uint32_t xid;             // Transaction ID
    uint16_t secs;            // Seconds elapsed
    uint16_t flags;           // Broadcast flags
    uint32_t ciaddr;          // Client IP (if known)
    uint32_t yiaddr;          // 'Your' assigned IP
    uint32_t siaddr;          // Next server IP (TFTP / boot server)
    uint32_t giaddr;          // Relay agent IP
    uint8_t  chaddr[16];      // Client hardware MAC address
    char     sname[64];       // Server hostname
    char     file[128];       // Boot file name
    uint8_t  vend[64];        // Vendor area / options
};
#pragma pack(pop)

static_assert(sizeof(BootpRawPacket) == 300, "BOOTP packet must be exactly 300 bytes according to RFC 951");

struct BootpPacket {
    uint8_t  op;
    uint8_t  htype;
    uint8_t  hlen;
    uint8_t  hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t  mac[6];
    std::string sname;
    std::string file;
    
    // Vendor options
    uint32_t subnet_mask;
    uint32_t gateway;
    uint32_t dns_server;
    std::string hostname;

    BootpPacket();
    
    std::vector<uint8_t> serialize() const;
    static bool deserialize(const uint8_t* data, size_t len, BootpPacket& out_pkt);
    
    static BootpPacket create_request(const uint8_t client_mac[6], uint32_t xid, const std::string& hostname = "client-node");
    static BootpPacket create_reply(const BootpPacket& request, uint32_t assigned_ip, uint32_t server_ip,
                                    uint32_t gateway_ip, uint32_t subnet, const std::string& boot_file = "pxelinux.0");

    std::string to_string() const;
};

} // namespace bdtp

