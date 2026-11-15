#include "routing_table.hpp"
#include <iomanip>

namespace net {

void RoutingTable::add_connected(const IPv4Address& network, const IPv4Address& netmask, const std::string& iface) {
    routes_.push_back(RouteEntry{network.network_address(netmask), netmask, IPv4Address::any(), iface, 'C', 0, 0});
}

void RoutingTable::add_static(const IPv4Address& network, const IPv4Address& netmask, const IPv4Address& next_hop, const std::string& iface) {
    routes_.push_back(RouteEntry{network.network_address(netmask), netmask, next_hop, iface, 'S', 1, 0});
}

bool RoutingTable::lookup(const IPv4Address& dst_ip, IPv4Address& out_next_hop, std::string& out_iface) const {
    const RouteEntry* best_match = nullptr;
    uint32_t best_mask_val = 0;

    for (const auto& r : routes_) {
        if ((dst_ip.val & r.netmask.val) == r.network.val) {
            // Check if this mask is more specific than current best
            if (!best_match || r.netmask.val >= best_mask_val) {
                best_match = &r;
                best_mask_val = r.netmask.val;
            }
        }
    }

    if (best_match) {
        out_next_hop = best_match->next_hop;
        out_iface = best_match->interface_name;
        return true;
    }
    return false;
}

void RoutingTable::print_cisco_style(std::ostream& os) const {
    os << "Codes: C - connected, S - static, R - RIP, M - mobile, B - BGP\n";
    os << "Gateway of last resort is not set\n\n";
    for (const auto& r : routes_) {
        // Calculate CIDR prefix length
        int prefix = 0;
        uint32_t m = r.netmask.val;
        while (m & 0x80000000) {
            prefix++;
            m <<= 1;
        }

        if (r.route_type == 'C') {
            os << "C    " << r.network.to_string() << "/" << prefix
               << " is directly connected, " << r.interface_name << "\n";
        } else if (r.route_type == 'S') {
            os << "S    " << r.network.to_string() << "/" << prefix
               << " [" << r.admin_distance << "/" << r.metric << "] via "
               << r.next_hop.to_string();
            if (!r.interface_name.empty()) {
                os << ", " << r.interface_name;
            }
            os << "\n";
        }
    }
}

} // namespace net

