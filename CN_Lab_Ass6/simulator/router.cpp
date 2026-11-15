#include "router.hpp"
#include <iomanip>

namespace net {

Router::Router(const std::string& name) : Device(name) {}

Port* Router::configure_interface(const std::string& name, const IPv4Address& ip,
                                 const IPv4Address& mask, const MacAddress& mac) {
    Port* p = add_port(name, mac);
    p->ip = ip;
    p->mask = mask;
    // Automatically add connected route
    routing_table_.add_connected(ip.network_address(mask), mask, name);
    return p;
}

void Router::add_static_route(const IPv4Address& net, const IPv4Address& mask, const IPv4Address& next_hop) {
    routing_table_.add_static(net, mask, next_hop);
}

void Router::send_arp_request(Port* out_port, const IPv4Address& target_ip) {
    EthernetFrame frame;
    frame.src_mac = out_port->mac;
    frame.dst_mac = MacAddress::broadcast();
    frame.ethertype = EtherType::ARP;

    frame.arp.opcode = ARPOpcode::Request;
    frame.arp.sender_mac = out_port->mac;
    frame.arp.sender_ip = out_port->ip;
    frame.arp.target_mac = MacAddress::zero();
    frame.arp.target_ip = target_ip;

    if (verbose) {
        std::cout << " [Router " << name_ << "] Broadcasting ARP request for " << target_ip
                  << " out of " << out_port->name << ".\n";
    }
    out_port->send_frame(frame);
}

void Router::receive_frame(const EthernetFrame& frame, Port* in_port) {
    // Check Layer 2 MAC filtering: Must be addressed to interface MAC or broadcast
    if (frame.dst_mac != in_port->mac && !frame.dst_mac.is_broadcast()) {
        return;
    }

    // 1. Handle ARP
    if (frame.ethertype == EtherType::ARP) {
        arp_table_.add_or_update(frame.arp.sender_ip, frame.arp.sender_mac);

        if (frame.arp.opcode == ARPOpcode::Request) {
            if (frame.arp.target_ip == in_port->ip) {
                // Generate ARP Reply
                EthernetFrame reply;
                reply.src_mac = in_port->mac;
                reply.dst_mac = frame.arp.sender_mac;
                reply.ethertype = EtherType::ARP;

                reply.arp.opcode = ARPOpcode::Reply;
                reply.arp.sender_mac = in_port->mac;
                reply.arp.sender_ip = in_port->ip;
                reply.arp.target_mac = frame.arp.sender_mac;
                reply.arp.target_ip = frame.arp.sender_ip;

                if (verbose) {
                    std::cout << " [Router " << name_ << "] Answering ARP request for "
                              << in_port->ip << " to " << frame.arp.sender_ip << ".\n";
                }
                in_port->send_frame(reply);
            }
        }
        return;
    }

    // 2. Handle IPv4
    if (frame.ethertype == EtherType::IPv4) {
        arp_table_.add_or_update(frame.ip.src_ip, frame.src_mac);

        // Check if addressed to one of router's interfaces
        bool targeted_to_router = false;
        for (const auto& kv : ports_) {
            if (kv.second->ip == frame.ip.dst_ip) {
                targeted_to_router = true;
                break;
            }
        }

        if (targeted_to_router) {
            // Process local packet (e.g. ICMP Echo Request)
            if (frame.ip.protocol == IPProtocol::ICMP && frame.ip.icmp.type == ICMPType::EchoRequest) {
                EthernetFrame echo_reply;
                echo_reply.src_mac = in_port->mac;
                echo_reply.dst_mac = frame.src_mac;
                echo_reply.ethertype = EtherType::IPv4;

                echo_reply.ip.src_ip = frame.ip.dst_ip;
                echo_reply.ip.dst_ip = frame.ip.src_ip;
                echo_reply.ip.protocol = IPProtocol::ICMP;
                echo_reply.ip.ttl = 64;
                echo_reply.ip.icmp.type = ICMPType::EchoReply;
                echo_reply.ip.icmp.code = 0;
                echo_reply.ip.icmp.identifier = frame.ip.icmp.identifier;
                echo_reply.ip.icmp.sequence_num = frame.ip.icmp.sequence_num;
                echo_reply.ip.icmp.payload = frame.ip.icmp.payload;

                in_port->send_frame(echo_reply);
            }
            return;
        }

        // --- Routing Transit Packet ---
        IPv4Packet routed_ip = frame.ip;
        if (routed_ip.ttl <= 1) {
            packets_dropped_ttl++;
            if (verbose) {
                std::cout << " [Router " << name_ << "] TTL expired in transit for packet to "
                          << routed_ip.dst_ip << ".\n";
            }
            return;
        }
        routed_ip.ttl--;

        IPv4Address next_hop;
        std::string out_iface_name;
        if (!routing_table_.lookup(routed_ip.dst_ip, next_hop, out_iface_name)) {
            if (verbose) {
                std::cout << " [Router " << name_ << "] No route to host " << routed_ip.dst_ip
                          << "; dropping packet.\n";
            }
            return;
        }

        Port* out_port = get_port(out_iface_name);
        if (!out_port) return;

        IPv4Address target_arp_ip = next_hop.is_zero() ? routed_ip.dst_ip : next_hop;
        MacAddress target_mac;

        if (!arp_table_.lookup(target_arp_ip, target_mac)) {
            // Resolve target via ARP
            send_arp_request(out_port, target_arp_ip);
            // After ARP request, re-check table
            if (!arp_table_.lookup(target_arp_ip, target_mac)) {
                if (verbose) {
                    std::cout << " [Router " << name_ << "] Target " << target_arp_ip
                              << " did not reply to ARP; packet lost.\n";
                }
                return;
            }
        }

        EthernetFrame out_frame;
        out_frame.src_mac = out_port->mac;
        out_frame.dst_mac = target_mac;
        out_frame.ethertype = EtherType::IPv4;
        out_frame.ip = routed_ip;

        packets_routed++;
        if (verbose) {
            std::cout << " [Router " << name_ << "] Routed packet to " << routed_ip.dst_ip
                      << " via " << out_iface_name << " (TTL=" << (int)routed_ip.ttl << ").\n";
        }
        out_port->send_frame(out_frame);
    }
}

void Router::show_ip_route(std::ostream& os) const {
    os << "Router#" << name_ << "# show ip route\n";
    routing_table_.print_cisco_style(os);
}

void Router::show_arp(std::ostream& os) const {
    os << "Router#" << name_ << "# show arp\n";
    arp_table_.print_cisco_style(os);
}

void Router::show_ip_interface_brief(std::ostream& os) const {
    os << "Interface                  IP-Address      OK? Method Status                Protocol\n";
    for (const auto& kv : ports_) {
        const auto& p = kv.second;
        os << std::left << std::setw(27) << p->name
           << std::setw(16) << (p->ip.is_zero() ? "unassigned" : p->ip.to_string())
           << "YES manual "
           << (p->is_up ? "up                    up" : "administratively down down")
           << "\n";
    }
}

} // namespace net

