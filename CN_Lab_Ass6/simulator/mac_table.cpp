#include "mac_table.hpp"
#include <iomanip>

namespace net {

void MACTable::learn(const MacAddress& mac, const std::string& port_name, int vlan) {
    if (mac.is_broadcast() || mac.is_zero()) return;
    table_[mac] = MACEntry{vlan, mac, "DYNAMIC", port_name, 0};
}

bool MACTable::lookup(const MacAddress& mac, std::string& out_port) const {
    auto it = table_.find(mac);
    if (it != table_.end()) {
        out_port = it->second.port_name;
        return true;
    }
    return false;
}

void MACTable::flush_dynamic() {
    table_.clear();
}

void MACTable::print_cisco_table(std::ostream& os) const {
    os << "          Mac Address Table\n";
    os << "-------------------------------------------\n";
    os << "Vlan    Mac Address       Type        Ports\n";
    os << "----    -----------       --------    -----\n";
    if (table_.empty()) {
        os << "Total Mac Addresses for this module: 0\n";
        return;
    }
    for (const auto& kv : table_) {
        const auto& e = kv.second;
        os << std::left << std::setw(8) << e.vlan
           << std::setw(18) << e.mac.to_cisco_string()
           << std::setw(12) << e.type
           << e.port_name << "\n";
    }
    os << "Total Mac Addresses for this module: " << table_.size() << "\n";
}

} // namespace net

