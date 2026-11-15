#include "common.hpp"
#include "error_detection.hpp"
#include "error_injection.hpp"
#include "channel.hpp"
#include "bootp.hpp"
#include "dhcp.hpp"
#include "telnet.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <cstring>
#include <random>

using namespace bdtp;

struct ClientConfig {
    std::string mode = "lifecycle";
    std::string server_ip = "127.0.0.1";
    int bootp_port = 6767;
    int dhcp_port = 6769;
    int telnet_port = 2323;
    std::string input_file = "input_commands.txt";
    int scheme_code = SCHEME_CRC32;
    int error_type = -1;
    double prob_loss = 0.0;
    double prob_error = 0.0;
    double prob_delay = 0.0;
    double prob_ack_loss = 0.0;
    int timeout_ms = 400;
    int max_retries = 5;
    bool interactive = false;
    bool step_mode = false;      // Pause between stages for demo
    int delay_ms = 0;           // Inter-packet pacing delay (ms)
    bool verbose_hex = false;   // Print raw hex dumps
};

static void print_usage(const char* prog) {
    std::cout << color::BOLD_WHITE << "Usage: " << prog << " [options]\n" << color::RESET
              << "  --mode <mode>         Execution mode: lifecycle (default), concurrent, bootp, dhcp, telnet\n"
              << "                        - lifecycle : Staged real-world sequence (BOOTP -> DHCP -> TELNET)\n"
              << "                        - concurrent: Execute all 3 protocols SIMULTANEOUSLY in parallel threads\n"
              << "                        - bootp     : Execute only BOOTP protocol\n"
              << "                        - dhcp      : Execute only DHCP protocol\n"
              << "                        - telnet    : Execute only TELNET protocol\n"
              << "  --step                Interactive Showcase Mode (pause & wait for ENTER between stages)\n"
              << "  --delay <ms>          Pacing delay in milliseconds between packets (e.g. 200 or 500)\n"
              << "  --hex / --verbose     Print raw packet hex dump on the wire\n"
              << "  --server-ip <ip>      Server IP address (default: 127.0.0.1)\n"
              << "  --bootp-port <port>   BOOTP UDP port (default: 6767)\n"
              << "  --dhcp-port <port>    DHCP UDP port (default: 6769)\n"
              << "  --telnet-port <port>  TELNET TCP port (default: 2323)\n"
              << "  --input <file>        Input commands/payload file (default: input_commands.txt)\n"
              << "  --scheme <1..7>       Error Detection Scheme (default: 5 [CRC-32])\n"
              << "                        1: Checksum-16, 2: CRC-8, 3: CRC-10,\n"
              << "                        4: CRC-16, 5: CRC-32, 6: Hamming, 7: Parity\n"
              << "  --error-type <0..7>   Error Injection Mode (-1 for cycle/random)\n"
              << "                        0: None, 1: Single-bit, 2: Two-isolated,\n"
              << "                        3: Odd bits, 4: Burst, 5: Checksum-blind,\n"
              << "                        6: CRC-blind, 7: Random byte\n"
              << "  --prob-loss <p>       Packet Loss probability [0.0 - 1.0] (default: 0.0)\n"
              << "  --prob-error <p>      Corruption probability [0.0 - 1.0] (default: 0.0)\n"
              << "  --prob-delay <p>      Delay/Jitter probability [0.0 - 1.0] (default: 0.0)\n"
              << "  --prob-ack-loss <p>   ACK Loss probability [0.0 - 1.0] (default: 0.0)\n"
              << "  --timeout-ms <ms>     Retransmission timeout in ms (default: 400)\n"
              << "  --max-retries <n>     Max ARQ retransmissions (default: 5)\n"
              << "  --interactive         Prompt interactively for all parameters\n";
}

static void wait_for_user_step(const std::string& next_stage_name) {
    std::cout << color::BOLD_YELLOW << "\n  >>> [SHOWCASE PAUSE] Press [ENTER] to advance to: "
              << color::BOLD_WHITE << next_stage_name << color::BOLD_YELLOW << " <<< " << color::RESET;
    std::string line;
    std::getline(std::cin, line);
    std::cout << "\n";
}

