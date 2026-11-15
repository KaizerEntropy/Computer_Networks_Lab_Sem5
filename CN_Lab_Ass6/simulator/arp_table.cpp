#include "arp_table.hpp"
#include <iomanip>

namespace net {

void ARPTable::add_or_update(const IPv4Address& ip, const MacAddress& mac, bool is_dynamic) {
    table_[ip] = ARPEntry{ip, mac, is_dynamic, 0};
}

bool ARPTable::lookup(const IPv4Address& ip, MacAddress& out_mac) const {
    auto it = table_.find(ip);
    if (it != table_.end()) {
        out_mac = it->second.mac;
        return true;
    }
    return false;
}

void ARPTable::remove(const IPv4Address& ip) {
    table_.erase(ip);
}

void ARPTable::flush() {
    table_.clear();
}

void ARPTable::print_table(std::ostream& os, const std::string& title) const {
    os << "------------------------------------------------------------\n";
    os << " " << title << "\n";
    os << "------------------------------------------------------------\n";
    if (table_.empty()) {
        os << " No ARP Entries Found.\n";
        os << "------------------------------------------------------------\n";
        return;
    }
    os << "  Internet Address      Physical Address      Type\n";
    for (const auto& kv : table_) {
        const auto& entry = kv.second;
        os << "  " << std::left << std::setw(22) << entry.ip.to_string()
           << std::setw(22) << entry.mac.to_cisco_string()
           << (entry.is_dynamic ? "dynamic" : "static") << "\n";
    }
    os << "------------------------------------------------------------\n";
}

void ARPTable::print_cisco_style(std::ostream& os, const std::string& iface) const {
    os << "Protocol  Address          Age (min)  Hardware Addr   Type   Interface\n";
    if (table_.empty()) {
        return;
    }
    for (const auto& kv : table_) {
        const auto& entry = kv.second;
        os << "Internet  " << std::left << std::setw(17) << entry.ip.to_string()
           << std::setw(11) << entry.age_seconds / 60
           << std::setw(16) << entry.mac.to_cisco_string()
           << "ARPA   " << iface << "\n";
    }
}

} // namespace net

