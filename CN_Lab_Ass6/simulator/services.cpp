#include "services.hpp"
#include <iomanip>

namespace net {

// =============================================================================
// 1. DHCP SERVER IMPLEMENTATION
// =============================================================================
DHCPServer::DHCPServer(const std::string& name, const MacAddress& mac,
                       const IPv4Address& ip, const IPv4Address& mask,
                       const IPv4Address& gateway, const IPv4Address& dns,
                       const IPv4Address& pool_start, int pool_size)
    : Device(name), ip_(ip), mask_(mask), gateway_(gateway), dns_(dns),
      pool_start_(pool_start), pool_size_(pool_size) {
    Port* p = add_port("FastEthernet0", mac);
    p->ip = ip;
    p->mask = mask;
}

IPv4Address DHCPServer::allocate_ip(const MacAddress& mac) {
    auto it = active_leases_.find(mac);
    if (it != active_leases_.end()) {
        return it->second;
    }

    for (int i = 0; i < pool_size_; ++i) {
        uint32_t candidate = pool_start_.val + i;
        if (allocated_ips_.find(candidate) == allocated_ips_.end()) {
            allocated_ips_.insert(candidate);
            IPv4Address assigned(candidate);
            active_leases_[mac] = assigned;
            return assigned;
        }
    }
    return IPv4Address::any();
}

void DHCPServer::receive_frame(const EthernetFrame& frame, Port* in_port) {
    // ARP processing
    if (frame.ethertype == EtherType::ARP) {
        arp_table_.add_or_update(frame.arp.sender_ip, frame.arp.sender_mac);
        if (frame.arp.opcode == ARPOpcode::Request && frame.arp.target_ip == ip_) {
            EthernetFrame reply;
            reply.src_mac = in_port->mac;
            reply.dst_mac = frame.arp.sender_mac;
            reply.ethertype = EtherType::ARP;
            reply.arp.opcode = ARPOpcode::Reply;
            reply.arp.sender_mac = in_port->mac;
            reply.arp.sender_ip = ip_;
            reply.arp.target_mac = frame.arp.sender_mac;
            reply.arp.target_ip = frame.arp.sender_ip;
            in_port->send_frame(reply);
        }
        return;
    }

    if (frame.ethertype != EtherType::IPv4) return;

    const auto& ip_pkt = frame.ip;
    if (ip_pkt.protocol == IPProtocol::UDP && ip_pkt.udp.dst_port == 67) {
        const auto& dhcp = ip_pkt.dhcp;

        if (dhcp.msg_type == DHCPType::Discover) {
            discover_count++;
            offer_count++;

            IPv4Address offered_ip = allocate_ip(dhcp.chaddr);

            EthernetFrame offer_frame;
            offer_frame.src_mac = in_port->mac;
            offer_frame.dst_mac = MacAddress::broadcast(); // Standard DHCPOFFER
            offer_frame.ethertype = EtherType::IPv4;

            offer_frame.ip.src_ip = ip_;
            offer_frame.ip.dst_ip = IPv4Address::broadcast();
            offer_frame.ip.protocol = IPProtocol::UDP;
            offer_frame.ip.ttl = 64;
            offer_frame.ip.udp.src_port = 67;
            offer_frame.ip.udp.dst_port = 68;

            DHCPMessage& off = offer_frame.ip.dhcp;
            off.op = 2; // BootReply
            off.xid = dhcp.xid;
            off.ciaddr = IPv4Address::any();
            off.yiaddr = offered_ip;
            off.siaddr = ip_;
            off.chaddr = dhcp.chaddr;
            off.msg_type = DHCPType::Offer;
            off.subnet_mask = mask_;
            off.default_gateway = gateway_;
            off.dns_server = dns_;
            off.lease_time = 86400;

            if (verbose) {
                std::cout << " [DHCP Server " << name_ << "] Sent DHCPOFFER (" << offered_ip
                          << ") to " << dhcp.chaddr.to_cisco_string() << "\n";
            }
            in_port->send_frame(offer_frame);

        } else if (dhcp.msg_type == DHCPType::Request) {
            request_count++;
            ack_count++;

            IPv4Address ack_ip = allocate_ip(dhcp.chaddr);

            EthernetFrame ack_frame;
            ack_frame.src_mac = in_port->mac;
            ack_frame.dst_mac = MacAddress::broadcast();
            ack_frame.ethertype = EtherType::IPv4;

            ack_frame.ip.src_ip = ip_;
            ack_frame.ip.dst_ip = IPv4Address::broadcast();
            ack_frame.ip.protocol = IPProtocol::UDP;
            ack_frame.ip.ttl = 64;
            ack_frame.ip.udp.src_port = 67;
            ack_frame.ip.udp.dst_port = 68;

            DHCPMessage& ack = ack_frame.ip.dhcp;
            ack.op = 2;
            ack.xid = dhcp.xid;
            ack.ciaddr = dhcp.ciaddr;
            ack.yiaddr = ack_ip;
            ack.siaddr = ip_;
            ack.chaddr = dhcp.chaddr;
            ack.msg_type = DHCPType::Ack;
            ack.subnet_mask = mask_;
            ack.default_gateway = gateway_;
            ack.dns_server = dns_;
            ack.lease_time = 86400;

            if (verbose) {
                std::cout << " [DHCP Server " << name_ << "] Sent DHCPACK (" << ack_ip
                          << ") to " << dhcp.chaddr.to_cisco_string() << "\n";
            }
            in_port->send_frame(ack_frame);
        }
    }
}

void DHCPServer::show_leases(std::ostream& os) const {
    os << "------------------------------------------------------------\n";
    os << " DHCP Active Leases Pool (" << name_ << ")\n";
    os << "------------------------------------------------------------\n";
    os << " Client MAC           Assigned IP      Subnet Mask      Gateway\n";
    for (const auto& kv : active_leases_) {
        os << " " << std::left << std::setw(21) << kv.first.to_cisco_string()
           << std::setw(17) << kv.second.to_string()
           << std::setw(17) << mask_.to_string()
           << gateway_.to_string() << "\n";
    }
    os << "------------------------------------------------------------\n";
}

// =============================================================================
// 2. DNS SERVER IMPLEMENTATION
// =============================================================================
DNSServer::DNSServer(const std::string& name, const MacAddress& mac,
                     const IPv4Address& ip, const IPv4Address& mask,
                     const IPv4Address& gateway)
    : Device(name), ip_(ip), mask_(mask), gateway_(gateway) {
    Port* p = add_port("FastEthernet0", mac);
    p->ip = ip;
    p->mask = mask;
}

void DNSServer::add_a_record(const std::string& domain, const IPv4Address& addr, uint32_t ttl) {
    zone_database_[domain] = DNSRecord{domain, "A", ttl, addr};
}

void DNSServer::receive_frame(const EthernetFrame& frame, Port* in_port) {
    // ARP processing
    if (frame.ethertype == EtherType::ARP) {
        arp_table_.add_or_update(frame.arp.sender_ip, frame.arp.sender_mac);
        if (frame.arp.opcode == ARPOpcode::Request && frame.arp.target_ip == ip_) {
            EthernetFrame reply;
            reply.src_mac = in_port->mac;
            reply.dst_mac = frame.arp.sender_mac;
            reply.ethertype = EtherType::ARP;
            reply.arp.opcode = ARPOpcode::Reply;
            reply.arp.sender_mac = in_port->mac;
            reply.arp.sender_ip = ip_;
            reply.arp.target_mac = frame.arp.sender_mac;
            reply.arp.target_ip = frame.arp.sender_ip;
            in_port->send_frame(reply);
        }
        return;
    }

    if (frame.ethertype != EtherType::IPv4) return;
    const auto& ip_pkt = frame.ip;

    if (ip_pkt.dst_ip == ip_ && ip_pkt.protocol == IPProtocol::UDP && ip_pkt.udp.dst_port == 53) {
        query_count++;
        response_count++;

        const auto& dns_req = ip_pkt.dns;
        EthernetFrame dns_resp_frame;
        dns_resp_frame.src_mac = in_port->mac;
        dns_resp_frame.dst_mac = frame.src_mac;
        dns_resp_frame.ethertype = EtherType::IPv4;

        dns_resp_frame.ip.src_ip = ip_;
        dns_resp_frame.ip.dst_ip = ip_pkt.src_ip;
        dns_resp_frame.ip.protocol = IPProtocol::UDP;
        dns_resp_frame.ip.ttl = 64;
        dns_resp_frame.ip.udp.src_port = 53;
        dns_resp_frame.ip.udp.dst_port = ip_pkt.udp.src_port;

        DNSMessage& resp = dns_resp_frame.ip.dns;
        resp.id = dns_req.id;
        resp.is_response = true;
        resp.query_name = dns_req.query_name;
        resp.query_type = dns_req.query_type;

        auto it = zone_database_.find(dns_req.query_name);
        if (it != zone_database_.end()) {
            resp.answers.push_back(it->second);
        }

        if (verbose) {
            std::cout << " [DNS Server " << name_ << "] Query '" << dns_req.query_name
                      << "' resolved -> " << (resp.answers.empty() ? "NXDOMAIN" : resp.answers[0].address.to_string())
                      << "\n";
        }
        in_port->send_frame(dns_resp_frame);
    }
}

void DNSServer::show_records(std::ostream& os) const {
    os << "------------------------------------------------------------\n";
    os << " DNS Zone Database (" << name_ << ")\n";
    os << "------------------------------------------------------------\n";
    os << " Domain Name                  Type   TTL    IP Address\n";
    for (const auto& kv : zone_database_) {
        const auto& r = kv.second;
        os << " " << std::left << std::setw(29) << r.name
           << std::setw(7) << r.type
           << std::setw(7) << r.ttl
           << r.address.to_string() << "\n";
    }
    os << "------------------------------------------------------------\n";
}

// =============================================================================
// 3. WEB SERVER IMPLEMENTATION
// =============================================================================
WebServer::WebServer(const std::string& name, const MacAddress& mac,
                     const IPv4Address& ip, const IPv4Address& mask,
                     const IPv4Address& gateway)
    : Device(name), ip_(ip), mask_(mask), gateway_(gateway) {
    Port* p = add_port("FastEthernet0", mac);
    p->ip = ip;
    p->mask = mask;
}

void WebServer::set_html_page(const std::string& uri, const std::string& content) {
    pages_[uri] = content;
}

void WebServer::receive_frame(const EthernetFrame& frame, Port* in_port) {
    if (frame.ethertype == EtherType::ARP) {
        arp_table_.add_or_update(frame.arp.sender_ip, frame.arp.sender_mac);
        if (frame.arp.opcode == ARPOpcode::Request && frame.arp.target_ip == ip_) {
            EthernetFrame reply;
            reply.src_mac = in_port->mac;
            reply.dst_mac = frame.arp.sender_mac;
            reply.ethertype = EtherType::ARP;
            reply.arp.opcode = ARPOpcode::Reply;
            reply.arp.sender_mac = in_port->mac;
            reply.arp.sender_ip = ip_;
            reply.arp.target_mac = frame.arp.sender_mac;
            reply.arp.target_ip = frame.arp.sender_ip;
            in_port->send_frame(reply);
        }
        return;
    }

    if (frame.ethertype != EtherType::IPv4) return;
    const auto& ip_pkt = frame.ip;

    if (ip_pkt.dst_ip == ip_ && ip_pkt.protocol == IPProtocol::TCP) {
        const auto& http_req = ip_pkt.http;
        if (http_req.is_request && http_req.method == "GET") {
            get_request_count++;

            EthernetFrame http_resp;
            http_resp.src_mac = in_port->mac;
            http_resp.dst_mac = frame.src_mac;
            http_resp.ethertype = EtherType::IPv4;

            http_resp.ip.src_ip = ip_;
            http_resp.ip.dst_ip = ip_pkt.src_ip;
            http_resp.ip.protocol = IPProtocol::TCP;
            http_resp.ip.ttl = 64;

            HTTPMessage& resp = http_resp.ip.http;
            resp.is_request = false;
            resp.status_code = 200;
            resp.status_text = "OK";
            resp.headers["Content-Type"] = "text/html; charset=UTF-8";
            resp.headers["Server"] = "Cisco-PacketTracer-PT-Server/8.0";

            auto it = pages_.find(http_req.uri);
            if (it != pages_.end()) {
                resp.body = it->second;
            } else {
                resp.body = "<html><body><h1>200 OK</h1><p>Default Home Page</p></body></html>";
            }
            resp.headers["Content-Length"] = std::to_string(resp.body.size());

            if (verbose) {
                std::cout << " [Web Server " << name_ << "] Servicing HTTP GET " << http_req.uri
                          << " -> 200 OK (" << resp.body.size() << " bytes)\n";
            }
            in_port->send_frame(http_resp);
        }
    }
}

// =============================================================================
// 4. FTP SERVER IMPLEMENTATION
// =============================================================================
FTPServer::FTPServer(const std::string& name, const MacAddress& mac,
                     const IPv4Address& ip, const IPv4Address& mask,
                     const IPv4Address& gateway)
    : Device(name), ip_(ip), mask_(mask), gateway_(gateway) {
    Port* p = add_port("FastEthernet0", mac);
    p->ip = ip;
    p->mask = mask;
}

void FTPServer::add_user(const std::string& user, const std::string& pass) {
    users_[user] = pass;
}

void FTPServer::add_file(const std::string& filename, const std::string& content) {
    files_[filename] = content;
}

void FTPServer::receive_frame(const EthernetFrame& frame, Port* in_port) {
    if (frame.ethertype == EtherType::ARP) {
        arp_table_.add_or_update(frame.arp.sender_ip, frame.arp.sender_mac);
        if (frame.arp.opcode == ARPOpcode::Request && frame.arp.target_ip == ip_) {
            EthernetFrame reply;
            reply.src_mac = in_port->mac;
            reply.dst_mac = frame.arp.sender_mac;
            reply.ethertype = EtherType::ARP;
            reply.arp.opcode = ARPOpcode::Reply;
            reply.arp.sender_mac = in_port->mac;
            reply.arp.sender_ip = ip_;
            reply.arp.target_mac = frame.arp.sender_mac;
            reply.arp.target_ip = frame.arp.sender_ip;
            in_port->send_frame(reply);
        }
        return;
    }

    if (frame.ethertype != EtherType::IPv4) return;
    const auto& ip_pkt = frame.ip;

    if (ip_pkt.dst_ip == ip_ && ip_pkt.protocol == IPProtocol::TCP) {
        const auto& ftp_in = ip_pkt.ftp;
        if (ftp_in.is_command) {
            session_count++;

            EthernetFrame ftp_resp;
            ftp_resp.src_mac = in_port->mac;
            ftp_resp.dst_mac = frame.src_mac;
            ftp_resp.ethertype = EtherType::IPv4;

            ftp_resp.ip.src_ip = ip_;
            ftp_resp.ip.dst_ip = ip_pkt.src_ip;
            ftp_resp.ip.protocol = IPProtocol::TCP;
            ftp_resp.ip.ttl = 64;

            FTPMessage& resp = ftp_resp.ip.ftp;
            resp.is_command = false;

            if (ftp_in.cmd == "CONNECT") {
                resp.reply_code = 220;
                resp.reply_text = "Welcome to Jadavpur University CSE FTP Server (Version 1.0)";
            } else if (ftp_in.cmd == "USER") {
                auth_sessions_[ip_pkt.src_ip] = ftp_in.argument;
                resp.reply_code = 331;
                resp.reply_text = "Password required for " + ftp_in.argument;
            } else if (ftp_in.cmd == "PASS") {
                std::string u = auth_sessions_[ip_pkt.src_ip];
                if (users_.find(u) != users_.end() && users_[u] == ftp_in.argument) {
                    resp.reply_code = 230;
                    resp.reply_text = "User " + u + " logged in, proceed.";
                } else {
                    resp.reply_code = 530;
                    resp.reply_text = "Login incorrect.";
                }
            } else if (ftp_in.cmd == "LIST" || ftp_in.cmd == "DIR") {
                resp.reply_code = 150;
                resp.reply_text = "Opening ASCII mode data connection for file list.";
                std::ostringstream oss;
                for (const auto& f : files_) {
                    oss << "-rw-r--r-- 1 ftp ftp " << std::setw(6) << f.second.size()
                        << " Nov 06 00:00 " << f.first << "\n";
                }
                resp.data_payload = oss.str();
            } else if (ftp_in.cmd == "RETR" || ftp_in.cmd == "GET") {
                std::string fname = ftp_in.argument;
                if (files_.find(fname) != files_.end()) {
                    resp.reply_code = 226;
                    resp.reply_text = "Transfer complete.";
                    resp.data_payload = files_[fname];
                } else {
                    resp.reply_code = 550;
                    resp.reply_text = "Failed to open file: " + fname;
                }
            } else if (ftp_in.cmd == "QUIT") {
                resp.reply_code = 221;
                resp.reply_text = "Goodbye.";
                auth_sessions_.erase(ip_pkt.src_ip);
            }

            if (verbose) {
                std::cout << " [FTP Server " << name_ << "] Handled " << ftp_in.cmd
                          << " -> " << resp.reply_code << " " << resp.reply_text << "\n";
            }
            in_port->send_frame(ftp_resp);
        }
    }
}

} // namespace net