// ============================================================================
// STAGE 1: BOOTP PROTOCOL (UDP)
// ============================================================================
bool run_bootp_stage(const ClientConfig& cfg, const uint8_t client_mac[6],
                     SimulatedChannel& channel, Statistics& stats,
                     std::string& out_assigned_ip) {
    double t_stage_start = now_ms();
    std::cout << color::BOLD_CYAN
              << "┌──────────────────────────────────────────────────────────────────────────────┐\n"
              << "│ STAGE 1 / 4 : BOOTP NETWORK BOOTSTRAP INITIALIZATION (RFC 951 / RFC 1048)    │\n"
              << "│ Layer: Application (Bootstrap) | Transport: UDP Port " << std::left << std::setw(23) << cfg.bootp_port << "│\n"
              << "│ Role : Request bootloader filename & initial host network configuration     │\n"
              << "└──────────────────────────────────────────────────────────────────────────────┘\n"
              << color::RESET;

    int bootp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct timeval tv;
    tv.tv_sec = cfg.timeout_ms / 1000;
    tv.tv_usec = (cfg.timeout_ms % 1000) * 1000;
    setsockopt(bootp_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in srv_bootp_addr;
    std::memset(&srv_bootp_addr, 0, sizeof(srv_bootp_addr));
    srv_bootp_addr.sin_family = AF_INET;
    srv_bootp_addr.sin_port = htons(cfg.bootp_port);
    inet_pton(AF_INET, cfg.server_ip.c_str(), &srv_bootp_addr.sin_addr);

    uint32_t bootp_xid = 0xABCD1234;
    BootpPacket bootp_req = BootpPacket::create_request(client_mac, bootp_xid, "client-workstation");
    std::vector<uint8_t> bootp_payload = bootp_req.serialize();

    std::cout << color::BOLD_WHITE << "  [BOOTP Request Prepared]:\n" << color::RESET
              << "  * Client Hardware MAC   : " << mac_to_string(client_mac) << "\n"
              << "  * Transaction ID (XID)  : 0x" << std::hex << bootp_xid << std::dec << "\n"
              << "  * Client Hostname       : client-workstation\n"
              << "  * Serialization Length  : " << bootp_payload.size() << " bytes (RFC 951 compliant)\n";

    bool bootp_success = false;
    BootpPacket received_bootp_reply;
    uint16_t bootp_seq = 1;

    for (int retry = 0; retry <= cfg.max_retries && !bootp_success; ++retry) {
        if (retry > 0) {
            stats.total_retransmissions++;
            std::cout << color::BOLD_YELLOW << "  [BOOTP Timeout] Retransmitting BOOTREQUEST (Attempt "
                      << retry + 1 << "/" << cfg.max_retries + 1 << ")...\n" << color::RESET;
        }

        std::vector<uint8_t> frame(sizeof(FrameHeader) + bootp_payload.size());
        FrameHeader* hdr = reinterpret_cast<FrameHeader*>(frame.data());
        hdr->magic = BDTP_MAGIC;
        hdr->protocol_id = PROTO_BOOTP;
        hdr->seq_num = bootp_seq;
        hdr->ack_num = 0;
        hdr->detection_scheme = cfg.scheme_code;
        hdr->payload_len = static_cast<uint16_t>(bootp_payload.size());
        hdr->fcs = compute_fcs(bootp_payload.data(), bootp_payload.size(), cfg.scheme_code);
        std::memcpy(frame.data() + sizeof(FrameHeader), bootp_payload.data(), bootp_payload.size());

        if (cfg.verbose_hex) {
            std::cout << color::CYAN << "  [Outbound BOOTP Frame Hex Dump (" << frame.size() << " bytes)]:\n"
                      << color::RESET << hex_dump(frame.data(), std::min(frame.size(), (size_t)64), "    ");
        }

        ChannelOutcome outcome = channel.process_outgoing(
            frame.data() + sizeof(FrameHeader), hdr->payload_len, bootp_seq, PROTO_BOOTP
        );
        std::cout << "  " << outcome.summary << "\n";

        if (outcome.was_lost) {
            stats.packets_lost++;
            stats.record_packet_sent(PROTO_BOOTP, frame.size(), bootp_payload.size());
            continue;
        }
        if (outcome.was_corrupted) {
            stats.packets_corrupted++;
            stats.errors_injected++;
        }
        if (outcome.was_delayed) {
            stats.packets_delayed++;
        }

        double t_start = now_ms();
        sendto(bootp_sock, frame.data(), frame.size(), 0,
               (struct sockaddr*)&srv_bootp_addr, sizeof(srv_bootp_addr));
        stats.record_packet_sent(PROTO_BOOTP, frame.size(), bootp_payload.size());

        if (cfg.delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg.delay_ms));
        }

        uint8_t recv_buf[2048];
        socklen_t srv_len = sizeof(srv_bootp_addr);
        ssize_t n = recvfrom(bootp_sock, recv_buf, sizeof(recv_buf), 0,
                             (struct sockaddr*)&srv_bootp_addr, &srv_len);

        if (n > 0) {
            double rtt = now_ms() - t_start;
            stats.record_rtt(rtt);

            if (static_cast<size_t>(n) >= sizeof(FrameHeader)) {
                const FrameHeader* r_hdr = reinterpret_cast<const FrameHeader*>(recv_buf);
                stats.record_packet_received(PROTO_BOOTP, n, r_hdr->payload_len);

                if (r_hdr->magic == BDTP_MAGIC) {
                    if (r_hdr->protocol_id == PROTO_NACK) {
                        stats.errors_detected++;
                        std::cout << color::RED << "  [BOOTP] Received NACK from server (server detected corruption)\n" << color::RESET;
                        continue;
                    }

                    const uint8_t* pld = recv_buf + sizeof(FrameHeader);
                    bool ok = verify_fcs(pld, r_hdr->payload_len, r_hdr->fcs, r_hdr->detection_scheme);
                    if (ok) {
                        if (BootpPacket::deserialize(pld, r_hdr->payload_len, received_bootp_reply)) {
                            std::cout << color::BOLD_GREEN << "  [BOOTP SUCCESS] Valid BOOTREPLY Received in "
                                      << std::fixed << std::setprecision(2) << rtt << " ms!\n" << color::RESET
                                      << received_bootp_reply.to_string();
                            bootp_success = true;
                            out_assigned_ip = ip_to_string(received_bootp_reply.yiaddr);
                            if (outcome.was_corrupted) {
                                stats.errors_undetected++;
                                std::cout << color::BOLD_RED << "  [BLIND SPOT WARNING] Injected error went undetected by receiver!\n" << color::RESET;
                            }
                        }
                    } else {
                        stats.packets_corrupted++;
                        stats.errors_detected++;
                        std::cout << color::RED << "  [BOOTP FCS ERROR] Received reply corrupted in reverse path\n" << color::RESET;
                    }
                }
            }
        }
    }
    close(bootp_sock);
    stats.bootp_duration_ms = now_ms() - t_stage_start;
    return bootp_success;
}

