#pragma once

#include "common.hpp"
#include "bootp.hpp"
#include <string>
#include <vector>
#include <map>

namespace bdtp {

// DHCP Message Types (RFC 2132 Option 53)
enum DhcpMessageType : uint8_t {
    DHCP_DISCOVER = 1,
    DHCP_OFFER    = 2,
    DHCP_REQUEST  = 3,
    DHCP_DECLINE  = 4,
    DHCP_ACK      = 5,
    DHCP_NAK      = 6,
    DHCP_RELEASE  = 7,
    DHCP_INFORM   = 8
};

inline std::string dhcp_msg_type_to_string(uint8_t type) {
    switch (type) {
        case DHCP_DISCOVER: return "DHCPDISCOVER";
        case DHCP_OFFER:    return "DHCPOFFER";
        case DHCP_REQUEST:  return "DHCPREQUEST";
        case DHCP_DECLINE:  return "DHCPDECLINE";
        case DHCP_ACK:      return "DHCPACK";
        case DHCP_NAK:      return "DHCPNAK";
        case DHCP_RELEASE:  return "DHCPRELEASE";
        case DHCP_INFORM:   return "DHCPINFORM";
        default:            return "UNKNOWN(" + std::to_string(type) + ")";
    }
}

// Option Tags
constexpr uint8_t OPT_PAD                 = 0;
constexpr uint8_t OPT_SUBNET_MASK         = 1;
constexpr uint8_t OPT_ROUTER              = 3;
constexpr uint8_t OPT_DNS_SERVER          = 6;
constexpr uint8_t OPT_HOST_NAME           = 12;
constexpr uint8_t OPT_DOMAIN_NAME         = 15;
constexpr uint8_t OPT_REQUESTED_IP        = 50;
constexpr uint8_t OPT_LEASE_TIME          = 51;
constexpr uint8_t OPT_DHCP_MSG_TYPE       = 53;
constexpr uint8_t OPT_SERVER_ID           = 54;
constexpr uint8_t OPT_PARAM_REQUEST_LIST  = 55;
constexpr uint8_t OPT_RENEWAL_TIME_T1     = 58;
constexpr uint8_t OPT_REBINDING_TIME_T2   = 59;
constexpr uint8_t OPT_CLIENT_ID           = 61;
constexpr uint8_t OPT_END                 = 255;

struct DhcpOption {
    uint8_t tag;
    std::vector<uint8_t> data;
};

struct DhcpPacket {
    uint8_t  op;              // 1=BOOTREQUEST (client), 2=BOOTREPLY (server)
    uint8_t  htype;           // 1=Ethernet
    uint8_t  hlen;            // 6
    uint8_t  hops;            // 0
    uint32_t xid;             // Transaction ID
    uint16_t secs;            // Seconds
    uint16_t flags;           // Broadcast bit
    uint32_t ciaddr;          // Client IP (if bound)
    uint32_t yiaddr;          // 'Your' IP (assigned/offered)
    uint32_t siaddr;          // Next server IP
    uint32_t giaddr;          // Relay agent IP
    uint8_t  mac[6];          // Client MAC
    std::string sname;
    std::string file;

    // DHCP Specific Parsed Fields
    uint8_t  msg_type;
    uint32_t requested_ip;
    uint32_t server_id;
    uint32_t lease_time_secs;
    uint32_t renewal_time_t1;
    uint32_t rebinding_time_t2;
    uint32_t subnet_mask;
    uint32_t router;
    uint32_t dns_server;
    std::string host_name;
    std::string domain_name;
    std::vector<uint8_t> param_req_list;

    DhcpPacket();

    std::vector<uint8_t> serialize() const;
    static bool deserialize(const uint8_t* data, size_t len, DhcpPacket& out_pkt);

    // Factory methods for DORA protocol stages
    static DhcpPacket create_discover(const uint8_t client_mac[6], uint32_t xid, const std::string& host_name);
    static DhcpPacket create_offer(const DhcpPacket& discover, uint32_t offered_ip, uint32_t server_ip,
                                   uint32_t mask, uint32_t gateway, uint32_t dns, uint32_t lease_secs);
    static DhcpPacket create_request(const uint8_t client_mac[6], uint32_t xid, uint32_t req_ip,
                                     uint32_t server_ip, const std::string& host_name);
    static DhcpPacket create_ack(const DhcpPacket& request, uint32_t assigned_ip, uint32_t server_ip,
                                 uint32_t mask, uint32_t gateway, uint32_t dns, uint32_t lease_secs);
    static DhcpPacket create_release(const uint8_t client_mac[6], uint32_t client_ip, uint32_t server_ip, uint32_t xid);

    std::string to_string() const;
};

// ============================================================================
// DHCP Server Lease Pool Management
// ============================================================================
struct LeaseRecord {
    std::string mac_address;
    uint32_t ip_address;
    double lease_start_ms;
    uint32_t lease_duration_secs;
    std::string hostname;
    bool is_active;
};

class DhcpLeasePool {
public:
    uint32_t subnet_mask;
    uint32_t default_gateway;
    uint32_t dns_server;
    uint32_t server_ip;
    uint32_t lease_duration_secs;

    DhcpLeasePool(uint32_t srv_ip, uint32_t mask, uint32_t gw, uint32_t dns,
                  uint32_t start_ip, uint32_t end_ip, uint32_t duration = 7200);

    uint32_t allocate_offer(const uint8_t mac[6], uint32_t requested_ip = 0);
    bool commit_lease(const uint8_t mac[6], uint32_t ip, const std::string& host);
    void release_lease(const uint8_t mac[6], uint32_t ip);
    std::string print_active_leases() const;

private:
    uint32_t pool_start;
    uint32_t pool_end;
    std::map<std::string, LeaseRecord> active_leases; // MAC -> Record
    std::map<std::string, uint32_t> pending_offers;    // MAC -> IP
};

} // namespace bdtp
