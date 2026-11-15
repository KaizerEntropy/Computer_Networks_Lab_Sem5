#include "host.hpp"
#include <iomanip>
#include <chrono>

namespace net {

Host::Host(const std::string& name, const MacAddress& mac,
           const IPv4Address& ip, const IPv4Address& mask,
           const IPv4Address& gateway, const IPv4Address& dns)
    : Device(name), mac_(mac), ip_(ip), mask_(mask), gateway_(gateway), dns_(dns) {
    port_ = add_port("FastEthernet0", mac);
    port_->ip = ip;
    port_->mask = mask;
}

bool Host::resolve_arp(const IPv4Address& target_ip, MacAddress& out_mac) {
    if (arp_table_.lookup(target_ip, out_mac)) {
        return true;
    }

    // Broadcast ARP Request
    EthernetFrame arp_req;
    arp_req.src_mac = mac_;
    arp_req.dst_mac = MacAddress::broadcast();
    arp_req.ethertype = EtherType::ARP;

    arp_req.arp.opcode = ARPOpcode::Request;
    arp_req.arp.sender_mac = mac_;
    arp_req.arp.sender_ip = ip_;
    arp_req.arp.target_mac = MacAddress::zero();
    arp_req.arp.target_ip = target_ip;

    if (verbose) {
        std::cout << " [" << name_ << "] Broadcasting ARP Request: Who has " << target_ip
                  << "? Tell " << ip_ << "\n";
    }
    port_->send_frame(arp_req);

    return arp_table_.lookup(target_ip, out_mac);
}

void Host::receive_frame(const EthernetFrame& frame, Port* /*in_port*/) {
    // Layer 2 filtering: Must match host MAC or broadcast
    if (frame.dst_mac != mac_ && !frame.dst_mac.is_broadcast()) {
        return;
    }

    // 1. ARP Handling
    if (frame.ethertype == EtherType::ARP) {
        arp_table_.add_or_update(frame.arp.sender_ip, frame.arp.sender_mac);

        if (frame.arp.opcode == ARPOpcode::Request) {
            if (frame.arp.target_ip == ip_ && !ip_.is_zero()) {
                // Generate ARP Reply
                EthernetFrame reply;
                reply.src_mac = mac_;
                reply.dst_mac = frame.arp.sender_mac;
                reply.ethertype = EtherType::ARP;

                reply.arp.opcode = ARPOpcode::Reply;
                reply.arp.sender_mac = mac_;
                reply.arp.sender_ip = ip_;
                reply.arp.target_mac = frame.arp.sender_mac;
                reply.arp.target_ip = frame.arp.sender_ip;

                if (verbose) {
                    std::cout << " [" << name_ << "] Sending ARP Reply to " << frame.arp.sender_ip
                              << " (" << mac_.to_cisco_string() << ")\n";
                }
                port_->send_frame(reply);
            }
        }
        return;
    }

    // 2. IPv4 Handling
    if (frame.ethertype == EtherType::IPv4) {
        if (!frame.ip.src_ip.is_zero()) {
            arp_table_.add_or_update(frame.ip.src_ip, frame.src_mac);
        }

        // ICMP
        if (frame.ip.protocol == IPProtocol::ICMP) {
            if (frame.ip.icmp.type == ICMPType::EchoRequest && (frame.ip.dst_ip == ip_ || frame.dst_mac == mac_)) {
                // Send ICMP Echo Reply
                EthernetFrame echo_rep;
                echo_rep.src_mac = mac_;
                echo_rep.dst_mac = frame.src_mac;
                echo_rep.ethertype = EtherType::IPv4;

                echo_rep.ip.src_ip = ip_;
                echo_rep.ip.dst_ip = frame.ip.src_ip;
                echo_rep.ip.protocol = IPProtocol::ICMP;
                echo_rep.ip.ttl = 128; // Standard Windows / Host TTL

                echo_rep.ip.icmp.type = ICMPType::EchoReply;
                echo_rep.ip.icmp.code = 0;
                echo_rep.ip.icmp.identifier = frame.ip.icmp.identifier;
                echo_rep.ip.icmp.sequence_num = frame.ip.icmp.sequence_num;
                echo_rep.ip.icmp.payload = frame.ip.icmp.payload;

                if (verbose) {
                    std::cout << " [" << name_ << "] ICMP Echo Request received from "
                              << frame.ip.src_ip << "; sending Echo Reply.\n";
                }
                port_->send_frame(echo_rep);

            } else if (frame.ip.icmp.type == ICMPType::EchoReply) {
                icmp_reply_received_ = true;
                last_icmp_seq_ = frame.ip.icmp.sequence_num;
            }
        }
        // UDP (DHCP & DNS)
        else if (frame.ip.protocol == IPProtocol::UDP) {
            if (frame.ip.udp.dst_port == 68) { // DHCP Client
                if (frame.ip.dhcp.msg_type == DHCPType::Offer) {
                    dhcp_offer_received_ = true;
                    last_dhcp_offer_ = frame.ip.dhcp;
                } else if (frame.ip.dhcp.msg_type == DHCPType::Ack) {
                    dhcp_ack_received_ = true;
                    last_dhcp_ack_ = frame.ip.dhcp;
                }
            } else if (frame.ip.udp.dst_port == 53535 || frame.ip.udp.src_port == 53) { // DNS Client
                dns_reply_received_ = true;
                last_dns_reply_ = frame.ip.dns;
            }
        }
        // TCP (HTTP & FTP)
        else if (frame.ip.protocol == IPProtocol::TCP) {
            if (!frame.ip.http.is_request) {
                http_reply_received_ = true;
                last_http_reply_ = frame.ip.http;
            }
            if (!frame.ip.ftp.is_command) {
                ftp_reply_received_ = true;
                last_ftp_reply_ = frame.ip.ftp;
            }
        }
    }
}

PingStats Host::ping(const IPv4Address& target, int count, std::ostream& os) {
    os << "\nPinging " << target << " with 32 bytes of data:\n";
    PingStats stats;
    stats.sent = count;

    IPv4Address next_hop_ip;
    if (ip_.in_same_subnet(target, mask_)) {
        next_hop_ip = target;
    } else {
        if (gateway_.is_zero()) {
            for (int i = 0; i < count; ++i) {
                os << "Destination host unreachable.\n";
            }
            stats.lost = count;
            return stats;
        }
        next_hop_ip = gateway_;
    }

    MacAddress dst_mac;
    bool arp_resolved = resolve_arp(next_hop_ip, dst_mac);

    for (int seq = 1; seq <= count; ++seq) {
        icmp_reply_received_ = false;

        if (!arp_resolved) {
            arp_resolved = resolve_arp(next_hop_ip, dst_mac);
        }

        if (!arp_resolved) {
            os << "Request timed out.\n";
            stats.lost++;
            continue;
        }

        // Send ICMP Echo Request
        EthernetFrame frame;
        frame.src_mac = mac_;
        frame.dst_mac = dst_mac;
        frame.ethertype = EtherType::IPv4;

        frame.ip.src_ip = ip_;
        frame.ip.dst_ip = target;
        frame.ip.protocol = IPProtocol::ICMP;
        frame.ip.ttl = 128;

        frame.ip.icmp.type = ICMPType::EchoRequest;
        frame.ip.icmp.code = 0;
        frame.ip.icmp.identifier = 1;
        frame.ip.icmp.sequence_num = static_cast<uint16_t>(seq);
        frame.ip.icmp.payload = "abcdefghijklmnopqrstuvwabcdefghi";

        port_->send_frame(frame);

        if (icmp_reply_received_) {
            stats.received++;
            // Calculate simulated realistic RTT
            double rtt = ip_.in_same_subnet(target, mask_) ? 0.8 : 10.4;
            if (seq == 1 && stats.received == 1) rtt += 2.1;
            stats.min_rtt = (stats.received == 1) ? rtt : std::min(stats.min_rtt, rtt);
            stats.max_rtt = (stats.received == 1) ? rtt : std::max(stats.max_rtt, rtt);
            stats.avg_rtt += rtt;

            os << "Reply from " << target << ": bytes=32 time="
               << (rtt < 1.0 ? "<1ms" : std::to_string(static_cast<int>(rtt)) + "ms")
               << " TTL=" << (ip_.in_same_subnet(target, mask_) ? 128 : 126) << "\n";
        } else {
            stats.lost++;
            os << "Request timed out.\n";
        }
    }

    if (stats.received > 0) {
        stats.avg_rtt /= stats.received;
    }

    int loss_pct = static_cast<int>((stats.lost * 100.0) / stats.sent);
    os << "\nPing statistics for " << target << ":\n"
       << "    Packets: Sent = " << stats.sent << ", Received = " << stats.received
       << ", Lost = " << stats.lost << " (" << loss_pct << "% loss),\n";
    if (stats.received > 0) {
        os << "Approximate round trip times in milli-seconds:\n"
           << "    Minimum = " << static_cast<int>(stats.min_rtt) << "ms, Maximum = "
           << static_cast<int>(stats.max_rtt) << "ms, Average = "
           << static_cast<int>(stats.avg_rtt) << "ms\n";
    }

    return stats;
}

bool Host::request_dhcp(std::ostream& os) {
    os << "\n[" << name_ << "] Requesting dynamic IP configuration via DHCP...\n";
    dhcp_offer_received_ = false;
    dhcp_ack_received_ = false;

    // 1. Send DHCPDISCOVER
    EthernetFrame disc_frame;
    disc_frame.src_mac = mac_;
    disc_frame.dst_mac = MacAddress::broadcast();
    disc_frame.ethertype = EtherType::IPv4;

    disc_frame.ip.src_ip = IPv4Address::any();
    disc_frame.ip.dst_ip = IPv4Address::broadcast();
    disc_frame.ip.protocol = IPProtocol::UDP;
    disc_frame.ip.ttl = 64;
    disc_frame.ip.udp.src_port = 68;
    disc_frame.ip.udp.dst_port = 67;

    DHCPMessage& disc = disc_frame.ip.dhcp;
    disc.op = 1; // BootRequest
    disc.xid = 0x3914AC11;
    disc.ciaddr = IPv4Address::any();
    disc.chaddr = mac_;
    disc.msg_type = DHCPType::Discover;

    os << " > Sent DHCPDISCOVER (broadcast 255.255.255.255:67)\n";
    port_->send_frame(disc_frame);

    if (!dhcp_offer_received_) {
        os << " [!] DHCP failed: No DHCPOFFER received.\n";
        return false;
    }
    os << " < Received DHCPOFFER from " << last_dhcp_offer_.siaddr << " offering IP "
       << last_dhcp_offer_.yiaddr << "\n";

    // 2. Send DHCPREQUEST
    EthernetFrame req_frame;
    req_frame.src_mac = mac_;
    req_frame.dst_mac = MacAddress::broadcast();
    req_frame.ethertype = EtherType::IPv4;

    req_frame.ip.src_ip = IPv4Address::any();
    req_frame.ip.dst_ip = IPv4Address::broadcast();
    req_frame.ip.protocol = IPProtocol::UDP;
    req_frame.ip.ttl = 64;
    req_frame.ip.udp.src_port = 68;
    req_frame.ip.udp.dst_port = 67;

    DHCPMessage& req = req_frame.ip.dhcp;
    req.op = 1;
    req.xid = disc.xid;
    req.ciaddr = IPv4Address::any();
    req.yiaddr = last_dhcp_offer_.yiaddr;
    req.siaddr = last_dhcp_offer_.siaddr;
    req.chaddr = mac_;
    req.msg_type = DHCPType::Request;

    os << " > Sent DHCPREQUEST for offered IP " << req.yiaddr << "\n";
    port_->send_frame(req_frame);

    if (!dhcp_ack_received_) {
        os << " [!] DHCP failed: No DHCPACK received.\n";
        return false;
    }

    // Apply configuration
    set_ip(last_dhcp_ack_.yiaddr);
    set_mask(last_dhcp_ack_.subnet_mask);
    set_gateway(last_dhcp_ack_.default_gateway);
    set_dns(last_dhcp_ack_.dns_server);
    dhcp_enabled = true;

    os << " < Received DHCPACK from " << last_dhcp_ack_.siaddr << "\n";
    os << " [OK] DHCP request successful.\n";
    os << "      IP Address:      " << ip_ << "\n"
       << "      Subnet Mask:     " << mask_ << "\n"
       << "      Default Gateway: " << gateway_ << "\n"
       << "      DNS Server:      " << dns_ << "\n";
    return true;
}

IPv4Address Host::nslookup(const std::string& domain, std::ostream& os) {
    os << "\n" << name_ << "> nslookup " << domain << "\n";
    if (dns_.is_zero()) {
        os << "*** Can't find server address: No DNS server configured.\n";
        return IPv4Address::any();
    }

    IPv4Address next_hop = ip_.in_same_subnet(dns_, mask_) ? dns_ : gateway_;
    MacAddress dst_mac;
    if (!resolve_arp(next_hop, dst_mac)) {
        os << "*** DNS server " << dns_ << " is unreachable.\n";
        return IPv4Address::any();
    }

    dns_reply_received_ = false;

    EthernetFrame dns_frame;
    dns_frame.src_mac = mac_;
    dns_frame.dst_mac = dst_mac;
    dns_frame.ethertype = EtherType::IPv4;

    dns_frame.ip.src_ip = ip_;
    dns_frame.ip.dst_ip = dns_;
    dns_frame.ip.protocol = IPProtocol::UDP;
    dns_frame.ip.ttl = 128;
    dns_frame.ip.udp.src_port = 53535;
    dns_frame.ip.udp.dst_port = 53;

    DNSMessage& query = dns_frame.ip.dns;
    query.id = 0x1A2B;
    query.is_response = false;
    query.query_name = domain;
    query.query_type = "A";

    port_->send_frame(dns_frame);

    if (dns_reply_received_ && !last_dns_reply_.answers.empty()) {
        IPv4Address resolved_ip = last_dns_reply_.answers[0].address;
        os << "Server:  dns." << domain << "\n"
           << "Address: " << dns_ << "\n\n"
           << "Non-authoritative answer:\n"
           << "Name:    " << domain << "\n"
           << "Address: " << resolved_ip << "\n";
        return resolved_ip;
    }

    os << "*** " << dns_ << " can't find " << domain << ": Non-existent domain\n";
    return IPv4Address::any();
}

std::string Host::http_get(const std::string& url_or_domain, std::ostream& os) {
    os << "\n" << name_ << "> curl http://" << url_or_domain << "/\n";

    IPv4Address server_ip;
    // Check if url is already dotted IP
    if (isdigit(url_or_domain[0])) {
        server_ip = IPv4Address(url_or_domain);
    } else {
        server_ip = nslookup(url_or_domain, os);
        if (server_ip.is_zero()) {
            os << "curl: (6) Could not resolve host: " << url_or_domain << "\n";
            return "";
        }
    }

    IPv4Address next_hop = ip_.in_same_subnet(server_ip, mask_) ? server_ip : gateway_;
    MacAddress dst_mac;
    if (!resolve_arp(next_hop, dst_mac)) {
        os << "curl: (7) Failed to connect to " << server_ip << " port 80: Connection timed out\n";
        return "";
    }

    http_reply_received_ = false;

    EthernetFrame http_frame;
    http_frame.src_mac = mac_;
    http_frame.dst_mac = dst_mac;
    http_frame.ethertype = EtherType::IPv4;

    http_frame.ip.src_ip = ip_;
    http_frame.ip.dst_ip = server_ip;
    http_frame.ip.protocol = IPProtocol::TCP;
    http_frame.ip.ttl = 128;

    HTTPMessage& req = http_frame.ip.http;
    req.is_request = true;
    req.method = "GET";
    req.uri = "/";
    req.headers["Host"] = url_or_domain;
    req.headers["User-Agent"] = "CiscoPacketTracer/BrowserClient";

    port_->send_frame(http_frame);

    if (http_reply_received_) {
        os << "HTTP/1.1 " << last_http_reply_.status_code << " " << last_http_reply_.status_text << "\n";
        for (const auto& h : last_http_reply_.headers) {
            os << h.first << ": " << h.second << "\n";
        }
        os << "\n" << last_http_reply_.body << "\n";
        return last_http_reply_.body;
    }

    os << "curl: (52) Empty reply from server\n";
    return "";
}

bool Host::ftp_session(const std::string& host_or_domain, const std::string& user,
                       const std::string& pass, const std::string& get_file,
                       std::ostream& os) {
    os << "\n" << name_ << "> ftp " << host_or_domain << "\n";

    IPv4Address server_ip;
    if (isdigit(host_or_domain[0])) {
        server_ip = IPv4Address(host_or_domain);
    } else {
        server_ip = nslookup(host_or_domain, os);
        if (server_ip.is_zero()) {
            os << "ftp: Unknown host " << host_or_domain << "\n";
            return false;
        }
    }

    IPv4Address next_hop = ip_.in_same_subnet(server_ip, mask_) ? server_ip : gateway_;
    MacAddress dst_mac;
    if (!resolve_arp(next_hop, dst_mac)) {
        os << "ftp: connect to address " << server_ip << ": Connection timed out\n";
        return false;
    }

    auto send_cmd = [&](const std::string& cmd, const std::string& arg) -> FTPMessage {
        ftp_reply_received_ = false;
        EthernetFrame frame;
        frame.src_mac = mac_;
        frame.dst_mac = dst_mac;
        frame.ethertype = EtherType::IPv4;

        frame.ip.src_ip = ip_;
        frame.ip.dst_ip = server_ip;
        frame.ip.protocol = IPProtocol::TCP;
        frame.ip.ttl = 128;

        frame.ip.ftp.is_command = true;
        frame.ip.ftp.cmd = cmd;
        frame.ip.ftp.argument = arg;

        port_->send_frame(frame);
        return last_ftp_reply_;
    };

    // 1. Connect
    os << "Connected to " << host_or_domain << ".\n";
    FTPMessage r = send_cmd("CONNECT", "");
    os << r.reply_code << " " << r.reply_text << "\n";

    // 2. User
    os << "User (" << host_or_domain << ":(none)): " << user << "\n";
    r = send_cmd("USER", user);
    os << r.reply_code << " " << r.reply_text << "\n";

    // 3. Password
    os << "Password: ******\n";
    r = send_cmd("PASS", pass);
    os << r.reply_code << " " << r.reply_text << "\n";
    if (r.reply_code != 230) {
        os << "Login failed.\n";
        return false;
    }

    // 4. Directory listing
    os << "ftp> dir\n";
    r = send_cmd("LIST", "");
    os << r.reply_code << " " << r.reply_text << "\n";
    os << r.data_payload;
    os << "226 Transfer complete.\n";

    // 5. Get file
    if (!get_file.empty()) {
        os << "ftp> get " << get_file << "\n";
        r = send_cmd("RETR", get_file);
        if (r.reply_code == 226) {
            os << "200 PORT command successful.\n"
               << "150 Opening BINARY mode data connection for " << get_file
               << " (" << r.data_payload.size() << " bytes).\n"
               << "226 Transfer complete.\n"
               << "ftp: " << r.data_payload.size() << " bytes received in 0.05 seconds.\n";
        } else {
            os << r.reply_code << " " << r.reply_text << "\n";
        }
    }

    // 6. Quit
    os << "ftp> quit\n";
    r = send_cmd("QUIT", "");
    os << r.reply_code << " " << r.reply_text << "\n";

    return true;
}

void Host::ipconfig(bool all, std::ostream& os) const {
    os << "\nFastEthernet0 Connection:\n";
    if (all) {
        os << "   Connection-specific DNS Suffix..: \n"
           << "   Physical Address................: " << mac_.to_cisco_string() << "\n"
           << "   DHCP Enabled....................: " << (dhcp_enabled ? "Yes" : "No") << "\n";
    }
    os << "   IP Address......................: " << ip_ << "\n"
       << "   Subnet Mask.....................: " << mask_ << "\n"
       << "   Default Gateway.................: " << gateway_ << "\n";
    if (all && !dns_.is_zero()) {
        os << "   DNS Servers.....................: " << dns_ << "\n";
    }
}

void Host::show_arp(std::ostream& os) const {
    arp_table_.print_table(os, "ARP Cache on " + name_);
}

void Host::flush_arp() {
    arp_table_.flush();
}

} // namespace net

