#ifndef HOST_HPP
#define HOST_HPP

#include "network.hpp"
#include "arp_table.hpp"
#include <iostream>
#include <vector>

namespace net {

struct PingStats {
    int sent{0};
    int received{0};
    int lost{0};
    double min_rtt{0.0};
    double max_rtt{0.0};
    double avg_rtt{0.0};
};

class Host : public Device {
public:
    Host(const std::string& name, const MacAddress& mac,
         const IPv4Address& ip = IPv4Address::any(),
         const IPv4Address& mask = IPv4Address(255, 255, 255, 0),
         const IPv4Address& gateway = IPv4Address::any(),
         const IPv4Address& dns = IPv4Address::any());

    void set_ip(const IPv4Address& ip) { ip_ = ip; port_->ip = ip; }
    void set_mask(const IPv4Address& mask) { mask_ = mask; port_->mask = mask; }
    void set_gateway(const IPv4Address& gw) { gateway_ = gw; }
    void set_dns(const IPv4Address& dns) { dns_ = dns; }

    const IPv4Address& ip() const { return ip_; }
    const IPv4Address& mask() const { return mask_; }
    const IPv4Address& gateway() const { return gateway_; }
    const IPv4Address& dns() const { return dns_; }
    const MacAddress& mac() const { return mac_; }
    Port* port() { return port_; }

    ARPTable& arp_table() { return arp_table_; }
    const ARPTable& arp_table() const { return arp_table_; }

    void receive_frame(const EthernetFrame& frame, Port* in_port) override;

    // CLI Network Commands
    PingStats ping(const IPv4Address& target, int count = 4, std::ostream& os = std::cout);
    bool request_dhcp(std::ostream& os = std::cout);
    IPv4Address nslookup(const std::string& domain, std::ostream& os = std::cout);
    std::string http_get(const std::string& url_or_domain, std::ostream& os = std::cout);
    bool ftp_session(const std::string& host_or_domain, const std::string& user,
                     const std::string& pass, const std::string& get_file,
                     std::ostream& os = std::cout);

    void ipconfig(bool all = false, std::ostream& os = std::cout) const;
    void show_arp(std::ostream& os = std::cout) const;
    void flush_arp();

    bool dhcp_enabled{false};

private:
    MacAddress  mac_;
    IPv4Address ip_;
    IPv4Address mask_;
    IPv4Address gateway_;
    IPv4Address dns_;
    Port*       port_{nullptr};
    ARPTable    arp_table_;

    // Temp buffers for synchronous request-response captures
    bool        icmp_reply_received_{false};
    uint16_t    last_icmp_seq_{0};
    bool        dhcp_offer_received_{false};
    bool        dhcp_ack_received_{false};
    DHCPMessage last_dhcp_offer_;
    DHCPMessage last_dhcp_ack_;

    bool        dns_reply_received_{false};
    DNSMessage  last_dns_reply_;

    bool        http_reply_received_{false};
    HTTPMessage last_http_reply_;

    bool        ftp_reply_received_{false};
    FTPMessage  last_ftp_reply_;

    bool resolve_arp(const IPv4Address& target_ip, MacAddress& out_mac);
};

} // namespace net

#endif // HOST_HPP