// ============================================================================
// STAGE 2: DHCP PROTOCOL (DORA)
// ============================================================================
bool run_dhcp_stage(const ClientConfig& cfg, const uint8_t client_mac[6],
                    SimulatedChannel& channel, Statistics& stats,
                    uint32_t& out_leased_ip, uint32_t& out_server_id) {
    double t_stage_start = now_ms();
    std::cout << color::BOLD_CYAN
              << "┌──────────────────────────────────────────────────────────────────────────────┐\n"
              << "│ STAGE 2 / 4 : DHCP 4-WAY DORA LEASE ACQUISITION (RFC 2131 / RFC 2132)         │\n"
              << "│ Layer: Application (Host Config) | Transport: UDP Port " << std::left << std::setw(22) << cfg.dhcp_port << "│\n"
              << "│ State Flow: INIT -> DISCOVER -> OFFER -> REQUEST -> ACK -> BOUND             │\n"
              << "└──────────────────────────────────────────────────────────────────────────────┘\n"
              << color::RESET;

    int dhcp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct timeval tv;
    tv.tv_sec = cfg.timeout_ms / 1000;
    tv.tv_usec = (cfg.timeout_ms % 1000) * 1000;
    setsockopt(dhcp_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in srv_dhcp_addr;
    std::memset(&srv_dhcp_addr, 0, sizeof(srv_dhcp_addr));
    srv_dhcp_addr.sin_family = AF_INET;
    srv_dhcp_addr.sin_port = htons(cfg.dhcp_port);
    inet_pton(AF_INET, cfg.server_ip.c_str(), &srv_dhcp_addr.sin_addr);

    uint32_t dhcp_xid = 0x5678ABCD;
    uint16_t dhcp_seq = 10;

    // --- Step 2.1: DHCPDISCOVER -> DHCPOFFER ---
    std::cout << color::BOLD_WHITE << "  [DHCP Phase 1/4] Disagreeing on Static IP -> Broadcasting DHCPDISCOVER...\n" << color::RESET;
    bool offer_received = false;
    DhcpPacket received_offer;

    for (int retry = 0; retry <= cfg.max_retries && !offer_received; ++retry) {
        if (retry > 0) {
            stats.total_retransmissions++;
            std::cout << color::BOLD_YELLOW << "  [DHCP Timeout] Retransmitting DHCPDISCOVER (Attempt "
                      << retry + 1 << ")...\n" << color::RESET;
        }

        DhcpPacket discover = DhcpPacket::create_discover(client_mac, dhcp_xid, "client-node");
        std::vector<uint8_t> pld = discover.serialize();

        std::vector<uint8_t> frame(sizeof(FrameHeader) + pld.size());
        FrameHeader* hdr = reinterpret_cast<FrameHeader*>(frame.data());
        hdr->magic = BDTP_MAGIC;
        hdr->protocol_id = PROTO_DHCP;
        hdr->seq_num = dhcp_seq++;
        hdr->ack_num = 0;
        hdr->detection_scheme = cfg.scheme_code;
        hdr->payload_len = static_cast<uint16_t>(pld.size());
        hdr->fcs = compute_fcs(pld.data(), pld.size(), cfg.scheme_code);
        std::memcpy(frame.data() + sizeof(FrameHeader), pld.data(), pld.size());

        ChannelOutcome outcome = channel.process_outgoing(
            frame.data() + sizeof(FrameHeader), hdr->payload_len, hdr->seq_num, PROTO_DHCP
        );
        std::cout << "  " << outcome.summary << "\n";

        if (outcome.was_lost) {
            stats.packets_lost++;
            stats.record_packet_sent(PROTO_DHCP, frame.size(), pld.size());
            continue;
        }
        if (outcome.was_corrupted) {
            stats.packets_corrupted++;
            stats.errors_injected++;
        }
        if (outcome.was_delayed) {
            stats.packets_delayed++;
        }

        double t_start = now_ms();
        sendto(dhcp_sock, frame.data(), frame.size(), 0,
               (struct sockaddr*)&srv_dhcp_addr, sizeof(srv_dhcp_addr));
        stats.record_packet_sent(PROTO_DHCP, frame.size(), pld.size());

        if (cfg.delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg.delay_ms));
        }

        uint8_t recv_buf[2048];
        socklen_t srv_len = sizeof(srv_dhcp_addr);
        ssize_t n = recvfrom(dhcp_sock, recv_buf, sizeof(recv_buf), 0,
                             (struct sockaddr*)&srv_dhcp_addr, &srv_len);

        if (n > 0) {
            double rtt = now_ms() - t_start;
            stats.record_rtt(rtt);

            if (static_cast<size_t>(n) >= sizeof(FrameHeader)) {
                const FrameHeader* r_hdr = reinterpret_cast<const FrameHeader*>(recv_buf);
                stats.record_packet_received(PROTO_DHCP, n, r_hdr->payload_len);

                if (r_hdr->magic == BDTP_MAGIC) {
                    if (r_hdr->protocol_id == PROTO_NACK) {
                        stats.errors_detected++;
                        std::cout << color::RED << "  [DHCP NACK] Server rejected corrupted DHCP packet\n" << color::RESET;
                        continue;
                    }
                    if (r_hdr->protocol_id == PROTO_DHCP) {
                        const uint8_t* p = recv_buf + sizeof(FrameHeader);
                        if (verify_fcs(p, r_hdr->payload_len, r_hdr->fcs, r_hdr->detection_scheme)) {
                            if (DhcpPacket::deserialize(p, r_hdr->payload_len, received_offer) &&
                                received_offer.msg_type == DHCP_OFFER) {
                                std::cout << color::BOLD_GREEN << "  [DHCP Phase 2/4 SUCCESS] DHCPOFFER Received in "
                                          << std::fixed << std::setprecision(2) << rtt << " ms!\n" << color::RESET
                                          << received_offer.to_string();
                                out_leased_ip = received_offer.yiaddr;
                                out_server_id = received_offer.server_id;
                                offer_received = true;
                                if (outcome.was_corrupted) {
                                    stats.errors_undetected++;
                                    std::cout << color::BOLD_RED << "  [BLIND SPOT WARNING] Injected error went undetected by receiver!\n" << color::RESET;
                                }
                            }
                        } else {
                            stats.packets_corrupted++;
                            stats.errors_detected++;
                            std::cout << color::RED << "  [DHCP FCS ERROR] DHCPOFFER frame check failed\n" << color::RESET;
                        }
                    }
                }
            }
        }
    }

    if (!offer_received) {
        close(dhcp_sock);
        return false;
    }

    // --- Step 2.2: DHCPREQUEST -> DHCPACK ---
    std::cout << color::BOLD_WHITE << "  [DHCP Phase 3/4] Requesting Offered Lease -> Sending DHCPREQUEST for "
              << ip_to_string(out_leased_ip) << "...\n" << color::RESET;
    bool ack_received = false;
    DhcpPacket received_ack;

    for (int retry = 0; retry <= cfg.max_retries && !ack_received; ++retry) {
        if (retry > 0) {
            stats.total_retransmissions++;
            std::cout << color::BOLD_YELLOW << "  [DHCP Timeout] Retransmitting DHCPREQUEST (Attempt "
                      << retry + 1 << ")...\n" << color::RESET;
        }

        DhcpPacket request = DhcpPacket::create_request(client_mac, dhcp_xid, out_leased_ip, out_server_id, "client-node");
        std::vector<uint8_t> pld = request.serialize();

        std::vector<uint8_t> frame(sizeof(FrameHeader) + pld.size());
        FrameHeader* hdr = reinterpret_cast<FrameHeader*>(frame.data());
        hdr->magic = BDTP_MAGIC;
        hdr->protocol_id = PROTO_DHCP;
        hdr->seq_num = dhcp_seq++;
        hdr->ack_num = 0;
        hdr->detection_scheme = cfg.scheme_code;
        hdr->payload_len = static_cast<uint16_t>(pld.size());
        hdr->fcs = compute_fcs(pld.data(), pld.size(), cfg.scheme_code);
        std::memcpy(frame.data() + sizeof(FrameHeader), pld.data(), pld.size());

        ChannelOutcome outcome = channel.process_outgoing(
            frame.data() + sizeof(FrameHeader), hdr->payload_len, hdr->seq_num, PROTO_DHCP
        );
        std::cout << "  " << outcome.summary << "\n";

        if (outcome.was_lost) {
            stats.packets_lost++;
            stats.record_packet_sent(PROTO_DHCP, frame.size(), pld.size());
            continue;
        }
        if (outcome.was_corrupted) {
            stats.packets_corrupted++;
            stats.errors_injected++;
        }
        if (outcome.was_delayed) {
            stats.packets_delayed++;
        }

        double t_start = now_ms();
        sendto(dhcp_sock, frame.data(), frame.size(), 0,
               (struct sockaddr*)&srv_dhcp_addr, sizeof(srv_dhcp_addr));
        stats.record_packet_sent(PROTO_DHCP, frame.size(), pld.size());

        if (cfg.delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg.delay_ms));
        }

        uint8_t recv_buf[2048];
        socklen_t srv_len = sizeof(srv_dhcp_addr);
        ssize_t n = recvfrom(dhcp_sock, recv_buf, sizeof(recv_buf), 0,
                             (struct sockaddr*)&srv_dhcp_addr, &srv_len);

        if (n > 0) {
            double rtt = now_ms() - t_start;
            stats.record_rtt(rtt);

            if (static_cast<size_t>(n) >= sizeof(FrameHeader)) {
                const FrameHeader* r_hdr = reinterpret_cast<const FrameHeader*>(recv_buf);
                stats.record_packet_received(PROTO_DHCP, n, r_hdr->payload_len);

                if (r_hdr->magic == BDTP_MAGIC) {
                    if (r_hdr->protocol_id == PROTO_NACK) {
                        stats.errors_detected++;
                        std::cout << color::RED << "  [DHCP NACK] Server rejected corrupted DHCP packet\n" << color::RESET;
                        continue;
                    }
                    if (r_hdr->protocol_id == PROTO_DHCP) {
                        const uint8_t* p = recv_buf + sizeof(FrameHeader);
                        if (verify_fcs(p, r_hdr->payload_len, r_hdr->fcs, r_hdr->detection_scheme)) {
                            if (DhcpPacket::deserialize(p, r_hdr->payload_len, received_ack) &&
                                received_ack.msg_type == DHCP_ACK) {
                                std::cout << color::BOLD_GREEN << "  [DHCP Phase 4/4 SUCCESS] DHCPACK Received! Bound IP: "
                                          << ip_to_string(received_ack.yiaddr) << " in "
                                          << std::fixed << std::setprecision(2) << rtt << " ms!\n" << color::RESET
                                          << received_ack.to_string();
                                ack_received = true;
                                if (outcome.was_corrupted) {
                                    stats.errors_undetected++;
                                    std::cout << color::BOLD_RED << "  [BLIND SPOT WARNING] Injected error went undetected by receiver!\n" << color::RESET;
                                }
                            }
                        } else {
                            stats.packets_corrupted++;
                            stats.errors_detected++;
                            std::cout << color::RED << "  [DHCP FCS ERROR] DHCPACK frame check failed\n" << color::RESET;
                        }
                    }
                }
            }
        }
    }
    close(dhcp_sock);
    stats.dhcp_duration_ms = now_ms() - t_stage_start;
    return ack_received;
}

