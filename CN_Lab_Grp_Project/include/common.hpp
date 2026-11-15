#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace bdtp {

// ============================================================================
// ANSI Color Escape Sequences for Terminal Presentation
// ============================================================================
namespace color {
    inline const std::string RESET       = "\033[0m";
    inline const std::string BOLD        = "\033[1m";
    inline const std::string DIM         = "\033[2m";
    inline const std::string UNDERLINE   = "\033[4m";
    
    inline const std::string BLACK       = "\033[30m";
    inline const std::string RED         = "\033[31m";
    inline const std::string GREEN       = "\033[32m";
    inline const std::string YELLOW      = "\033[33m";
    inline const std::string BLUE        = "\033[34m";
    inline const std::string MAGENTA     = "\033[35m";
    inline const std::string CYAN        = "\033[36m";
    inline const std::string WHITE       = "\033[37m";
    
    inline const std::string BOLD_RED    = "\033[1;31m";
    inline const std::string BOLD_GREEN  = "\033[1;32m";
    inline const std::string BOLD_YELLOW = "\033[1;33m";
    inline const std::string BOLD_BLUE   = "\033[1;34m";
    inline const std::string BOLD_MAGENTA= "\033[1;35m";
    inline const std::string BOLD_CYAN   = "\033[1;36m";
    inline const std::string BOLD_WHITE  = "\033[1;37m";

    inline const std::string BG_BLUE     = "\033[44m";
    inline const std::string BG_DARK     = "\033[48;5;236m";
}

// ============================================================================
// Timing & Clock Utilities
// ============================================================================
inline double now_ms() {
    using namespace std::chrono;
    auto now = steady_clock::now().time_since_epoch();
    return duration_cast<microseconds>(now).count() / 1000.0;
}

inline std::string current_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
    return std::string(buf);
}

// ============================================================================
// Protocol Identifiers & Constants
// ============================================================================
constexpr uint32_t BDTP_MAGIC = 0x42445450; // "BDTP"

enum ProtocolId : uint8_t {
    PROTO_BOOTP           = 1,
    PROTO_DHCP            = 2,
    PROTO_TELNET_CONTROL  = 3,
    PROTO_TELNET_DATA     = 4,
    PROTO_ACK             = 5,
    PROTO_NACK            = 6
};

inline std::string protocol_to_string(uint8_t proto) {
    switch (proto) {
        case PROTO_BOOTP:          return "BOOTP";
        case PROTO_DHCP:           return "DHCP";
        case PROTO_TELNET_CONTROL: return "TELNET-CTRL";
        case PROTO_TELNET_DATA:    return "TELNET-DATA";
        case PROTO_ACK:            return "ACK";
        case PROTO_NACK:           return "NACK";
        default:                   return "UNKNOWN";
    }
}

// ============================================================================
// Network Addresses Utilities
// ============================================================================
inline std::string ip_to_string(uint32_t ip) {
    struct in_addr addr;
    addr.s_addr = ip;
    return std::string(inet_ntoa(addr));
}

inline uint32_t string_to_ip(const std::string& ip_str) {
    struct in_addr addr;
    if (inet_aton(ip_str.c_str(), &addr) == 0) return 0;
    return addr.s_addr;
}

inline std::string mac_to_string(const uint8_t mac[6]) {
    char buf[20];
    std::snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buf);
}

inline bool string_to_mac(const std::string& mac_str, uint8_t mac[6]) {
    unsigned int m[6];
    if (std::sscanf(mac_str.c_str(), "%x:%x:%x:%x:%x:%x",
                    &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) == 6) {
        for (int i = 0; i < 6; ++i) mac[i] = static_cast<uint8_t>(m[i]);
        return true;
    }
    return false;
}

