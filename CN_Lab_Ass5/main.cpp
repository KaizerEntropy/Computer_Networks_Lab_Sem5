#include "packet_analyzer.hpp"
#include <iostream>
#include <string>

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n"
              << "Options:\n"
              << "  -r <file.pcap>    Specify input pcap file (default: network_capture.pcap)\n"
              << "  -l                List all captured packets (Packet List Panel)\n"
              << "  -d <frame_no>     Display protocol details for packet (Packet Details Panel)\n"
              << "  -b <frame_no>     Display raw Hex and ASCII bytes (Packet Bytes Panel)\n"
              << "  -Y <filter>       Apply display filter (e.g. http, tcp, udp, dns, icmp, arp)\n"
              << "  -z hierarchy      Display protocol hierarchy statistics\n"
              << "  -z flow           Display sequence flow graph\n"
              << "  --follow-stream   Follow TCP conversation stream (HTTP session)\n"
              << "  --analyze-all     Run thorough automated analysis of Questions 1-10\n"
              << "  -h, --help        Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string pcap_file = "network_capture.pcap";
    bool list_mode = false;
    int detail_frame = -1;
    int bytes_frame = -1;
    std::string filter = "";
    bool show_hierarchy = false;
    bool show_flow = false;
    bool follow_stream = false;
    bool analyze_all = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-r" && i + 1 < argc) {
            pcap_file = argv[++i];
        } else if (arg == "-l") {
            list_mode = true;
        } else if (arg == "-d" && i + 1 < argc) {
            detail_frame = std::stoi(argv[++i]);
        } else if (arg == "-b" && i + 1 < argc) {
            bytes_frame = std::stoi(argv[++i]);
        } else if (arg == "-Y" && i + 1 < argc) {
            filter = argv[++i];
        } else if (arg == "-z" && i + 1 < argc) {
            std::string z_opt = argv[++i];
            if (z_opt == "hierarchy") show_hierarchy = true;
            else if (z_opt == "flow") show_flow = true;
        } else if (arg == "--follow-stream") {
            follow_stream = true;
        } else if (arg == "--analyze-all") {
            analyze_all = true;
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    // Default to analyze_all if no specific flags provided
    if (!list_mode && detail_frame == -1 && bytes_frame == -1 && filter.empty() &&
        !show_hierarchy && !show_flow && !follow_stream && !analyze_all) {
        analyze_all = true;
    }

    PacketAnalyzer analyzer(pcap_file);
    if (!analyzer.load()) {
        if (pcap_file == "network_capture.pcap") {
            analyzer = PacketAnalyzer("CN_Lab_Ass5/network_capture.pcap");
            if (!analyzer.load()) {
                std::cerr << "[-] Failed to load PCAP file: network_capture.pcap or CN_Lab_Ass5/network_capture.pcap\n";
                return 1;
            }
        } else {
            return 1;
        }
    }

    if (analyze_all) {
        analyzer.run_all_questions();
        return 0;
    }

    if (!filter.empty() || list_mode) {
        analyzer.display_packet_list(filter);
    }
    if (detail_frame > 0) {
        analyzer.display_packet_details(detail_frame);
    }
    if (bytes_frame > 0) {
        analyzer.display_packet_bytes(bytes_frame);
    }
    if (follow_stream) {
        analyzer.follow_tcp_stream();
    }
    if (show_hierarchy) {
        analyzer.display_protocol_hierarchy();
    }
    if (show_flow) {
        analyzer.display_flow_graph();
    }

    return 0;
}