// ============================================================================
// STAGE 4: DHCP RELEASE (UDP)
// ============================================================================
void send_dhcp_release(const ClientConfig& cfg, const uint8_t client_mac[6],
                       uint32_t leased_ip, uint32_t server_id, Statistics& stats) {
    std::cout << color::BOLD_CYAN
              << "┌──────────────────────────────────────────────────────────────────────────────┐\n"
              << "│ STAGE 4 / 4 : DHCP LEASE TEARDOWN & CLEANUP (RFC 2131 Section 3.1.6)         │\n"
              << "│ Layer: Application (Host Teardown) | Transport: UDP Port " << std::left << std::setw(20) << cfg.dhcp_port << "│\n"
              << "│ Action: Unicast DHCPRELEASE -> Gracefully Return IP to Server Pool           │\n"
              << "└──────────────────────────────────────────────────────────────────────────────┘\n"
              << color::RESET;

    int dhcp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in srv_dhcp_addr;
    std::memset(&srv_dhcp_addr, 0, sizeof(srv_dhcp_addr));
    srv_dhcp_addr.sin_family = AF_INET;
    srv_dhcp_addr.sin_port = htons(cfg.dhcp_port);
    inet_pton(AF_INET, cfg.server_ip.c_str(), &srv_dhcp_addr.sin_addr);

    std::cout << color::BOLD_YELLOW << "  >>> Transmitting DHCPRELEASE for IP: "
              << ip_to_string(leased_ip) << " (Client MAC: " << mac_to_string(client_mac) << ") <<<\n" << color::RESET;
    DhcpPacket rel_pkt = DhcpPacket::create_release(client_mac, leased_ip, server_id, 0x5678ABCE);
    std::vector<uint8_t> rel_pld = rel_pkt.serialize();
    std::vector<uint8_t> rel_frame(sizeof(FrameHeader) + rel_pld.size());
    FrameHeader* r_hdr = reinterpret_cast<FrameHeader*>(rel_frame.data());
    r_hdr->magic = BDTP_MAGIC;
    r_hdr->protocol_id = PROTO_DHCP;
    r_hdr->seq_num = 50;
    r_hdr->ack_num = 0;
    r_hdr->detection_scheme = cfg.scheme_code;
    r_hdr->payload_len = static_cast<uint16_t>(rel_pld.size());
    r_hdr->fcs = compute_fcs(rel_pld.data(), rel_pld.size(), cfg.scheme_code);
    std::memcpy(rel_frame.data() + sizeof(FrameHeader), rel_pld.data(), rel_pld.size());

    sendto(dhcp_sock, rel_frame.data(), rel_frame.size(), 0,
           (struct sockaddr*)&srv_dhcp_addr, sizeof(srv_dhcp_addr));
    stats.record_packet_sent(PROTO_DHCP, rel_frame.size(), rel_pld.size());
    close(dhcp_sock);
    std::cout << color::BOLD_GREEN << "  [DHCP Release Confirmed] IP " << ip_to_string(leased_ip)
              << " returned to Server Pool.\n\n" << color::RESET;
}

