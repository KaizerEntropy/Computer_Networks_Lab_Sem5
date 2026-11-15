#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "network.hpp"
#include "routing_table.hpp"
#include "arp_table.hpp"
#include <iostream>

namespace net {

class Router : public Device {
public:
    explicit Router(const std::string& name);

    Port* configure_interface(const std::string& name, const IPv4Address& ip,
                             const IPv4Address& mask, const MacAddress& mac);

    void add_static_route(const IPv4Address& net, const IPv4Address& mask, const IPv4Address& next_hop);

    void receive_frame(const EthernetFrame& frame, Port* in_port) override;

    void show_ip_route(std::ostream& os = std::cout) const;
    void show_arp(std::ostream& os = std::cout) const;
    void show_ip_interface_brief(std::ostream& os = std::cout) const;

    RoutingTable& routing_table() { return routing_table_; }
    ARPTable& arp_table() { return arp_table_; }

    int packets_routed{0};
    int packets_dropped_ttl{0};

private:
    RoutingTable routing_table_;
    ARPTable     arp_table_;

    void send_arp_request(Port* out_port, const IPv4Address& target_ip);
};

} // namespace net

#endif // ROUTER_HPP