// ============================================================================
// Beautiful Hex Dump Utility
// ============================================================================
inline std::string hex_dump(const void* data, size_t size, const std::string& indent = "  ") {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
    std::ostringstream oss;
    
    for (size_t i = 0; i < size; i += 16) {
        oss << indent << color::CYAN << std::hex << std::setw(4) << std::setfill('0') << i << color::RESET << "  ";
        
        // Hex bytes
        for (size_t j = 0; j < 16; ++j) {
            if (i + j < size) {
                oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(p[i + j]) << " ";
            } else {
                oss << "   ";
            }
            if (j == 7) oss << " ";
        }
        
        oss << " |" << color::YELLOW;
        // Printable ASCII
        for (size_t j = 0; j < 16 && (i + j) < size; ++j) {
            char c = static_cast<char>(p[i + j]);
            if (c >= 32 && c <= 126) oss << c;
            else oss << ".";
        }
        oss << color::RESET << "|\n";
    }
    return oss.str();
}

// ============================================================================
// Encapsulation Frame Header (Standard Data Link / Transport Envelope)
// ============================================================================
#pragma pack(push, 1)
struct FrameHeader {
    uint32_t magic;           // BDTP_MAGIC
    uint8_t  protocol_id;     // ProtocolId (BOOTP, DHCP, TELNET)
    uint16_t seq_num;         // Sequence number
    uint16_t ack_num;         // Acknowledgment sequence number
    uint8_t  detection_scheme;// 1=Checksum16, 2=CRC8, 3=CRC10, 4=CRC16, 5=CRC32, 6=Hamming, 7=Parity
    uint16_t payload_len;     // Length of wrapped protocol payload
    uint32_t fcs;             // Frame Check Sequence (Trailer value)
};
#pragma pack(pop)

// ============================================================================
// Comprehensive Transmission Statistics & Telemetry
// ============================================================================
struct Statistics {
    std::string role; // "Sender/Client" or "Receiver/Server"
    int total_packets_sent;
    int total_packets_received;
    int total_retransmissions;
    int packets_lost;
    int packets_corrupted;
    int packets_delayed;
    int errors_injected;
    int errors_detected;
    int errors_undetected; // Blind spots
    int bootp_packets;
    int dhcp_packets;
    int telnet_packets;
    int udp_packets;
    int tcp_packets;
    size_t total_bytes;         // Gross wire bytes (including headers)
    size_t net_payload_bytes;   // Pure protocol payload bytes
    size_t header_overhead_bytes;// Framing overhead bytes
    size_t bootp_bytes;
    size_t dhcp_bytes;
    size_t telnet_bytes;
    double start_time_ms;
    double end_time_ms;
    double bootp_duration_ms;
    double dhcp_duration_ms;
    double telnet_duration_ms;
    std::vector<double> rtt_samples;

    Statistics(const std::string& r = "Node")
        : role(r), total_packets_sent(0), total_packets_received(0),
          total_retransmissions(0), packets_lost(0), packets_corrupted(0),
          packets_delayed(0), errors_injected(0), errors_detected(0),
          errors_undetected(0), bootp_packets(0), dhcp_packets(0),
          telnet_packets(0), udp_packets(0), tcp_packets(0),
          total_bytes(0), net_payload_bytes(0), header_overhead_bytes(0),
          bootp_bytes(0), dhcp_bytes(0), telnet_bytes(0),
          start_time_ms(0), end_time_ms(0),
          bootp_duration_ms(0), dhcp_duration_ms(0), telnet_duration_ms(0) {}

    void start() {
        start_time_ms = now_ms();
    }

    void stop() {
        end_time_ms = now_ms();
    }

    void record_packet_sent(uint8_t proto_id, size_t frame_sz, size_t pld_sz) {
        total_packets_sent++;
        total_bytes += frame_sz;
        net_payload_bytes += pld_sz;
        header_overhead_bytes += (frame_sz - pld_sz);

        if (proto_id == PROTO_BOOTP) {
            bootp_packets++;
            udp_packets++;
            bootp_bytes += frame_sz;
        } else if (proto_id == PROTO_DHCP) {
            dhcp_packets++;
            udp_packets++;
            dhcp_bytes += frame_sz;
        } else if (proto_id == PROTO_TELNET_CONTROL || proto_id == PROTO_TELNET_DATA) {
            telnet_packets++;
            tcp_packets++;
            telnet_bytes += frame_sz;
        }
    }

