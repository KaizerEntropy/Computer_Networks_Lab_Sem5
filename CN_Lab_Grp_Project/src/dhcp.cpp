#include "dhcp.hpp"
#include <cstring>
#include <sstream>
#include <iomanip>

namespace bdtp {

DhcpPacket::DhcpPacket()
    : op(BOOTREQUEST), htype(HTYPE_ETHER), hlen(HLEN_ETHER), hops(0),
      xid(0), secs(0), flags(0), ciaddr(0), yiaddr(0), siaddr(0), giaddr(0),
      msg_type(0), requested_ip(0), server_id(0), lease_time_secs(7200),
      renewal_time_t1(3600), rebinding_time_t2(6300), subnet_mask(0),
      router(0), dns_server(0) {
    std::memset(mac, 0, 6);
}

std::vector<uint8_t> DhcpPacket::serialize() const {
    std::vector<uint8_t> buf(236, 0);

    // 1. BOOTP header portion
    buf[0] = op;
    buf[1] = htype;
    buf[2] = hlen;
    buf[3] = hops;

    uint32_t net_xid = htonl(xid);
    std::memcpy(&buf[4], &net_xid, 4);

    uint16_t net_secs = htons(secs);
    std::memcpy(&buf[8], &net_secs, 2);

    uint16_t net_flags = htons(flags);
    std::memcpy(&buf[10], &net_flags, 2);

    std::memcpy(&buf[12], &ciaddr, 4);
    std::memcpy(&buf[16], &yiaddr, 4);
    std::memcpy(&buf[20], &siaddr, 4);
    std::memcpy(&buf[24], &giaddr, 4);

    std::memcpy(&buf[28], mac, 6);
    // bytes 34-43 zero padded hardware address

    if (!sname.empty()) {
        std::strncpy(reinterpret_cast<char*>(&buf[44]), sname.c_str(), 63);
    }
    if (!file.empty()) {
        std::strncpy(reinterpret_cast<char*>(&buf[108]), file.c_str(), 127);
    }

    // 2. DHCP Magic Cookie (0x63, 0x82, 0x53, 0x63)
    buf.insert(buf.end(), BOOTP_MAGIC_COOKIE, BOOTP_MAGIC_COOKIE + 4);

    // Helper lambda to append option
    auto add_option = [&buf](uint8_t tag, const void* data, uint8_t len) {
        buf.push_back(tag);
        buf.push_back(len);
        const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
        buf.insert(buf.end(), p, p + len);
    };

    // 3. DHCP Message Type (Option 53)
    if (msg_type != 0) {
        add_option(OPT_DHCP_MSG_TYPE, &msg_type, 1);
    }

    // Server Identifier (Option 54)
    if (server_id != 0) {
        add_option(OPT_SERVER_ID, &server_id, 4);
    }

    // Requested IP (Option 50)
    if (requested_ip != 0) {
        add_option(OPT_REQUESTED_IP, &requested_ip, 4);
    }

    // Lease Time (Option 51)
    if (lease_time_secs != 0) {
        uint32_t net_lease = htonl(lease_time_secs);
        add_option(OPT_LEASE_TIME, &net_lease, 4);
    }

    // Renewal Time T1 (Option 58)
    if (renewal_time_t1 != 0) {
        uint32_t net_t1 = htonl(renewal_time_t1);
        add_option(OPT_RENEWAL_TIME_T1, &net_t1, 4);
    }

    // Rebinding Time T2 (Option 59)
    if (rebinding_time_t2 != 0) {
        uint32_t net_t2 = htonl(rebinding_time_t2);
        add_option(OPT_REBINDING_TIME_T2, &net_t2, 4);
    }

    // Subnet Mask (Option 1)
    if (subnet_mask != 0) {
        add_option(OPT_SUBNET_MASK, &subnet_mask, 4);
    }

    // Router Gateway (Option 3)
    if (router != 0) {
        add_option(OPT_ROUTER, &router, 4);
    }

    // DNS Server (Option 6)
    if (dns_server != 0) {
        add_option(OPT_DNS_SERVER, &dns_server, 4);
    }

    // Host Name (Option 12)
    if (!host_name.empty()) {
        add_option(OPT_HOST_NAME, host_name.data(), static_cast<uint8_t>(host_name.size()));
    }

    // Domain Name (Option 15)
    if (!domain_name.empty()) {
        add_option(OPT_DOMAIN_NAME, domain_name.data(), static_cast<uint8_t>(domain_name.size()));
    }

    // Parameter Request List (Option 55)
    if (!param_req_list.empty()) {
        add_option(OPT_PARAM_REQUEST_LIST, param_req_list.data(), static_cast<uint8_t>(param_req_list.size()));
    }

    // Client Identifier (Option 61): 1 byte htype + 6 bytes MAC
    uint8_t client_id[7];
    client_id[0] = 1; // Ethernet
    std::memcpy(&client_id[1], mac, 6);
    add_option(OPT_CLIENT_ID, client_id, 7);

    // End Option (Option 255)
    buf.push_back(OPT_END);

    // Pad to minimum BOOTP size (300 bytes) if needed
    while (buf.size() < 300) {
        buf.push_back(OPT_PAD);
    }

    return buf;
}

bool DhcpPacket::deserialize(const uint8_t* data, size_t len, DhcpPacket& out_pkt) {
    if (len < 240) return false; // 236 header + 4 magic cookie

    out_pkt.op = data[0];
    out_pkt.htype = data[1];
    out_pkt.hlen = data[2];
    out_pkt.hops = data[3];

    uint32_t net_xid;
    std::memcpy(&net_xid, &data[4], 4);
    out_pkt.xid = ntohl(net_xid);

    uint16_t net_secs;
    std::memcpy(&net_secs, &data[8], 2);
    out_pkt.secs = ntohs(net_secs);

    uint16_t net_flags;
    std::memcpy(&net_flags, &data[10], 2);
    out_pkt.flags = ntohs(net_flags);

    std::memcpy(&out_pkt.ciaddr, &data[12], 4);
    std::memcpy(&out_pkt.yiaddr, &data[16], 4);
    std::memcpy(&out_pkt.siaddr, &data[20], 4);
    std::memcpy(&out_pkt.giaddr, &data[24], 4);
    std::memcpy(out_pkt.mac, &data[28], 6);

    out_pkt.sname = std::string(reinterpret_cast<const char*>(&data[44]), strnlen(reinterpret_cast<const char*>(&data[44]), 64));
    out_pkt.file  = std::string(reinterpret_cast<const char*>(&data[108]), strnlen(reinterpret_cast<const char*>(&data[108]), 128));

    // Verify Magic Cookie
    if (std::memcmp(&data[236], BOOTP_MAGIC_COOKIE, 4) != 0) {
        return false;
    }

    size_t off = 240;
    while (off < len) {
        uint8_t tag = data[off++];
        if (tag == OPT_PAD) continue;
        if (tag == OPT_END) break;
        if (off >= len) break;

        uint8_t opt_len = data[off++];
        if (off + opt_len > len) break;

        switch (tag) {
            case OPT_DHCP_MSG_TYPE:
                if (opt_len >= 1) out_pkt.msg_type = data[off];
                break;
            case OPT_SERVER_ID:
                if (opt_len >= 4) std::memcpy(&out_pkt.server_id, &data[off], 4);
                break;
            case OPT_REQUESTED_IP:
                if (opt_len >= 4) std::memcpy(&out_pkt.requested_ip, &data[off], 4);
                break;
            case OPT_LEASE_TIME:
                if (opt_len >= 4) {
                    uint32_t val;
                    std::memcpy(&val, &data[off], 4);
                    out_pkt.lease_time_secs = ntohl(val);
                }
                break;
            case OPT_RENEWAL_TIME_T1:
                if (opt_len >= 4) {
                    uint32_t val;
                    std::memcpy(&val, &data[off], 4);
                    out_pkt.renewal_time_t1 = ntohl(val);
                }
                break;
            case OPT_REBINDING_TIME_T2:
                if (opt_len >= 4) {
                    uint32_t val;
                    std::memcpy(&val, &data[off], 4);
                    out_pkt.rebinding_time_t2 = ntohl(val);
                }
                break;
            case OPT_SUBNET_MASK:
                if (opt_len >= 4) std::memcpy(&out_pkt.subnet_mask, &data[off], 4);
                break;
            case OPT_ROUTER:
                if (opt_len >= 4) std::memcpy(&out_pkt.router, &data[off], 4);
                break;
            case OPT_DNS_SERVER:
                if (opt_len >= 4) std::memcpy(&out_pkt.dns_server, &data[off], 4);
                break;
            case OPT_HOST_NAME:
                out_pkt.host_name = std::string(reinterpret_cast<const char*>(&data[off]), opt_len);
                break;
            case OPT_DOMAIN_NAME:
                out_pkt.domain_name = std::string(reinterpret_cast<const char*>(&data[off]), opt_len);
                break;
            case OPT_PARAM_REQUEST_LIST:
                out_pkt.param_req_list.assign(&data[off], &data[off + opt_len]);
                break;
            default:
                break;
        }

        off += opt_len;
    }

    return true;
}

DhcpPacket DhcpPacket::create_discover(const uint8_t client_mac[6], uint32_t xid, const std::string& host_name) {
    DhcpPacket pkt;
    pkt.op = BOOTREQUEST;
    pkt.xid = xid;
    pkt.flags = 0x8000; // Broadcast
    std::memcpy(pkt.mac, client_mac, 6);
    pkt.msg_type = DHCP_DISCOVER;
    pkt.host_name = host_name;
    pkt.param_req_list = {OPT_SUBNET_MASK, OPT_ROUTER, OPT_DNS_SERVER, OPT_DOMAIN_NAME, OPT_LEASE_TIME};
    return pkt;
}

DhcpPacket DhcpPacket::create_offer(const DhcpPacket& discover, uint32_t offered_ip, uint32_t server_ip,
                                   uint32_t mask, uint32_t gateway, uint32_t dns, uint32_t lease_secs) {
    DhcpPacket pkt;
    pkt.op = BOOTREPLY;
    pkt.xid = discover.xid;
    pkt.flags = discover.flags;
    pkt.yiaddr = offered_ip;
    pkt.siaddr = server_ip;
    std::memcpy(pkt.mac, discover.mac, 6);
    pkt.msg_type = DHCP_OFFER;
    pkt.server_id = server_ip;
    pkt.subnet_mask = mask;
    pkt.router = gateway;
    pkt.dns_server = dns;
    pkt.lease_time_secs = lease_secs;
    pkt.renewal_time_t1 = lease_secs / 2;
    pkt.rebinding_time_t2 = static_cast<uint32_t>(lease_secs * 0.875);
    pkt.domain_name = "lab.cse.ju";
    return pkt;
}

DhcpPacket DhcpPacket::create_request(const uint8_t client_mac[6], uint32_t xid, uint32_t req_ip,
                                     uint32_t server_ip, const std::string& host_name) {
    DhcpPacket pkt;
    pkt.op = BOOTREQUEST;
    pkt.xid = xid;
    pkt.flags = 0x8000;
    std::memcpy(pkt.mac, client_mac, 6);
    pkt.msg_type = DHCP_REQUEST;
    pkt.requested_ip = req_ip;
    pkt.server_id = server_ip;
    pkt.host_name = host_name;
    pkt.param_req_list = {OPT_SUBNET_MASK, OPT_ROUTER, OPT_DNS_SERVER, OPT_DOMAIN_NAME};
    return pkt;
}

DhcpPacket DhcpPacket::create_ack(const DhcpPacket& request, uint32_t assigned_ip, uint32_t server_ip,
                                 uint32_t mask, uint32_t gateway, uint32_t dns, uint32_t lease_secs) {
    DhcpPacket pkt;
    pkt.op = BOOTREPLY;
    pkt.xid = request.xid;
    pkt.flags = request.flags;
    pkt.yiaddr = assigned_ip;
    pkt.siaddr = server_ip;
    std::memcpy(pkt.mac, request.mac, 6);
    pkt.msg_type = DHCP_ACK;
    pkt.server_id = server_ip;
    pkt.subnet_mask = mask;
    pkt.router = gateway;
    pkt.dns_server = dns;
    pkt.lease_time_secs = lease_secs;
    pkt.renewal_time_t1 = lease_secs / 2;
    pkt.rebinding_time_t2 = static_cast<uint32_t>(lease_secs * 0.875);
    pkt.domain_name = "lab.cse.ju";
    return pkt;
}

DhcpPacket DhcpPacket::create_release(const uint8_t client_mac[6], uint32_t client_ip, uint32_t server_ip, uint32_t xid) {
    DhcpPacket pkt;
    pkt.op = BOOTREQUEST;
    pkt.xid = xid;
    pkt.ciaddr = client_ip;
    std::memcpy(pkt.mac, client_mac, 6);
    pkt.msg_type = DHCP_RELEASE;
    pkt.server_id = server_ip;
    return pkt;
}

std::string DhcpPacket::to_string() const {
    std::ostringstream oss;
    oss << color::BOLD << dhcp_msg_type_to_string(msg_type) << color::RESET << "\n"
        << "  - Client MAC   : " << mac_to_string(mac) << "\n"
        << "  - Trans. ID    : 0x" << std::hex << std::uppercase << xid << std::dec << "\n"
        << "  - Client IP    : " << ip_to_string(ciaddr) << "\n"
        << "  - Assigned/Off : " << color::GREEN << ip_to_string(yiaddr) << color::RESET << "\n"
        << "  - Server ID    : " << ip_to_string(server_id) << "\n";
    if (subnet_mask != 0)     oss << "  - Subnet Mask  : " << ip_to_string(subnet_mask) << "\n";
    if (router != 0)          oss << "  - Default GW   : " << ip_to_string(router) << "\n";
    if (dns_server != 0)       oss << "  - DNS Server   : " << ip_to_string(dns_server) << "\n";
    if (lease_time_secs != 0) oss << "  - Lease Time   : " << lease_time_secs << " s (T1="
                                  << renewal_time_t1 << "s, T2=" << rebinding_time_t2 << "s)\n";
    if (!host_name.empty())   oss << "  - Host Name    : " << host_name << "\n";
    if (!domain_name.empty()) oss << "  - Domain       : " << domain_name << "\n";
    return oss.str();
}

// ============================================================================
// DhcpLeasePool Implementation
// ============================================================================
DhcpLeasePool::DhcpLeasePool(uint32_t srv_ip, uint32_t mask, uint32_t gw, uint32_t dns,
                             uint32_t start_ip, uint32_t end_ip, uint32_t duration)
    : subnet_mask(mask), default_gateway(gw), dns_server(dns), server_ip(srv_ip),
      lease_duration_secs(duration), pool_start(ntohl(start_ip)), pool_end(ntohl(end_ip)) {}

uint32_t DhcpLeasePool::allocate_offer(const uint8_t mac[6], uint32_t requested_ip) {
    std::string mac_str = mac_to_string(mac);

    // 1. If client already has active lease, reuse it
    auto it = active_leases.find(mac_str);
    if (it != active_leases.end() && it->second.is_active) {
        return it->second.ip_address;
    }

    // 2. If client had a pending offer, reuse it
    auto offer_it = pending_offers.find(mac_str);
    if (offer_it != pending_offers.end()) {
        return offer_it->second;
    }

    // 3. If requested IP is valid and available
    uint32_t req_host_order = ntohl(requested_ip);
    if (req_host_order >= pool_start && req_host_order <= pool_end) {
        bool in_use = false;
        for (const auto& pair : active_leases) {
            if (pair.second.is_active && ntohl(pair.second.ip_address) == req_host_order) {
                in_use = true;
                break;
            }
        }
        if (!in_use) {
            pending_offers[mac_str] = requested_ip;
            return requested_ip;
        }
    }

    // 4. Find next available IP in pool
    for (uint32_t host_ip = pool_start; host_ip <= pool_end; ++host_ip) {
        uint32_t net_ip = htonl(host_ip);
        bool in_use = false;
        for (const auto& pair : active_leases) {
            if (pair.second.is_active && pair.second.ip_address == net_ip) {
                in_use = true;
                break;
            }
        }
        for (const auto& pair : pending_offers) {
            if (pair.second == net_ip) {
                in_use = true;
                break;
            }
        }
        if (!in_use) {
            pending_offers[mac_str] = net_ip;
            return net_ip;
        }
    }

    return 0; // Pool exhausted
}

bool DhcpLeasePool::commit_lease(const uint8_t mac[6], uint32_t ip, const std::string& host) {
    std::string mac_str = mac_to_string(mac);
    LeaseRecord rec;
    rec.mac_address = mac_str;
    rec.ip_address = ip;
    rec.lease_start_ms = now_ms();
    rec.lease_duration_secs = lease_duration_secs;
    rec.hostname = host;
    rec.is_active = true;

    active_leases[mac_str] = rec;
    pending_offers.erase(mac_str);
    return true;
}

void DhcpLeasePool::release_lease(const uint8_t mac[6], uint32_t ip) {
    (void)ip;
    std::string mac_str = mac_to_string(mac);
    auto it = active_leases.find(mac_str);
    if (it != active_leases.end()) {
        it->second.is_active = false;
    }
    pending_offers.erase(mac_str);
}

std::string DhcpLeasePool::print_active_leases() const {
    std::ostringstream oss;
    oss << color::BOLD_CYAN << "+-------------------+-----------------+---------------------+------------+\n"
        << "| MAC Address       | Assigned IP     | Hostname            | Status     |\n"
        << "+-------------------+-----------------+---------------------+------------+\n" << color::RESET;
    for (const auto& pair : active_leases) {
        oss << "| " << std::left << std::setw(17) << pair.second.mac_address
            << " | " << std::setw(15) << ip_to_string(pair.second.ip_address)
            << " | " << std::setw(19) << pair.second.hostname
            << " | " << (pair.second.is_active ? color::GREEN + "ACTIVE    " + color::RESET : color::RED + "RELEASED  " + color::RESET)
            << " |\n";
    }
    oss << color::BOLD_CYAN << "+-------------------+-----------------+---------------------+------------+\n" << color::RESET;
    return oss.str();
}

} // namespace bdtp