// ============================================================================
// STAGE 3: TELNET APPLICATION PROTOCOL (TCP)
// ============================================================================
bool run_telnet_stage(const ClientConfig& cfg, SimulatedChannel& channel, Statistics& stats) {
    double t_stage_start = now_ms();
    std::cout << color::BOLD_CYAN
              << "┌──────────────────────────────────────────────────────────────────────────────┐\n"
              << "│ STAGE 3 / 4 : TELNET APPLICATION SESSION (RFC 854 / RFC 855)                  │\n"
              << "│ Layer: Application (NVT Terminal) | Transport: TCP Port " << std::left << std::setw(21) << cfg.telnet_port << "│\n"
              << "│ Features: 3-Way NVT Option Negotiation, ARQ Stop-and-Wait Data Streaming     │\n"
              << "└──────────────────────────────────────────────────────────────────────────────┘\n"
              << color::RESET;

    int telnet_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (telnet_sock < 0) {
        std::cerr << color::RED << "Error creating TELNET TCP socket\n" << color::RESET;
        return false;
    }

    struct sockaddr_in srv_telnet_addr;
    std::memset(&srv_telnet_addr, 0, sizeof(srv_telnet_addr));
    srv_telnet_addr.sin_family = AF_INET;
    srv_telnet_addr.sin_port = htons(cfg.telnet_port);
    inet_pton(AF_INET, cfg.server_ip.c_str(), &srv_telnet_addr.sin_addr);

    std::cout << color::BOLD_WHITE << "  [TCP 3-Way Handshake] Initiating SYN to "
              << cfg.server_ip << ":" << cfg.telnet_port << "...\n" << color::RESET;

    double t_conn_start = now_ms();
    if (connect(telnet_sock, (struct sockaddr*)&srv_telnet_addr, sizeof(srv_telnet_addr)) < 0) {
        std::cerr << color::BOLD_RED << "[FATAL] Failed to connect to TELNET server at "
                  << cfg.server_ip << ":" << cfg.telnet_port << "\n" << color::RESET;
        close(telnet_sock);
        return false;
    }
    double conn_rtt = now_ms() - t_conn_start;

    struct timeval tv_telnet;
    tv_telnet.tv_sec = cfg.timeout_ms / 1000;
    tv_telnet.tv_usec = (cfg.timeout_ms % 1000) * 1000;
    setsockopt(telnet_sock, SOL_SOCKET, SO_RCVTIMEO, &tv_telnet, sizeof(tv_telnet));

    std::cout << color::BOLD_GREEN << "  [TELNET TCP ESTABLISHED] Connected in "
              << std::fixed << std::setprecision(2) << conn_rtt << " ms!\n" << color::RESET;

    // Receive Server Option Negotiation & Welcome Banner
    uint8_t t_buf[2048];
    ssize_t tn = recv(telnet_sock, t_buf, sizeof(t_buf), 0);
    if (tn > 0) {
        TelnetParser parser;
        parser.parse(t_buf, tn);
        std::cout << color::CYAN << "  [TELNET Option Negotiation]: Received NVT Option Probes (" << tn << " bytes)\n" << color::RESET;
        
        std::vector<uint8_t> neg_reply;
        auto add_r = [&neg_reply](uint8_t verb, uint8_t opt) {
            auto b = build_telnet_negotiation(verb, opt);
            neg_reply.insert(neg_reply.end(), b.begin(), b.end());
        };
        add_r(TELNET_WILL, TELOPT_TTYPE);
        add_r(TELNET_DO, TELOPT_ECHO);
        add_r(TELNET_DO, TELOPT_SGA);
        send(telnet_sock, neg_reply.data(), neg_reply.size(), 0);
        std::cout << color::BOLD_GREEN << "  [TELNET Options Negotiated] IAC WILL TTYPE | IAC DO ECHO | IAC DO SGA\n" << color::RESET;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    {
        uint8_t banner_buf[2048];
        ssize_t bn = recv(telnet_sock, banner_buf, sizeof(banner_buf), MSG_DONTWAIT);
        if (bn > 0) {
            std::string b_str(reinterpret_cast<const char*>(banner_buf), bn);
            std::cout << color::CYAN << b_str << color::RESET;
        }
    }

    // Read input file
    std::ifstream infile(cfg.input_file, std::ios::binary);
    std::vector<std::string> lines_to_send;
    if (infile) {
        std::string line;
        while (std::getline(infile, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) lines_to_send.push_back(line);
        }
        infile.close();
    } else {
        lines_to_send = {
            "help", "network_info", "status", "ping", "calc 42 * 137",
            "echo [TELNET] Transmission pipeline validated", "stats", "exit"
        };
    }

    std::cout << color::BOLD_WHITE << "  [Transmission Script]: Loaded "
              << lines_to_send.size() << " records/commands from " << cfg.input_file << "\n\n" << color::RESET;

    uint16_t telnet_seq = 100;
    for (size_t idx = 0; idx < lines_to_send.size(); ++idx) {
        const std::string& cmd_line = lines_to_send[idx];
        bool is_data_stream = (cmd_line.rfind("DATA:", 0) == 0);
        uint8_t proto_sub = is_data_stream ? PROTO_TELNET_DATA : PROTO_TELNET_CONTROL;

        std::string payload_str = cmd_line;
        if (is_data_stream) {
            payload_str = cmd_line.substr(5);
        }

        bool packet_acked = false;
        for (int retry = 0; retry <= cfg.max_retries && !packet_acked; ++retry) {
            if (retry > 0) {
                stats.total_retransmissions++;
                std::cout << color::BOLD_YELLOW << "    [TELNET Retransmit] Frame #" << telnet_seq
                          << " (Attempt " << retry + 1 << ")...\n" << color::RESET;
            }

            std::vector<uint8_t> frame(sizeof(FrameHeader) + payload_str.size());
            FrameHeader* hdr = reinterpret_cast<FrameHeader*>(frame.data());
            hdr->magic = BDTP_MAGIC;
            hdr->protocol_id = proto_sub;
            hdr->seq_num = telnet_seq;
            hdr->ack_num = 0;
            hdr->detection_scheme = cfg.scheme_code;
            hdr->payload_len = static_cast<uint16_t>(payload_str.size());
            hdr->fcs = compute_fcs(reinterpret_cast<const uint8_t*>(payload_str.data()), payload_str.size(), cfg.scheme_code);
            std::memcpy(frame.data() + sizeof(FrameHeader), payload_str.data(), payload_str.size());

            ChannelOutcome outcome = channel.process_outgoing(
                frame.data() + sizeof(FrameHeader), hdr->payload_len, hdr->seq_num, proto_sub
            );
            std::cout << "  " << outcome.summary << "\n";

            if (outcome.was_lost) {
                stats.packets_lost++;
                stats.record_packet_sent(proto_sub, frame.size(), payload_str.size());
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            if (outcome.was_corrupted) {
                stats.packets_corrupted++;
                stats.errors_injected++;
            }
            if (outcome.was_delayed) {
                stats.packets_delayed++;
            }

            double t_start = now_ms();
            send(telnet_sock, frame.data(), frame.size(), 0);
            stats.record_packet_sent(proto_sub, frame.size(), payload_str.size());

            if (cfg.delay_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cfg.delay_ms));
            }

            uint8_t resp_buf[2048];
            ssize_t rn = recv(telnet_sock, resp_buf, sizeof(resp_buf), 0);
            if (rn > 0) {
                double rtt = now_ms() - t_start;
                stats.record_rtt(rtt);

                if (static_cast<size_t>(rn) >= sizeof(FrameHeader)) {
                    const FrameHeader* rh = reinterpret_cast<const FrameHeader*>(resp_buf);
                    stats.record_packet_received(rh->protocol_id, rn, rh->payload_len);

                    if (rh->magic == BDTP_MAGIC) {
                        if (rh->protocol_id == PROTO_NACK) {
                            stats.errors_detected++;
                            std::cout << color::RED << "    [TELNET NACK] Server rejected corrupted frame #"
                                      << rh->seq_num << "\n" << color::RESET;
                            continue;
                        }
                        if (rh->protocol_id == PROTO_ACK) {
                            std::cout << color::BOLD_GREEN << "    [TELNET ACK #" << rh->seq_num
                                      << "] Confirmed in " << std::fixed << std::setprecision(2) << rtt << " ms\n" << color::RESET;
                            packet_acked = true;
                            if (outcome.was_corrupted) {
                                stats.errors_undetected++;
                                std::cout << color::BOLD_RED << "    [BLIND SPOT WARNING] Injected error went undetected by receiver!\n" << color::RESET;
                            }
                            break;
                        }
                        if (rh->protocol_id == PROTO_TELNET_CONTROL) {
                            const uint8_t* p = resp_buf + sizeof(FrameHeader);
                            if (verify_fcs(p, rh->payload_len, rh->fcs, rh->detection_scheme)) {
                                std::string reply(reinterpret_cast<const char*>(p), rh->payload_len);
                                std::cout << color::GREEN << "  [Remote Exec Reply]:\n" << color::RESET << reply;
                                packet_acked = true;
                                if (outcome.was_corrupted) {
                                    stats.errors_undetected++;
                                    std::cout << color::BOLD_RED << "    [BLIND SPOT WARNING] Injected error went undetected by receiver!\n" << color::RESET;
                                }
                                break;
                            } else {
                                stats.packets_corrupted++;
                                stats.errors_detected++;
                                std::cout << color::RED << "    [TELNET Reply Corrupted]\n" << color::RESET;
                            }
                        }
                    }
                } else {
                    std::string raw_reply(reinterpret_cast<const char*>(resp_buf), rn);
                    std::cout << color::GREEN << raw_reply << color::RESET;
                    stats.record_packet_received(proto_sub, rn, rn);
                    packet_acked = true;
                    break;
                }
            }
        }
        telnet_seq++;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    close(telnet_sock);
    stats.telnet_duration_ms = now_ms() - t_stage_start;
    return true;
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
int main(int argc, char* argv[]) {
    ClientConfig cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--mode" && i + 1 < argc) cfg.mode = argv[++i];
        else if (arg == "--server-ip" && i + 1 < argc) cfg.server_ip = argv[++i];
        else if (arg == "--bootp-port" && i + 1 < argc) cfg.bootp_port = std::stoi(argv[++i]);
        else if (arg == "--dhcp-port" && i + 1 < argc) cfg.dhcp_port = std::stoi(argv[++i]);
        else if (arg == "--telnet-port" && i + 1 < argc) cfg.telnet_port = std::stoi(argv[++i]);
        else if (arg == "--input" && i + 1 < argc) cfg.input_file = argv[++i];
        else if (arg == "--scheme" && i + 1 < argc) cfg.scheme_code = std::stoi(argv[++i]);
        else if (arg == "--error-type" && i + 1 < argc) cfg.error_type = std::stoi(argv[++i]);
        else if (arg == "--prob-loss" && i + 1 < argc) cfg.prob_loss = std::stod(argv[++i]);
        else if (arg == "--prob-error" && i + 1 < argc) cfg.prob_error = std::stod(argv[++i]);
        else if (arg == "--prob-delay" && i + 1 < argc) cfg.prob_delay = std::stod(argv[++i]);
        else if (arg == "--prob-ack-loss" && i + 1 < argc) cfg.prob_ack_loss = std::stod(argv[++i]);
        else if (arg == "--timeout-ms" && i + 1 < argc) cfg.timeout_ms = std::stoi(argv[++i]);
        else if (arg == "--max-retries" && i + 1 < argc) cfg.max_retries = std::stoi(argv[++i]);
        else if (arg == "--delay" && i + 1 < argc) cfg.delay_ms = std::stoi(argv[++i]);
        else if (arg == "--step") cfg.step_mode = true;
        else if (arg == "--hex" || arg == "--verbose") cfg.verbose_hex = true;
        else if (arg == "--interactive") cfg.interactive = true;
        else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (cfg.interactive) {
        std::cout << color::BOLD_WHITE << "=== Interactive Configuration Setup ===\n" << color::RESET;
        std::cout << "Select Execution Mode:\n"
                  << "  1. lifecycle  (Sequential: BOOTP -> DHCP -> TELNET)\n"
                  << "  2. concurrent (Simultaneous: All 3 running in parallel threads)\n"
                  << "  3. bootp      (Only BOOTP)\n"
                  << "  4. dhcp       (Only DHCP)\n"
                  << "  5. telnet     (Only TELNET)\n"
                  << "Choice [1-5, default 1]: ";
        std::string smode;
        std::getline(std::cin, smode);
        if (smode == "2") cfg.mode = "concurrent";
        else if (smode == "3") cfg.mode = "bootp";
        else if (smode == "4") cfg.mode = "dhcp";
        else if (smode == "5") cfg.mode = "telnet";
        else cfg.mode = "lifecycle";

        std::cout << "Enter Server IP [" << cfg.server_ip << "]: ";
        std::string s;
        std::getline(std::cin, s);
        if (!s.empty()) cfg.server_ip = s;

        std::cout << "Select Error Detection Scheme:\n"
                  << "  1. 16-bit Checksum (RFC 1071)\n"
                  << "  2. CRC-8\n"
                  << "  3. CRC-10\n"
                  << "  4. CRC-16\n"
                  << "  5. CRC-32 (IEEE 802.3 Ethernet)\n"
                  << "  6. Hamming SEC-DED Code\n"
                  << "  7. 2D Parity (LRC+VRC)\n"
                  << "Choice [1-7, default " << cfg.scheme_code << "]: ";
        std::getline(std::cin, s);
        if (!s.empty()) cfg.scheme_code = std::stoi(s);

        std::cout << "Enter Probability of Packet Loss [0.0 - 1.0, default " << cfg.prob_loss << "]: ";
        std::getline(std::cin, s);
        if (!s.empty()) cfg.prob_loss = std::stod(s);

        std::cout << "Enter Probability of Bit Corruption [0.0 - 1.0, default " << cfg.prob_error << "]: ";
        std::getline(std::cin, s);
        if (!s.empty()) cfg.prob_error = std::stod(s);

        std::cout << "Enter Input File [" << cfg.input_file << "]: ";
        std::getline(std::cin, s);
        if (!s.empty()) cfg.input_file = s;

        std::cout << "Enable Step-by-Step Interactive Showcase Mode? (y/N): ";
        std::getline(std::cin, s);
        if (s == "y" || s == "Y") cfg.step_mode = true;
    }

    std::cout << color::BOLD_CYAN
              << "================================================================================\n"
              << "        MINI-PROJECT CLIENT NODE: BOOTP | DHCP | TELNET ENGINE                  \n"
              << "================================================================================\n"
              << color::RESET;
    std::cout << " Execution Mode          : " << color::BOLD_YELLOW << cfg.mode << color::RESET;
    if (cfg.mode == "lifecycle") std::cout << " (Staged sequential pipeline)\n";
    else if (cfg.mode == "concurrent") std::cout << " (SIMULTANEOUS PARALLEL THREADS)\n";
    else std::cout << " (Standalone)\n";

    std::cout << " Target Server IP Address: " << color::BOLD_GREEN << cfg.server_ip << color::RESET << "\n";
    std::cout << " Ports                   : BOOTP=" << cfg.bootp_port << ", DHCP=" << cfg.dhcp_port << ", TELNET=" << cfg.telnet_port << "\n";
    std::cout << " Error Detection Scheme  : " << color::BOLD_MAGENTA << get_scheme_name(cfg.scheme_code) << color::RESET << "\n";
    std::cout << " Impairment Rates        : Loss=" << cfg.prob_loss << ", Error=" << cfg.prob_error
              << ", Delay=" << cfg.prob_delay << ", ACK-Loss=" << cfg.prob_ack_loss << "\n";
    std::cout << " Showcase Options        : StepMode=" << (cfg.step_mode ? "ON (Wait for ENTER)" : "OFF")
              << ", PacketDelay=" << cfg.delay_ms << "ms, VerboseHex=" << (cfg.verbose_hex ? "ON" : "OFF") << "\n";
    std::cout << " Input Transmission File : " << color::YELLOW << cfg.input_file << color::RESET << "\n";
    std::cout << "--------------------------------------------------------------------------------\n\n";

    Statistics stats("Client Node (" + cfg.mode + ")");
    stats.start();

    SimulatedChannel channel(cfg.prob_loss, cfg.prob_error, cfg.prob_delay, cfg.prob_ack_loss);
    channel.fixed_error_type = cfg.error_type;
    channel.detection_scheme = cfg.scheme_code;

    uint8_t client_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
    std::string assigned_bootp_ip;
    uint32_t leased_dhcp_ip = 0;
    uint32_t dhcp_srv_id = 0;

    if (cfg.mode == "lifecycle") {
        // Sequential Host Lifecycle with Showcase Step support
        if (cfg.step_mode) wait_for_user_step("STAGE 1: BOOTP Network Bootstrap");
        bool ok = run_bootp_stage(cfg, client_mac, channel, stats, assigned_bootp_ip);
        if (!ok) {
            std::cerr << color::BOLD_RED << "[FATAL] BOOTP stage failed.\n" << color::RESET;
            return 1;
        }

        std::cout << "\n";
        if (cfg.step_mode) wait_for_user_step("STAGE 2: DHCP 4-Way DORA Lease Acquisition");
        ok = run_dhcp_stage(cfg, client_mac, channel, stats, leased_dhcp_ip, dhcp_srv_id);
        if (!ok) {
            std::cerr << color::BOLD_RED << "[FATAL] DHCP stage failed.\n" << color::RESET;
            return 1;
        }

        std::cout << "\n";
        if (cfg.step_mode) wait_for_user_step("STAGE 3: TELNET Remote Application Session");
        run_telnet_stage(cfg, channel, stats);

        std::cout << "\n";
        if (cfg.step_mode) wait_for_user_step("STAGE 4: DHCP Teardown (DHCPRELEASE)");
        send_dhcp_release(cfg, client_mac, leased_dhcp_ip, dhcp_srv_id, stats);

        if (cfg.step_mode) wait_for_user_step("STAGE 5: Comprehensive Telemetry Dashboard");
    }
    else if (cfg.mode == "concurrent") {
        // SIMULTANEOUS / PARALLEL EXECUTION: All 3 protocols execute concurrently in parallel threads!
        std::cout << color::BOLD_YELLOW << ">>> LAUNCHING BOOTP, DHCP, AND TELNET SIMULTANEOUSLY IN PARALLEL THREADS <<<\n\n" << color::RESET;
        
        Statistics s_bootp("BOOTP-Thread");
        Statistics s_dhcp("DHCP-Thread");
        Statistics s_telnet("TELNET-Thread");
        s_bootp.start(); s_dhcp.start(); s_telnet.start();

        std::string ip_b;
        uint32_t ip_d = 0, sid_d = 0;

        std::thread t_bootp([&]() {
            run_bootp_stage(cfg, client_mac, channel, s_bootp, ip_b);
            s_bootp.stop();
        });

        std::thread t_dhcp([&]() {
            run_dhcp_stage(cfg, client_mac, channel, s_dhcp, ip_d, sid_d);
            send_dhcp_release(cfg, client_mac, ip_d, sid_d, s_dhcp);
            s_dhcp.stop();
        });

        std::thread t_telnet([&]() {
            run_telnet_stage(cfg, channel, s_telnet);
            s_telnet.stop();
        });

        t_bootp.join();
        t_dhcp.join();
        t_telnet.join();

        stats.merge(s_bootp);
        stats.merge(s_dhcp);
        stats.merge(s_telnet);
    }
    else if (cfg.mode == "bootp") {
        run_bootp_stage(cfg, client_mac, channel, stats, assigned_bootp_ip);
    }
    else if (cfg.mode == "dhcp") {
        run_dhcp_stage(cfg, client_mac, channel, stats, leased_dhcp_ip, dhcp_srv_id);
        send_dhcp_release(cfg, client_mac, leased_dhcp_ip, dhcp_srv_id, stats);
    }
    else if (cfg.mode == "telnet") {
        run_telnet_stage(cfg, channel, stats);
    }

    stats.stop();
    stats.print_summary("CLIENT COMPLETE END-OF-RUN REPORT: CROSS-LAYER TELEMETRY DASHBOARD");
    return 0;
}
