#ifndef MAC_TABLE_HPP
#define MAC_TABLE_HPP

#include "types.hpp"
#include <map>
#include <string>
#include <iostream>

namespace net {

struct MACEntry {
    int         vlan{1};
    MacAddress  mac;
    std::string type{"DYNAMIC"};
    std::string port_name;
    int         age_seconds{0};
};

class MACTable {
public:
    MACTable() = default;

    void learn(const MacAddress& mac, const std::string& port_name, int vlan = 1);
    bool lookup(const MacAddress& mac, std::string& out_port) const;
    void flush_dynamic();

    bool empty() const { return table_.empty(); }
    size_t size() const { return table_.size(); }

    void print_cisco_table(std::ostream& os = std::cout) const;

    const std::map<MacAddress, MACEntry>& get_entries() const { return table_; }

private:
    std::map<MacAddress, MACEntry> table_;
};

} // namespace net

#endif // MAC_TABLE_HPP

