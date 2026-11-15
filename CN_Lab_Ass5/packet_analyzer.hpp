#ifndef PACKET_ANALYZER_HPP
#define PACKET_ANALYZER_HPP

#include "pcap_parser.hpp"
#include <string>
#include <vector>

class PacketAnalyzer {
private:
    PcapParser parser;

    bool match_filter(const PacketRecord& pkt, const std::string& filter) const;

public:
    PacketAnalyzer(const std::string& pcap_file);
    bool load();

    void display_packet_list(const std::string& filter = "");
    void display_packet_details(uint32_t frame_num);
    void display_packet_bytes(uint32_t frame_num, int highlight_start = -1, int highlight_len = 0);
    void follow_tcp_stream(uint16_t port1 = 49152, uint16_t port2 = 80);
    void display_protocol_hierarchy();
    void display_flow_graph();
    void run_all_questions();
};

#endif // PACKET_ANALYZER_HPP

