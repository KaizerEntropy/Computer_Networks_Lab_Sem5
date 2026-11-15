#ifndef SERVICES_HPP
#define SERVICES_HPP

#include "network.hpp"
#include "arp_table.hpp"
#include <map>
#include <set>
#include <string>

namespace net {

// =============================================================================
// 1. DHCP SERVER
// =============================================================================
class DHCPServer : public Device {
public:
    DHCPServer(const std::string& name, const MacAddress& mac,
               const IPv4Address& ip, const IPv4Address& mask,
               const IPv4Address& gateway, const IPv4Address& dns,
               const IPv4Address& pool_start, int pool_size);

    void receive_frame(const EthernetFrame& frame, Port* in_port) override;

    void show_leases(std::ostream& os = std::cout) const;

    int discover_count{0};
    int offer_count{0};
    int request_count{0};
    int ack_count{0};

private:
    IPv4Address ip_;
    IPv4Address mask_;
    IPv4Address gateway_;
    IPv4Address dns_;
    IPv4Address pool_start_;
    int         pool_size_;

    ARPTable    arp_table_;
    std::map<MacAddress, IPv4Address> active_leases_;
    std::set<uint32_t> allocated_ips_;

    IPv4Address allocate_ip(const MacAddress& mac);
};

// =============================================================================
// 2. DNS SERVER
// =============================================================================
class DNSServer : public Device {
public:
    DNSServer(const std::string& name, const MacAddress& mac,
              const IPv4Address& ip, const IPv4Address& mask,
              const IPv4Address& gateway);

    void add_a_record(const std::string& domain, const IPv4Address& ip, uint32_t ttl = 300);
    void receive_frame(const EthernetFrame& frame, Port* in_port) override;

    void show_records(std::ostream& os = std::cout) const;

    int query_count{0};
    int response_count{0};

private:
    IPv4Address ip_;
    IPv4Address mask_;
    IPv4Address gateway_;
    ARPTable    arp_table_;
    std::map<std::string, DNSRecord> zone_database_;
};

// =============================================================================
// 3. HTTP WEB SERVER
// =============================================================================
class WebServer : public Device {
public:
    WebServer(const std::string& name, const MacAddress& mac,
              const IPv4Address& ip, const IPv4Address& mask,
              const IPv4Address& gateway);

    void set_html_page(const std::string& uri, const std::string& content);
    void receive_frame(const EthernetFrame& frame, Port* in_port) override;

    int get_request_count{0};

private:
    IPv4Address ip_;
    IPv4Address mask_;
    IPv4Address gateway_;
    ARPTable    arp_table_;
    std::map<std::string, std::string> pages_;
};

// =============================================================================
// 4. FTP SERVER
// =============================================================================
class FTPServer : public Device {
public:
    FTPServer(const std::string& name, const MacAddress& mac,
              const IPv4Address& ip, const IPv4Address& mask,
              const IPv4Address& gateway);

    void add_user(const std::string& user, const std::string& pass);
    void add_file(const std::string& filename, const std::string& content);
    void receive_frame(const EthernetFrame& frame, Port* in_port) override;

    int session_count{0};

private:
    IPv4Address ip_;
    IPv4Address mask_;
    IPv4Address gateway_;
    ARPTable    arp_table_;
    std::map<std::string, std::string> users_;
    std::map<std::string, std::string> files_;
    std::map<IPv4Address, std::string> auth_sessions_;
};

} // namespace net

#endif // SERVICES_HPP

