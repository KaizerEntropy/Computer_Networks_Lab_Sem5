#include "bootp.hpp"
#include <cstring>
#include <sstream>
#include <iomanip>

namespace bdtp {

BootpPacket::BootpPacket()
    : op(BOOTREQUEST), htype(HTYPE_ETHER), hlen(HLEN_ETHER), hops(0),
      xid(0), secs(0), flags(0), ciaddr(0), yiaddr(0), siaddr(0), giaddr(0),
      subnet_mask(0), gateway(0), dns_server(0) {
    std::memset(mac, 0, 6);
}

std::vector<uint8_t> BootpPacket::serialize() const {
    BootpRawPacket raw;
    std::memset(&raw, 0, sizeof(raw));

    raw.op = op;
    raw.htype = htype;
    raw.hlen = hlen;
    raw.hops = hops;
    raw.xid = htonl(xid);
    raw.secs = htons(secs);
    raw.flags = htons(flags);
    raw.ciaddr = ciaddr;
    raw.yiaddr = yiaddr;
    raw.siaddr = siaddr;
    raw.giaddr = giaddr;
    
    std::memcpy(raw.chaddr, mac, 6);
    std::strncpy(raw.sname, sname.c_str(), sizeof(raw.sname) - 1);
    std::strncpy(raw.file, file.c_str(), sizeof(raw.file) - 1);

    // Write vendor options with RFC 1048 magic cookie
    std::memcpy(raw.vend, BOOTP_MAGIC_COOKIE, 4);
    size_t offset = 4;

    if (subnet_mask != 0 && offset + 6 <= sizeof(raw.vend)) {
        raw.vend[offset++] = BOOTP_TAG_SUBNET_MASK;
        raw.vend[offset++] = 4;
        std::memcpy(&raw.vend[offset], &subnet_mask, 4);
        offset += 4;
    }

    if (gateway != 0 && offset + 6 <= sizeof(raw.vend)) {
        raw.vend[offset++] = BOOTP_TAG_GATEWAY;
        raw.vend[offset++] = 4;
        std::memcpy(&raw.vend[offset], &gateway, 4);
        offset += 4;
    }

    if (dns_server != 0 && offset + 6 <= sizeof(raw.vend)) {
        raw.vend[offset++] = BOOTP_TAG_DNS;
        raw.vend[offset++] = 4;
        std::memcpy(&raw.vend[offset], &dns_server, 4);
        offset += 4;
    }

    if (!hostname.empty() && offset + 2 + hostname.size() <= sizeof(raw.vend)) {
        raw.vend[offset++] = BOOTP_TAG_HOSTNAME;
        raw.vend[offset++] = static_cast<uint8_t>(hostname.size());
        std::memcpy(&raw.vend[offset], hostname.data(), hostname.size());
        offset += hostname.size();
    }

    if (offset < sizeof(raw.vend)) {
        raw.vend[offset++] = BOOTP_TAG_END;
    }

    const uint8_t* byte_ptr = reinterpret_cast<const uint8_t*>(&raw);
    return std::vector<uint8_t>(byte_ptr, byte_ptr + sizeof(raw));
}

bool BootpPacket::deserialize(const uint8_t* data, size_t len, BootpPacket& out_pkt) {
    if (len < sizeof(BootpRawPacket)) return false;

    const BootpRawPacket* raw = reinterpret_cast<const BootpRawPacket*>(data);
    out_pkt.op = raw->op;
    out_pkt.htype = raw->htype;
    out_pkt.hlen = raw->hlen;
    out_pkt.hops = raw->hops;
    out_pkt.xid = ntohl(raw->xid);
    out_pkt.secs = ntohs(raw->secs);
    out_pkt.flags = ntohs(raw->flags);
    out_pkt.ciaddr = raw->ciaddr;
    out_pkt.yiaddr = raw->yiaddr;
    out_pkt.siaddr = raw->siaddr;
    out_pkt.giaddr = raw->giaddr;
    
    std::memcpy(out_pkt.mac, raw->chaddr, 6);
    out_pkt.sname = std::string(raw->sname, strnlen(raw->sname, sizeof(raw->sname)));
    out_pkt.file = std::string(raw->file, strnlen(raw->file, sizeof(raw->file)));

    // Parse vendor options if magic cookie matches
    if (std::memcmp(raw->vend, BOOTP_MAGIC_COOKIE, 4) == 0) {
        size_t off = 4;
        while (off < sizeof(raw->vend)) {
            uint8_t tag = raw->vend[off++];
            if (tag == BOOTP_TAG_PAD) continue;
            if (tag == BOOTP_TAG_END) break;
            if (off >= sizeof(raw->vend)) break;
            
            uint8_t opt_len = raw->vend[off++];
            if (off + opt_len > sizeof(raw->vend)) break;

            if (tag == BOOTP_TAG_SUBNET_MASK && opt_len == 4) {
                std::memcpy(&out_pkt.subnet_mask, &raw->vend[off], 4);
            } else if (tag == BOOTP_TAG_GATEWAY && opt_len == 4) {
                std::memcpy(&out_pkt.gateway, &raw->vend[off], 4);
            } else if (tag == BOOTP_TAG_DNS && opt_len == 4) {
                std::memcpy(&out_pkt.dns_server, &raw->vend[off], 4);
            } else if (tag == BOOTP_TAG_HOSTNAME) {
                out_pkt.hostname = std::string(reinterpret_cast<const char*>(&raw->vend[off]), opt_len);
            }
            off += opt_len;
        }
    }

    return true;
}

BootpPacket BootpPacket::create_request(const uint8_t client_mac[6], uint32_t xid, const std::string& hostname) {
    BootpPacket pkt;
    pkt.op = BOOTREQUEST;
    pkt.xid = xid;
    pkt.secs = 0;
    pkt.flags = 0x8000; // Broadcast flag
    pkt.ciaddr = 0;     // No IP yet
    pkt.yiaddr = 0;
    pkt.siaddr = 0;
    pkt.giaddr = 0;
    std::memcpy(pkt.mac, client_mac, 6);
    pkt.hostname = hostname;
    pkt.file = "pxelinux.0";
    return pkt;
}

BootpPacket BootpPacket::create_reply(const BootpPacket& request, uint32_t assigned_ip, uint32_t server_ip,
                                     uint32_t gateway_ip, uint32_t subnet, const std::string& boot_file) {
    BootpPacket pkt;
    pkt.op = BOOTREPLY;
    pkt.xid = request.xid;
    pkt.secs = request.secs;
    pkt.flags = request.flags;
    pkt.ciaddr = request.ciaddr;
    pkt.yiaddr = assigned_ip;
    pkt.siaddr = server_ip;
    pkt.giaddr = request.giaddr;
    std::memcpy(pkt.mac, request.mac, 6);
    pkt.sname = "bootp-srv.campus.net";
    pkt.file = boot_file;
    pkt.subnet_mask = subnet;
    pkt.gateway = gateway_ip;
    pkt.dns_server = server_ip;
    pkt.hostname = request.hostname;
    return pkt;
}

std::string BootpPacket::to_string() const {
    std::ostringstream oss;
    oss << color::BOLD << (op == BOOTREQUEST ? "BOOTP REQUEST" : "BOOTP REPLY") << color::RESET << "\n"
        << "  - Opcode       : " << (op == BOOTREQUEST ? "1 (BOOTREQUEST)" : "2 (BOOTREPLY)") << "\n"
        << "  - Hardware     : Ethernet (MAC: " << mac_to_string(mac) << ")\n"
        << "  - Trans. ID    : 0x" << std::hex << std::uppercase << xid << std::dec << "\n"
        << "  - Client IP    : " << ip_to_string(ciaddr) << "\n"
        << "  - Assigned IP  : " << color::GREEN << ip_to_string(yiaddr) << color::RESET << "\n"
        << "  - Server IP    : " << ip_to_string(siaddr) << "\n"
        << "  - Gateway IP   : " << ip_to_string(giaddr) << "\n"
        << "  - Boot File    : " << color::CYAN << file << color::RESET << "\n"
        << "  - Server Name  : " << sname << "\n";
    if (subnet_mask != 0) oss << "  - Subnet Mask  : " << ip_to_string(subnet_mask) << "\n";
    if (gateway != 0)     oss << "  - Router DefGW : " << ip_to_string(gateway) << "\n";
    if (dns_server != 0)  oss << "  - DNS Server   : " << ip_to_string(dns_server) << "\n";
    return oss.str();
}

} // namespace bdtp

