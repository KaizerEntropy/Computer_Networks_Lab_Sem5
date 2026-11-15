#include "network.hpp"
#include <iostream>

namespace net {

void Port::send_frame(const EthernetFrame& frame) {
    if (!is_up || !peer || !peer->is_up) {
        return;
    }
    peer->owner->receive_frame(frame, peer);
}

Port* Device::add_port(const std::string& port_name, const MacAddress& mac) {
    auto port = std::make_unique<Port>();
    port->name = port_name;
    port->mac = mac;
    port->owner = this;
    Port* ptr = port.get();
    ports_[port_name] = std::move(port);
    return ptr;
}

Port* Device::get_port(const std::string& port_name) {
    auto it = ports_.find(port_name);
    if (it != ports_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool Device::connect_ports(Port* p1, Port* p2, CableType cable) {
    if (!p1 || !p2) return false;
    p1->peer = p2;
    p2->peer = p1;
    p1->cable = cable;
    p2->cable = cable;
    return true;
}

} // namespace net

