#include "hub.hpp"

namespace net {

Hub::Hub(const std::string& name, int num_ports) : Device(name) {
    for (int i = 0; i < num_ports; ++i) {
        add_port("FastEthernet" + std::to_string(i));
    }
}

void Hub::receive_frame(const EthernetFrame& frame, Port* in_port) {
    total_frames_repeated++;
    if (verbose) {
        std::cout << " [Hub " << name_ << "] Received bitstream on " << in_port->name
                  << "; repeating unconditionally to all other ports (single collision domain).\n";
    }

    // Layer-1 broadcast: repeat out of every connected port except in_port
    for (auto& kv : ports_) {
        Port* out_p = kv.second.get();
        if (out_p != in_port && out_p->peer != nullptr) {
            out_p->send_frame(frame);
        }
    }
}

} // namespace net