    void record_packet_received(uint8_t proto_id, size_t frame_sz, size_t pld_sz) {
        total_packets_received++;
        total_bytes += frame_sz;
        net_payload_bytes += pld_sz;
        header_overhead_bytes += (frame_sz - pld_sz);

        if (proto_id == PROTO_BOOTP) {
            bootp_packets++;
            udp_packets++;
            bootp_bytes += frame_sz;
        } else if (proto_id == PROTO_DHCP) {
            dhcp_packets++;
            udp_packets++;
            dhcp_bytes += frame_sz;
        } else if (proto_id == PROTO_TELNET_CONTROL || proto_id == PROTO_TELNET_DATA || proto_id == PROTO_ACK) {
            telnet_packets++;
            tcp_packets++;
            telnet_bytes += frame_sz;
        }
    }

    void merge(const Statistics& o) {
        total_packets_sent += o.total_packets_sent;
        total_packets_received += o.total_packets_received;
        total_retransmissions += o.total_retransmissions;
        packets_lost += o.packets_lost;
        packets_corrupted += o.packets_corrupted;
        packets_delayed += o.packets_delayed;
        errors_injected += o.errors_injected;
        errors_detected += o.errors_detected;
        errors_undetected += o.errors_undetected;
        bootp_packets += o.bootp_packets;
        dhcp_packets += o.dhcp_packets;
        telnet_packets += o.telnet_packets;
        udp_packets += o.udp_packets;
        tcp_packets += o.tcp_packets;
        total_bytes += o.total_bytes;
        net_payload_bytes += o.net_payload_bytes;
        header_overhead_bytes += o.header_overhead_bytes;
        bootp_bytes += o.bootp_bytes;
        dhcp_bytes += o.dhcp_bytes;
        telnet_bytes += o.telnet_bytes;
        bootp_duration_ms += o.bootp_duration_ms;
        dhcp_duration_ms += o.dhcp_duration_ms;
        telnet_duration_ms += o.telnet_duration_ms;
        rtt_samples.insert(rtt_samples.end(), o.rtt_samples.begin(), o.rtt_samples.end());
    }

    double elapsed_ms() const {
        if (end_time_ms <= start_time_ms) return now_ms() - start_time_ms;
        return end_time_ms - start_time_ms;
    }

    void record_rtt(double rtt) {
        if (rtt >= 0.0) rtt_samples.push_back(rtt);
    }

    double min_rtt() const {
        if (rtt_samples.empty()) return 0.0;
        return *std::min_element(rtt_samples.begin(), rtt_samples.end());
    }

    double max_rtt() const {
        if (rtt_samples.empty()) return 0.0;
        return *std::max_element(rtt_samples.begin(), rtt_samples.end());
    }

    double avg_rtt() const {
        if (rtt_samples.empty()) return 0.0;
        double sum = std::accumulate(rtt_samples.begin(), rtt_samples.end(), 0.0);
        return sum / rtt_samples.size();
    }

    double median_rtt() const {
        if (rtt_samples.empty()) return 0.0;
        std::vector<double> sorted = rtt_samples;
        std::sort(sorted.begin(), sorted.end());
        size_t n = sorted.size();
        if (n % 2 == 1) return sorted[n / 2];
        return (sorted[n / 2 - 1] + sorted[n / 2]) / 2.0;
    }

    double stddev_rtt() const {
        if (rtt_samples.size() < 2) return 0.0;
        double mean = avg_rtt();
        double var_sum = 0.0;
        for (double s : rtt_samples) {
            var_sum += (s - mean) * (s - mean);
        }
        return std::sqrt(var_sum / (rtt_samples.size() - 1));
    }

    double jitter() const {
        if (rtt_samples.size() < 2) return 0.0;
        double diff_sum = 0.0;
        for (size_t i = 1; i < rtt_samples.size(); ++i) {
            diff_sum += std::abs(rtt_samples[i] - rtt_samples[i - 1]);
        }
        return diff_sum / (rtt_samples.size() - 1);
    }

    double efficiency() const {
        int unique = total_packets_sent - total_retransmissions;
        if (total_packets_sent <= 0) return 100.0;
        return (static_cast<double>(unique) / total_packets_sent) * 100.0;
    }

    double gross_wire_rate_kbps() const {
        double sec = elapsed_ms() / 1000.0;
        if (sec <= 0.0001) return 0.0;
        return (total_bytes * 8.0) / (sec * 1000.0);
    }

