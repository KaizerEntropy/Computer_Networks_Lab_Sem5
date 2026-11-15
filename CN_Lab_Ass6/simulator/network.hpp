#ifndef NETWORK_HPP
#define NETWORK_HPP

#include "types.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

namespace net {

class Device;

enum class CableType {
    StraightThrough,
    CrossOver,
    Serial
};

struct Port {
    std::string name;
    MacAddress  mac;
    IPv4Address ip;
    IPv4Address mask;
    Device*     owner{nullptr};
    Port*       peer{nullptr};
    CableType   cable{CableType::StraightThrough};
    bool        is_up{true};

    void send_frame(const EthernetFrame& frame);
};

class Device {
public:
    explicit Device(std::string name) : name_(std::move(name)) {}
    virtual ~Device() = default;

    const std::string& name() const { return name_; }

    Port* add_port(const std::string& port_name, const MacAddress& mac = MacAddress::zero());
    Port* get_port(const std::string& port_name);

    virtual void receive_frame(const EthernetFrame& frame, Port* in_port) = 0;

    static bool connect_ports(Port* p1, Port* p2, CableType cable = CableType::StraightThrough);

    bool verbose{false};

protected:
    std::string name_;
    std::map<std::string, std::unique_ptr<Port>> ports_;
};

} // namespace net

#endif // NETWORK_HPP

