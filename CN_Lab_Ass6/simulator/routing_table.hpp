#ifndef ROUTING_TABLE_HPP
#define ROUTING_TABLE_HPP

#include "types.hpp"
#include <vector>
#include <string>
#include <iostream>

namespace net {

struct RouteEntry {
    IPv4Address network;
    IPv4Address netmask;
    IPv4Address next_hop;
    std::string interface_name;
    char        route_type{'S'}; // 'C' = Connected, 'S' = Static
    int         admin_distance{1};
    int         metric{0};
};

class RoutingTable {
public:
    RoutingTable() = default;

    void add_connected(const IPv4Address& network, const IPv4Address& netmask, const std::string& iface);
    void add_static(const IPv4Address& network, const IPv4Address& netmask, const IPv4Address& next_hop, const std::string& iface = "");
    
    bool lookup(const IPv4Address& dst_ip, IPv4Address& out_next_hop, std::string& out_iface) const;
    void print_cisco_style(std::ostream& os = std::cout) const;

    const std::vector<RouteEntry>& get_routes() const { return routes_; }

private:
    std::vector<RouteEntry> routes_;
};

} // namespace net

#endif // ROUTING_TABLE_HPP