    double net_goodput_kbps() const {
        double sec = elapsed_ms() / 1000.0;
        if (sec <= 0.0001) return 0.0;
        return (net_payload_bytes * 8.0) / (sec * 1000.0);
    }

    double header_overhead_pct() const {
        if (total_bytes == 0) return 0.0;
        return (static_cast<double>(header_overhead_bytes) / total_bytes) * 100.0;
    }

    void print_summary(const std::string& title = "NETWORK SIMULATION & PROTOCOL ANALYSIS") const {
        std::cout << "\n" << color::BOLD_CYAN
                  << "╔════════════════════════════════════════════════════════════════════════════════╗\n"
                  << "║ " << std::left << std::setw(78) << title << " ║\n"
                  << "╚════════════════════════════════════════════════════════════════════════════════╝\n"
                  << color::RESET;

        // 1. EXECUTION CONTEXT & RUNTIME
        std::cout << color::BOLD_WHITE << " [1] SESSION & RUNTIME ENVIRONMENT\n" << color::RESET;
        std::cout << "  * Target Execution Role  : " << color::BOLD_YELLOW << role << color::RESET << "\n";
        std::cout << "  * Total Elapsed Runtime  : " << color::BOLD_GREEN << std::fixed << std::setprecision(2)
                  << elapsed_ms() << " ms" << color::RESET << " (" << std::fixed << std::setprecision(4)
                  << (elapsed_ms() / 1000.0) << " seconds)\n";
        std::cout << "  * Wall-Clock Timestamps  : T_start=" << std::fixed << std::setprecision(0) << start_time_ms
                  << " ms | T_end=" << end_time_ms << " ms\n";
        std::cout << "--------------------------------------------------------------------------------\n";

        // 2. PROTOCOL DISSECTION & TRANSPORT LAYERS
        std::cout << color::BOLD_WHITE << " [2] PROTOCOL STACK & TRANSPORT LAYER DISSECTION\n" << color::RESET;
        std::cout << "  * BOOTP (RFC 951/1048)   : " << std::setw(4) << bootp_packets << " frames  | "
                  << std::setw(6) << bootp_bytes << " wire bytes | Transport: UDP (Port 6767)\n";
        std::cout << "  * DHCP DORA (RFC 2131)   : " << std::setw(4) << dhcp_packets << " frames  | "
                  << std::setw(6) << dhcp_bytes << " wire bytes | Transport: UDP (Port 6769)\n";
        std::cout << "  * TELNET App (RFC 854)   : " << std::setw(4) << telnet_packets << " frames  | "
                  << std::setw(6) << telnet_bytes << " wire bytes | Transport: TCP (Port 2323)\n";
        std::cout << "  * Transport Distribution : " << color::CYAN << udp_packets << " UDP datagrams" << color::RESET
                  << " vs " << color::MAGENTA << tcp_packets << " TCP segments" << color::RESET << "\n";
        std::cout << "--------------------------------------------------------------------------------\n";

        // 3. WIRE BANDWIDTH, PAYLOAD & GOODPUT
        std::cout << color::BOLD_WHITE << " [3] WIRE TRAFFIC VOLUME, GOODPUT & OVERHEAD ANALYSIS\n" << color::RESET;
        std::cout << "  * Gross Wire Transferred : " << color::BOLD_YELLOW << total_bytes << " bytes" << color::RESET
                  << " (" << std::fixed << std::setprecision(2) << (total_bytes / 1024.0) << " KB)\n";
        std::cout << "  * Net Payload Data (L7)  : " << color::BOLD_GREEN << net_payload_bytes << " bytes" << color::RESET
                  << " (" << std::fixed << std::setprecision(2) << (net_payload_bytes / 1024.0) << " KB)\n";
        std::cout << "  * Framing Overhead (L2)  : " << header_overhead_bytes << " bytes (16-byte BDTP FrameHeader)\n";
        std::cout << "  * Framing Overhead Ratio : " << std::fixed << std::setprecision(2)
                  << header_overhead_pct() << " %\n";
        std::cout << "  * Gross Wire Throughput  : " << color::BOLD_CYAN << std::fixed << std::setprecision(2)
                  << gross_wire_rate_kbps() << " Kbps" << color::RESET << " ("
                  << (gross_wire_rate_kbps() / 8.0) << " KB/sec)\n";
        std::cout << "  * Net Effective Goodput  : " << color::BOLD_GREEN << std::fixed << std::setprecision(2)
                  << net_goodput_kbps() << " Kbps" << color::RESET << " ("
                  << (net_goodput_kbps() / 8.0) << " KB/sec)\n";
        std::cout << "--------------------------------------------------------------------------------\n";

        // 4. ARQ FLOW CONTROL & PACKET RELIABILITY
        std::cout << color::BOLD_WHITE << " [4] ARQ FLOW CONTROL & PACKET RELIABILITY (STOP-AND-WAIT)\n" << color::RESET;
        std::cout << "  * Total Transmissions    : " << total_packets_sent << " packets (initial + retries)\n";
        std::cout << "  * Inbound Received       : " << total_packets_received << " packets\n";
        std::cout << "  * Unique Frames Sent     : " << (total_packets_sent - total_retransmissions) << " packets\n";
        std::cout << "  * ARQ Retransmissions    : " << (total_retransmissions > 0 ? color::BOLD_YELLOW : color::GREEN)
                  << total_retransmissions << color::RESET << "\n";
        double retrans_rate = (total_packets_sent > 0) ? (static_cast<double>(total_retransmissions) / total_packets_sent) * 100.0 : 0.0;
        std::cout << "  * Retransmission Overhead: " << std::fixed << std::setprecision(2) << retrans_rate << " %\n";
        std::cout << "  * Stop-and-Wait Success  : " << color::BOLD_GREEN << (100.0 - retrans_rate) << " %" << color::RESET << "\n";
        std::cout << "--------------------------------------------------------------------------------\n";

        // 5. CHANNEL IMPAIRMENTS ENCOUNTERED
        std::cout << color::BOLD_WHITE << " [5] PHYSICAL CHANNEL IMPAIRMENTS ENCOUNTERED\n" << color::RESET;
        std::cout << "  * Packets Dropped (Loss) : " << (packets_lost > 0 ? color::RED : color::GREEN)
                  << packets_lost << color::RESET << "\n";
        std::cout << "  * Bit Corruptions (Noise): " << (packets_corrupted > 0 ? color::YELLOW : color::GREEN)
                  << packets_corrupted << color::RESET << "\n";
        std::cout << "  * Jitter / Delay Events  : " << (packets_delayed > 0 ? color::BLUE : color::GREEN)
                  << packets_delayed << color::RESET << "\n";
        std::cout << "--------------------------------------------------------------------------------\n";

        // 6. ERROR DETECTION & MATHEMATICAL VERIFICATION
        std::cout << color::BOLD_WHITE << " [6] ERROR DETECTION & FCS VERIFICATION VERDICT\n" << color::RESET;
        std::cout << "  * Total Injected Errors  : " << errors_injected << "\n";
        std::cout << "  * Detected by FCS/NACK   : " << color::BOLD_GREEN << errors_detected << color::RESET << "\n";
        std::cout << "  * Undetected (Blind Spots: " << (errors_undetected > 0 ? color::BOLD_RED : color::BOLD_GREEN)
                  << errors_undetected << color::RESET << "\n";
        if (errors_injected > 0) {
            double det_pct = (static_cast<double>(errors_detected) / errors_injected) * 100.0;
            std::cout << "  * Detection Success Rate : " << color::BOLD_GREEN << std::fixed << std::setprecision(2)
                      << det_pct << " %" << color::RESET << "\n";
        } else {
            std::cout << "  * Channel Condition      : " << color::BOLD_GREEN << "100% CLEAN (Zero bit corruption injected)" << color::RESET << "\n";
        }
        std::cout << "--------------------------------------------------------------------------------\n";

        // 7. LATENCY, RTT DISTRIBUTION & RFC 3550 JITTER
        if (!rtt_samples.empty()) {
            std::cout << color::BOLD_WHITE << " [7] LATENCY, RTT DISTRIBUTION & NETWORK JITTER (RFC 3550)\n" << color::RESET;
            std::cout << "  * Minimum RTT (Best)     : " << std::fixed << std::setprecision(3) << min_rtt() << " ms\n";
            std::cout << "  * Maximum RTT (Worst)    : " << std::fixed << std::setprecision(3) << max_rtt() << " ms\n";
            std::cout << "  * Mean / Average RTT     : " << color::BOLD_YELLOW << std::fixed << std::setprecision(3) << avg_rtt() << " ms" << color::RESET << "\n";
            std::cout << "  * Median RTT             : " << std::fixed << std::setprecision(3) << median_rtt() << " ms\n";
            std::cout << "  * RTT Standard Deviation : " << std::fixed << std::setprecision(3) << stddev_rtt() << " ms (sigma)\n";
            std::cout << "  * RFC 3550 Inter-Jitter  : " << color::BOLD_CYAN << std::fixed << std::setprecision(3) << jitter() << " ms" << color::RESET << "\n";
            std::cout << "  * Total Timing Samples   : " << rtt_samples.size() << " round-trip probes\n";
            std::cout << "--------------------------------------------------------------------------------\n";
        }

        // 8. ASCII VISUAL GAUGES & ALLOCATION BREAKDOWN
        double eff = efficiency();
        std::cout << color::BOLD_WHITE << " [8] VISUAL TRANSMISSION TELEMETRY GAUGES\n" << color::RESET;
        std::cout << "  * Throughput Efficiency  : " << color::BOLD_GREEN << std::fixed << std::setprecision(2)
                  << eff << " %\n" << color::RESET;
        std::cout << "    [";
        int bars = static_cast<int>(eff / 2.0);
        bars = std::max(0, std::min(50, bars));
        for (int i = 0; i < 50; ++i) {
            if (i < bars) std::cout << color::GREEN << "#" << color::RESET;
            else std::cout << " ";
        }
        std::cout << "] " << eff << " %\n\n";

        std::cout << "  * Frame Allocation Breakdown:\n";
        std::cout << "    Unique Frames    : [";
        for (int i = 0; i < 50; ++i) {
            if (i < bars) std::cout << color::CYAN << "=" << color::RESET;
            else std::cout << " ";
        }
        std::cout << "] " << static_cast<int>(eff) << " %\n";

        int retrans_bars = 50 - bars;
        std::cout << "    Retransmissions  : [";
        for (int i = 0; i < 50; ++i) {
            if (i < retrans_bars) std::cout << color::MAGENTA << "*" << color::RESET;
            else std::cout << " ";
        }
        std::cout << "] " << (100 - static_cast<int>(eff)) << " %\n\n";

        if (total_bytes > 0) {
            std::cout << "  * Wire Byte Distribution (BOOTP / DHCP / TELNET):\n";
            double bp_pct = (static_cast<double>(bootp_bytes) / total_bytes) * 100.0;
            double dh_pct = (static_cast<double>(dhcp_bytes) / total_bytes) * 100.0;
            double te_pct = (static_cast<double>(telnet_bytes) / total_bytes) * 100.0;
            int b_bars = static_cast<int>(bp_pct / 2.0);
            int d_bars = static_cast<int>(dh_pct / 2.0);
            int t_bars = 50 - b_bars - d_bars;
            if (t_bars < 0) t_bars = 0;

            std::cout << "    Bytes on Wire    : [";
            for (int i = 0; i < b_bars; ++i) std::cout << color::BLUE << "B" << color::RESET;
            for (int i = 0; i < d_bars; ++i) std::cout << color::YELLOW << "D" << color::RESET;
            for (int i = 0; i < t_bars; ++i) std::cout << color::GREEN << "T" << color::RESET;
            std::cout << "]\n";
            std::cout << "    Legend: " << color::BLUE << "[B] BOOTP: " << std::fixed << std::setprecision(1) << bp_pct << "%" << color::RESET
                      << " | " << color::YELLOW << "[D] DHCP: " << dh_pct << "%" << color::RESET
                      << " | " << color::GREEN << "[T] TELNET: " << te_pct << "%" << color::RESET << "\n";
        }

        std::cout << color::BOLD_CYAN
                  << "════════════════════════════════════════════════════════════════════════════════\n\n"
                  << color::RESET;
    }
};

} // namespace bdtp

