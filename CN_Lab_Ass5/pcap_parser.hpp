#ifndef PCAP_PARSER_HPP
#define PCAP_PARSER_HPP

#include "protocol.hpp"
#include "oui_db.hpp"
#include <string>
#include <vector>
#include <memory>

class PcapParser {
private:
    std::string filename;
    OuiDatabase oui_db;
    std::vector<PacketRecord> packets;
    bool is_byte_swapped = false;

    std::string format_mac(const uint8_t* mac);
    std::string format_ip(const uint8_t* ip);
    uint16_t swap16(uint16_t val);
    uint32_t swap32(uint32_t val);

    void dissect_packet(PacketRecord& rec, uint32_t frame_idx, double first_ts);

public:
    PcapParser(const std::string& pcap_file);
    bool parse();
    const std::vector<PacketRecord>& get_packets() const { return packets; }
    const OuiDatabase& get_oui_db() const { return oui_db; }
};

#endif // PCAP_PARSER_HPP

