#ifndef HUB_HPP
#define HUB_HPP

#include "network.hpp"
#include <iostream>

namespace net {

class Hub : public Device {
public:
    explicit Hub(const std::string& name, int num_ports = 4);

    void receive_frame(const EthernetFrame& frame, Port* in_port) override;

    int total_frames_repeated{0};
};

} // namespace net

#endif // HUB_HPP

