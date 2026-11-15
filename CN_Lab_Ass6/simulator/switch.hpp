#ifndef SWITCH_HPP
#define SWITCH_HPP

#include "network.hpp"
#include "mac_table.hpp"

namespace net {

class Switch : public Device {
public:
    explicit Switch(const std::string& name, int num_ports = 24);

    void receive_frame(const EthernetFrame& frame, Port* in_port) override;

    MACTable& mac_table() { return mac_table_; }
    const MACTable& mac_table() const { return mac_table_; }

    void show_mac_address_table(std::ostream& os = std::cout) const;
    void clear_mac_address_table();

    int total_frames_switched{0};
    int total_frames_flooded{0};

private:
    MACTable mac_table_;
};

} // namespace net

#endif // SWITCH_HPP

