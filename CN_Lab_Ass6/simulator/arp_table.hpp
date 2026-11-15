#ifndef ARP_TABLE_HPP
#define ARP_TABLE_HPP

#include "types.hpp"
#include <map>
#include <string>
#include <iostream>

namespace net {

struct ARPEntry {
    IPv4Address ip;
    MacAddress  mac;
    bool        is_dynamic{true};
    int         age_seconds{0};
};

class ARPTable {
public:
    ARPTable() = default;

    void add_or_update(const IPv4Address& ip, const MacAddress& mac, bool is_dynamic = true);
    bool lookup(const IPv4Address& ip, MacAddress& out_mac) const;
    void remove(const IPv4Address& ip);
    void flush();

    bool empty() const { return table_.empty(); }
    size_t size() const { return table_.size(); }

    void print_table(std::ostream& os = std::cout, const std::string& title = "ARP Table") const;
    void print_cisco_style(std::ostream& os = std::cout, const std::string& iface = "FastEthernet0/0") const;

    const std::map<IPv4Address, ARPEntry>& get_entries() const { return table_; }

private:
    std::map<IPv4Address, ARPEntry> table_;
};

} // namespace net

#endif // ARP_TABLE_HPP

