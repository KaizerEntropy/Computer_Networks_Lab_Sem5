#include "switch.hpp"
#include <iostream>

namespace net {

Switch::Switch(const std::string& name, int num_ports) : Device(name) {
    for (int i = 1; i <= num_ports; ++i) {
        add_port("FastEthernet0/" + std::to_string(i));
    }
}

void Switch::receive_frame(const EthernetFrame& frame, Port* in_port) {
    // 1. MAC Learning (Backward learning from Source MAC)
    if (!frame.src_mac.is_broadcast() && !frame.src_mac.is_zero()) {
        mac_table_.learn(frame.src_mac, in_port->name);
    }

    // 2. Forwarding Decision
    if (frame.dst_mac.is_broadcast()) {
        total_frames_flooded++;
        if (verbose) {
            std::cout << " [Switch " << name_ << "] Broadcast frame from "
                      << frame.src_mac.to_cisco_string() << " flooded to all active ports.\n";
        }
        for (auto& kv : ports_) {
            Port* out_p = kv.second.get();
            if (out_p != in_port && out_p->peer != nullptr) {
                out_p->send_frame(frame);
            }
        }
    } else {
        std::string target_port_name;
        if (mac_table_.lookup(frame.dst_mac, target_port_name)) {
            // Known Unicast: Selective Forwarding
            total_frames_switched++;
            Port* target_p = get_port(target_port_name);
            if (target_p && target_p != in_port && target_p->peer != nullptr) {
                if (verbose) {
                    std::cout << " [Switch " << name_ << "] Filtered and forwarded frame destined to "
                              << frame.dst_mac.to_cisco_string() << " selectively to port "
                              << target_port_name << ".\n";
                }
                target_p->send_frame(frame);
            }
        } else {
            // Unknown Unicast: Flooding
            total_frames_flooded++;
            if (verbose) {
                std::cout << " [Switch " << name_ << "] Unknown unicast destination "
                          << frame.dst_mac.to_cisco_string() << " flooded to all ports.\n";
            }
            for (auto& kv : ports_) {
                Port* out_p = kv.second.get();
                if (out_p != in_port && out_p->peer != nullptr) {
                    out_p->send_frame(frame);
                }
            }
        }
    }
}

void Switch::show_mac_address_table(std::ostream& os) const {
    mac_table_.print_cisco_table(os);
}

void Switch::clear_mac_address_table() {
    mac_table_.flush_dynamic();
}

} // namespace net

