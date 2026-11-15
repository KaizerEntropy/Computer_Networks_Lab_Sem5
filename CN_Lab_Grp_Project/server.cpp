#include "common.hpp"
#include "error_detection.hpp"
#include "error_injection.hpp"
#include "channel.hpp"
#include "bootp.hpp"
#include "dhcp.hpp"
#include "telnet.hpp"

#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <csignal>
#include <fcntl.h>
#include <sys/select.h>

using namespace bdtp;

static std::atomic<bool> server_running(true);

void sigint_handler(int) {
    server_running = false;
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, sigint_handler);

    std::string bind_ip = "0.0.0.0";
    int bootp_port = 6767;
    int dhcp_port = 6769;
    int telnet_port = 2323;
    int scheme_code = SCHEME_CRC32;
    bool verbose_hex = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--ip" && i + 1 < argc) {
            bind_ip = argv[++i];
        } else if (arg == "--bootp-port" && i + 1 < argc) {
            bootp_port = std::stoi(argv[++i]);
        } else if (arg == "--dhcp-port" && i + 1 < argc) {
            dhcp_port = std::stoi(argv[++i]);
        } else if (arg == "--telnet-port" && i + 1 < argc) {
            telnet_port = std::stoi(argv[++i]);
        } else if (arg == "--scheme" && i + 1 < argc) {
            scheme_code = std::stoi(argv[++i]);
        } else if (arg == "--hex" || arg == "--verbose") {
            verbose_hex = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << color::BOLD_WHITE << "Usage: ./server [options]\n" << color::RESET
                      << "  --ip <address>        IP address to bind (default: 0.0.0.0)\n"
                      << "  --bootp-port <port>   BOOTP UDP port (default: 6767)\n"
                      << "  --dhcp-port <port>    DHCP UDP port (default: 6769)\n"
                      << "  --telnet-port <port>  TELNET TCP port (default: 2323)\n"
                      << "  --hex / --verbose     Print raw wire hex dump of packets\n"
                      << "  --scheme <1..7>       Error detection scheme (default: 5 [CRC-32])\n"
                      << "                        1: Checksum-16, 2: CRC-8, 3: CRC-10,\n"
                      << "                        4: CRC-16, 5: CRC-32, 6: Hamming, 7: Parity\n";
            return 0;
        }
    }

    std::cout << color::BOLD_CYAN
              << "================================================================================\n"
              << "          MINI-PROJECT MULTI-PROTOCOL SERVER DAEMON (BOOTP, DHCP, TELNET)       \n"
              << "================================================================================\n"
              << color::RESET;
    std::cout << " Binding IP Address     : " << color::BOLD_GREEN << bind_ip << color::RESET << " (Allows multi-device connections)\n";
    std::cout << " BOOTP UDP Service Port : " << color::YELLOW << bootp_port << color::RESET << "\n";
    std::cout << " DHCP UDP Service Port  : " << color::YELLOW << dhcp_port << color::RESET << "\n";
    std::cout << " TELNET TCP Service Port: " << color::YELLOW << telnet_port << color::RESET << "\n";
    std::cout << " Active Error Scheme    : " << color::BOLD_MAGENTA << get_scheme_name(scheme_code) << color::RESET << "\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    Statistics stats("Multi-Protocol Server");
    stats.start();

    // 1. Setup BOOTP UDP Socket
    int bootp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (bootp_sock < 0) {
        std::cerr << color::RED << "Error creating BOOTP socket\n" << color::RESET;
        return 1;
    }
    int optval = 1;
    setsockopt(bootp_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in bootp_addr;
    std::memset(&bootp_addr, 0, sizeof(bootp_addr));
    bootp_addr.sin_family = AF_INET;
    bootp_addr.sin_port = htons(bootp_port);
    inet_pton(AF_INET, bind_ip.c_str(), &bootp_addr.sin_addr);

    if (bind(bootp_sock, (struct sockaddr*)&bootp_addr, sizeof(bootp_addr)) < 0) {
        std::cerr << color::RED << "Error binding BOOTP socket to port " << bootp_port << "\n" << color::RESET;
        close(bootp_sock);
        return 1;
    }

    // 2. Setup DHCP UDP Socket
    int dhcp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (dhcp_sock < 0) {
        std::cerr << color::RED << "Error creating DHCP socket\n" << color::RESET;
        close(bootp_sock);
        return 1;
    }
    setsockopt(dhcp_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in dhcp_addr;
    std::memset(&dhcp_addr, 0, sizeof(dhcp_addr));
    dhcp_addr.sin_family = AF_INET;
    dhcp_addr.sin_port = htons(dhcp_port);
    inet_pton(AF_INET, bind_ip.c_str(), &dhcp_addr.sin_addr);

    if (bind(dhcp_sock, (struct sockaddr*)&dhcp_addr, sizeof(dhcp_addr)) < 0) {
        std::cerr << color::RED << "Error binding DHCP socket to port " << dhcp_port << "\n" << color::RESET;
        close(bootp_sock);
        close(dhcp_sock);
        return 1;
    }

    // 3. Setup TELNET TCP Listening Socket
    int telnet_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (telnet_listen_sock < 0) {
        std::cerr << color::RED << "Error creating TELNET socket\n" << color::RESET;
        close(bootp_sock);
        close(dhcp_sock);
        return 1;
    }
    setsockopt(telnet_listen_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in telnet_addr;
    std::memset(&telnet_addr, 0, sizeof(telnet_addr));
    telnet_addr.sin_family = AF_INET;
    telnet_addr.sin_port = htons(telnet_port);
    inet_pton(AF_INET, bind_ip.c_str(), &telnet_addr.sin_addr);

    if (bind(telnet_listen_sock, (struct sockaddr*)&telnet_addr, sizeof(telnet_addr)) < 0) {
        std::cerr << color::RED << "Error binding TELNET socket to port " << telnet_port << "\n" << color::RESET;
        close(bootp_sock);
        close(dhcp_sock);
        close(telnet_listen_sock);
        return 1;
    }

    if (listen(telnet_listen_sock, 5) < 0) {
        std::cerr << color::RED << "Error listening on TELNET socket\n" << color::RESET;
        close(bootp_sock);
        close(dhcp_sock);
        close(telnet_listen_sock);
        return 1;
    }

    // Initialize DHCP Lease Pool
    uint32_t srv_ip   = string_to_ip("192.168.10.1");
    uint32_t mask     = string_to_ip("255.255.255.0");
    uint32_t def_gw   = string_to_ip("192.168.10.1");
    uint32_t dns      = string_to_ip("192.168.10.1");
    uint32_t pool_st  = string_to_ip("192.168.10.100");
    uint32_t pool_end = string_to_ip("192.168.10.200");
    DhcpLeasePool lease_pool(srv_ip, mask, def_gw, dns, pool_st, pool_end, 7200);

    // Context for Telnet session
    RemoteCommandContext telnet_ctx;
    telnet_ctx.client_mac = "00:00:00:00:00:00";
    telnet_ctx.gateway_ip = "192.168.10.1";
    telnet_ctx.dns_ip = "192.168.10.1";
    telnet_ctx.lease_secs = 7200;
    telnet_ctx.commands_executed = 0;

    std::cout << color::BOLD_GREEN << "[Server Daemon Ready] Listening for incoming network traffic...\n" << color::RESET << std::endl;

    while (server_running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(bootp_sock, &read_fds);
        FD_SET(dhcp_sock, &read_fds);
        FD_SET(telnet_listen_sock, &read_fds);

        int max_fd = std::max({bootp_sock, dhcp_sock, telnet_listen_sock});

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        if (activity < 0) {
            if (server_running) break;
            else break;
        }
        if (activity == 0) continue;

        // --------------------------------------------------------------------
        // Handle BOOTP Requests (UDP)
        // --------------------------------------------------------------------
        if (FD_ISSET(bootp_sock, &read_fds)) {
            uint8_t buffer[2048];
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            ssize_t bytes_recv = recvfrom(bootp_sock, buffer, sizeof(buffer), 0,
                                          (struct sockaddr*)&client_addr, &addr_len);
            if (bytes_recv > 0) {
                char cli_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, cli_ip, sizeof(cli_ip));
                int cli_port = ntohs(client_addr.sin_port);

                std::cout << "\n" << color::BOLD_BLUE << "[" << current_timestamp() << "]"
                          << " [BOOTP] Received datagram (" << bytes_recv << " bytes) from "
                          << cli_ip << ":" << cli_port << color::RESET << "\n";

                // Check encapsulation frame
                if (static_cast<size_t>(bytes_recv) >= sizeof(FrameHeader)) {
                    const FrameHeader* hdr = reinterpret_cast<const FrameHeader*>(buffer);
                    stats.record_packet_received(PROTO_BOOTP, bytes_recv, hdr->payload_len);
                    if (hdr->magic == BDTP_MAGIC) {
                        const uint8_t* payload = buffer + sizeof(FrameHeader);
                        size_t payload_len = hdr->payload_len;

                        if (verbose_hex) {
                            std::cout << color::CYAN << "  [Inbound BOOTP Hex Dump]:\n" << color::RESET
                                      << hex_dump(buffer, std::min((size_t)bytes_recv, (size_t)64), "    ");
                        }

                        // Verify FCS
                        bool fcs_valid = verify_fcs(payload, payload_len, hdr->fcs, hdr->detection_scheme);
                        if (!fcs_valid) {
                            stats.packets_corrupted++;
                            stats.errors_detected++;
                            std::cout << color::BOLD_RED << "  [FCS ERROR] Frame Check Sequence verification FAILED for BOOTP packet! Corrupted frame rejected.\n"
                                      << color::RESET;
                            
                            // Send NACK
                            FrameHeader nack_hdr;
                            nack_hdr.magic = BDTP_MAGIC;
                            nack_hdr.protocol_id = PROTO_NACK;
                            nack_hdr.seq_num = hdr->seq_num;
                            nack_hdr.ack_num = hdr->seq_num;
                            nack_hdr.detection_scheme = hdr->detection_scheme;
                            nack_hdr.payload_len = 0;
                            nack_hdr.fcs = 0;
                            sendto(bootp_sock, &nack_hdr, sizeof(nack_hdr), 0, (struct sockaddr*)&client_addr, addr_len);
                            stats.record_packet_sent(PROTO_NACK, sizeof(nack_hdr), 0);
                            continue;
                        }

                        BootpPacket bootp_req;
                        if (BootpPacket::deserialize(payload, payload_len, bootp_req)) {
                            std::cout << color::CYAN << "  [BOOTP Request Details]:\n" << color::RESET
                                      << bootp_req.to_string();

                            uint32_t assigned_ip = string_to_ip("192.168.10.50");
                            telnet_ctx.client_mac = mac_to_string(bootp_req.mac);
                            telnet_ctx.bootp_ip = ip_to_string(assigned_ip);
                            telnet_ctx.boot_file = "pxelinux.0";

                            // Generate BOOTP Reply
                            BootpPacket bootp_reply = BootpPacket::create_reply(
                                bootp_req, assigned_ip, srv_ip, def_gw, mask, "pxelinux.0"
                            );

                            std::vector<uint8_t> reply_payload = bootp_reply.serialize();

                            // Encapsulate into FrameHeader
                            std::vector<uint8_t> send_buf(sizeof(FrameHeader) + reply_payload.size());
                            FrameHeader* out_hdr = reinterpret_cast<FrameHeader*>(send_buf.data());
                            out_hdr->magic = BDTP_MAGIC;
                            out_hdr->protocol_id = PROTO_BOOTP;
                            out_hdr->seq_num = hdr->seq_num;
                            out_hdr->ack_num = hdr->seq_num;
                            out_hdr->detection_scheme = scheme_code;
                            out_hdr->payload_len = static_cast<uint16_t>(reply_payload.size());
                            out_hdr->fcs = compute_fcs(reply_payload.data(), reply_payload.size(), scheme_code);

                            std::memcpy(send_buf.data() + sizeof(FrameHeader), reply_payload.data(), reply_payload.size());

                            sendto(bootp_sock, send_buf.data(), send_buf.size(), 0,
                                   (struct sockaddr*)&client_addr, addr_len);

                            stats.record_packet_sent(PROTO_BOOTP, send_buf.size(), reply_payload.size());

                            std::cout << color::BOLD_GREEN << "  [BOOTP Reply Dispatched] Assigned IP: "
                                      << ip_to_string(assigned_ip) << ", Boot file: pxelinux.0" << color::RESET << "\n";
                        }
                    }
                }
            }
        }

        // --------------------------------------------------------------------
        // Handle DHCP Requests (UDP)
        // --------------------------------------------------------------------
        if (FD_ISSET(dhcp_sock, &read_fds)) {
            uint8_t buffer[2048];
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            ssize_t bytes_recv = recvfrom(dhcp_sock, buffer, sizeof(buffer), 0,
                                          (struct sockaddr*)&client_addr, &addr_len);
            if (bytes_recv > 0) {
                char cli_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, cli_ip, sizeof(cli_ip));
                int cli_port = ntohs(client_addr.sin_port);

                std::cout << "\n" << color::BOLD_MAGENTA << "[" << current_timestamp() << "]"
                          << " [DHCP] Received datagram (" << bytes_recv << " bytes) from "
                          << cli_ip << ":" << cli_port << color::RESET << "\n";

                if (static_cast<size_t>(bytes_recv) >= sizeof(FrameHeader)) {
                    const FrameHeader* hdr = reinterpret_cast<const FrameHeader*>(buffer);
                    stats.record_packet_received(PROTO_DHCP, bytes_recv, hdr->payload_len);
                    if (hdr->magic == BDTP_MAGIC) {
                        const uint8_t* payload = buffer + sizeof(FrameHeader);
                        size_t payload_len = hdr->payload_len;

                        if (verbose_hex) {
                            std::cout << color::MAGENTA << "  [Inbound DHCP Hex Dump]:\n" << color::RESET
                                      << hex_dump(buffer, std::min((size_t)bytes_recv, (size_t)64), "    ");
                        }

                        bool fcs_valid = verify_fcs(payload, payload_len, hdr->fcs, hdr->detection_scheme);
                        if (!fcs_valid) {
                            stats.packets_corrupted++;
                            stats.errors_detected++;
                            std::cout << color::BOLD_RED << "  [FCS ERROR] DHCP packet checksum verification FAILED! Discarded.\n" << color::RESET;
                            
                            FrameHeader nack_hdr;
                            nack_hdr.magic = BDTP_MAGIC;
                            nack_hdr.protocol_id = PROTO_NACK;
                            nack_hdr.seq_num = hdr->seq_num;
                            nack_hdr.ack_num = hdr->seq_num;
                            nack_hdr.detection_scheme = hdr->detection_scheme;
                            nack_hdr.payload_len = 0;
                            nack_hdr.fcs = 0;
                            sendto(dhcp_sock, &nack_hdr, sizeof(nack_hdr), 0, (struct sockaddr*)&client_addr, addr_len);
                            stats.record_packet_sent(PROTO_NACK, sizeof(nack_hdr), 0);
                            continue;
                        }

                        DhcpPacket dhcp_pkt;
                        if (DhcpPacket::deserialize(payload, payload_len, dhcp_pkt)) {
                            std::cout << color::MAGENTA << "  [DHCP Packet Received]:\n" << color::RESET
                                      << dhcp_pkt.to_string();

                            telnet_ctx.client_mac = mac_to_string(dhcp_pkt.mac);

                            if (dhcp_pkt.msg_type == DHCP_DISCOVER) {
                                // Allocate DHCPOFFER
                                uint32_t offered_ip = lease_pool.allocate_offer(dhcp_pkt.mac, dhcp_pkt.requested_ip);
                                DhcpPacket offer_pkt = DhcpPacket::create_offer(
                                    dhcp_pkt, offered_ip, srv_ip, mask, def_gw, dns, 7200
                                );

                                std::vector<uint8_t> offer_payload = offer_pkt.serialize();
                                std::vector<uint8_t> send_buf(sizeof(FrameHeader) + offer_payload.size());
                                FrameHeader* out_hdr = reinterpret_cast<FrameHeader*>(send_buf.data());
                                out_hdr->magic = BDTP_MAGIC;
                                out_hdr->protocol_id = PROTO_DHCP;
                                out_hdr->seq_num = hdr->seq_num;
                                out_hdr->ack_num = hdr->seq_num;
                                out_hdr->detection_scheme = scheme_code;
                                out_hdr->payload_len = static_cast<uint16_t>(offer_payload.size());
                                out_hdr->fcs = compute_fcs(offer_payload.data(), offer_payload.size(), scheme_code);

                                std::memcpy(send_buf.data() + sizeof(FrameHeader), offer_payload.data(), offer_payload.size());

                                sendto(dhcp_sock, send_buf.data(), send_buf.size(), 0,
                                       (struct sockaddr*)&client_addr, addr_len);

                                stats.record_packet_sent(PROTO_DHCP, send_buf.size(), offer_payload.size());

                                std::cout << color::BOLD_GREEN << "  [DHCP Phase 2] Dispatched DHCPOFFER ("
                                          << ip_to_string(offered_ip) << ") to " << mac_to_string(dhcp_pkt.mac) << color::RESET << "\n";
                            }
                            else if (dhcp_pkt.msg_type == DHCP_REQUEST) {
                                uint32_t chosen_ip = dhcp_pkt.requested_ip;
                                if (chosen_ip == 0) chosen_ip = dhcp_pkt.ciaddr;
                                lease_pool.commit_lease(dhcp_pkt.mac, chosen_ip, dhcp_pkt.host_name);

                                telnet_ctx.dhcp_ip = ip_to_string(chosen_ip);

                                DhcpPacket ack_pkt = DhcpPacket::create_ack(
                                    dhcp_pkt, chosen_ip, srv_ip, mask, def_gw, dns, 7200
                                );

                                std::vector<uint8_t> ack_payload = ack_pkt.serialize();
                                std::vector<uint8_t> send_buf(sizeof(FrameHeader) + ack_payload.size());
                                FrameHeader* out_hdr = reinterpret_cast<FrameHeader*>(send_buf.data());
                                out_hdr->magic = BDTP_MAGIC;
                                out_hdr->protocol_id = PROTO_DHCP;
                                out_hdr->seq_num = hdr->seq_num;
                                out_hdr->ack_num = hdr->seq_num;
                                out_hdr->detection_scheme = scheme_code;
                                out_hdr->payload_len = static_cast<uint16_t>(ack_payload.size());
                                out_hdr->fcs = compute_fcs(ack_payload.data(), ack_payload.size(), scheme_code);

                                std::memcpy(send_buf.data() + sizeof(FrameHeader), ack_payload.data(), ack_payload.size());

                                sendto(dhcp_sock, send_buf.data(), send_buf.size(), 0,
                                       (struct sockaddr*)&client_addr, addr_len);

                                stats.record_packet_sent(PROTO_DHCP, send_buf.size(), ack_payload.size());

                                std::cout << color::BOLD_GREEN << "  [DHCP Phase 4] Dispatched DHCPACK! Committed lease for "
                                          << ip_to_string(chosen_ip) << color::RESET << "\n";
                                std::cout << lease_pool.print_active_leases();
                            }
                            else if (dhcp_pkt.msg_type == DHCP_RELEASE) {
                                lease_pool.release_lease(dhcp_pkt.mac, dhcp_pkt.ciaddr);
                                std::cout << color::YELLOW << "  [DHCP] Released IP address for "
                                          << mac_to_string(dhcp_pkt.mac) << color::RESET << "\n";
                            }
                        }
                    }
                }
            }
        }

        // --------------------------------------------------------------------
        // Handle TELNET Incoming Connection (TCP)
        // --------------------------------------------------------------------
        if (FD_ISSET(telnet_listen_sock, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_sock = accept(telnet_listen_sock, (struct sockaddr*)&client_addr, &addr_len);
            if (client_sock >= 0) {
                char cli_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, cli_ip, sizeof(cli_ip));
                int cli_port = ntohs(client_addr.sin_port);

                std::cout << "\n" << color::BOLD_YELLOW << "[" << current_timestamp() << "]"
                          << " [TELNET] Incoming TCP connection accepted from "
                          << cli_ip << ":" << cli_port << color::RESET << "\n";

                // Execute RFC 854 Option Negotiation
                std::vector<uint8_t> neg_buf;
                auto append_neg = [&neg_buf](uint8_t verb, uint8_t opt) {
                    auto b = build_telnet_negotiation(verb, opt);
                    neg_buf.insert(neg_buf.end(), b.begin(), b.end());
                };
                append_neg(TELNET_DO, TELOPT_TTYPE);
                append_neg(TELNET_WILL, TELOPT_ECHO);
                append_neg(TELNET_WILL, TELOPT_SGA);
                
                send(client_sock, neg_buf.data(), neg_buf.size(), 0);

                // Send Welcome Banner
                std::string banner = 
                    "\r\n" + color::BOLD_CYAN +
                    "========================================================================\r\n"
                    "  WELCOME TO THE MINI-PROJECT TELNET APPLICATION SERVER (RFC 854)       \r\n"
                    "  Host Configured: BOOTP & DHCP Network Pipeline Validated               \r\n"
                    "========================================================================\r\n" +
                    color::RESET +
                    "Type 'help' to inspect commands, or 'network_info' to verify leases.\r\n\r\n";
                
                send(client_sock, banner.data(), banner.size(), 0);

                // Service client requests until exit
                TelnetParser parser;
                bool session_active = true;
                std::ofstream uploaded_file;

                while (session_active && server_running) {
                    uint8_t recv_buf[2048];
                    ssize_t n = recv(client_sock, recv_buf, sizeof(recv_buf), 0);
                    if (n <= 0) {
                        break; // Connection closed
                    }

                    // Check if message is encapsulated frame (with error checking FCS)
                    if (static_cast<size_t>(n) >= sizeof(FrameHeader)) {
                        const FrameHeader* hdr = reinterpret_cast<const FrameHeader*>(recv_buf);
                        stats.record_packet_received(hdr->protocol_id, n, hdr->payload_len);
                        if (hdr->magic == BDTP_MAGIC) {
                            const uint8_t* payload = recv_buf + sizeof(FrameHeader);
                            size_t payload_len = hdr->payload_len;

                            if (verbose_hex) {
                                std::cout << color::CYAN << "  [Inbound TELNET Frame Hex Dump]:\n" << color::RESET
                                          << hex_dump(recv_buf, std::min((size_t)n, (size_t)64), "    ");
                            }

                            bool fcs_valid = verify_fcs(payload, payload_len, hdr->fcs, hdr->detection_scheme);
                            if (!fcs_valid) {
                                stats.packets_corrupted++;
                                stats.errors_detected++;
                                std::cout << color::BOLD_RED << "  [TELNET FCS ERROR] Data frame #"
                                          << hdr->seq_num << " failed " << get_scheme_name(hdr->detection_scheme)
                                          << " check! Discarding.\n" << color::RESET;

                                // Send NACK frame back
                                FrameHeader nack_hdr;
                                nack_hdr.magic = BDTP_MAGIC;
                                nack_hdr.protocol_id = PROTO_NACK;
                                nack_hdr.seq_num = hdr->seq_num;
                                nack_hdr.ack_num = hdr->seq_num;
                                nack_hdr.detection_scheme = hdr->detection_scheme;
                                nack_hdr.payload_len = 0;
                                nack_hdr.fcs = 0;
                                send(client_sock, &nack_hdr, sizeof(nack_hdr), 0);
                                stats.record_packet_sent(PROTO_NACK, sizeof(nack_hdr), 0);
                                continue;
                            }

                            if (hdr->protocol_id == PROTO_TELNET_DATA) {
                                // Data packet from input file
                                std::string chunk(reinterpret_cast<const char*>(payload), payload_len);
                                std::cout << color::CYAN << "  [TELNET DATA PKT #" << hdr->seq_num
                                          << "] (" << payload_len << " bytes, FCS Validated)\n" << color::RESET;

                                if (!uploaded_file.is_open()) {
                                    uploaded_file.open("server_received_payload.txt", std::ios::binary);
                                }
                                if (uploaded_file.is_open()) {
                                    uploaded_file.write(chunk.data(), chunk.size());
                                    uploaded_file.flush();
                                }

                                // Send ACK frame
                                FrameHeader ack_hdr;
                                ack_hdr.magic = BDTP_MAGIC;
                                ack_hdr.protocol_id = PROTO_ACK;
                                ack_hdr.seq_num = hdr->seq_num;
                                ack_hdr.ack_num = hdr->seq_num;
                                ack_hdr.detection_scheme = scheme_code;
                                ack_hdr.payload_len = 0;
                                ack_hdr.fcs = 0;
                                send(client_sock, &ack_hdr, sizeof(ack_hdr), 0);

                                stats.record_packet_sent(PROTO_ACK, sizeof(ack_hdr), 0);
                                continue;
                            }
                            else if (hdr->protocol_id == PROTO_TELNET_CONTROL) {
                                std::string cmd(reinterpret_cast<const char*>(payload), payload_len);
                                std::cout << color::YELLOW << "  [TELNET CMD #" << hdr->seq_num
                                          << "]: " << cmd << color::RESET << "\n";

                                std::string reply = execute_remote_command(cmd, telnet_ctx);

                                // Encapsulate reply
                                std::vector<uint8_t> reply_bytes(reply.begin(), reply.end());
                                std::vector<uint8_t> send_frame(sizeof(FrameHeader) + reply_bytes.size());
                                FrameHeader* out_h = reinterpret_cast<FrameHeader*>(send_frame.data());
                                out_h->magic = BDTP_MAGIC;
                                out_h->protocol_id = PROTO_TELNET_CONTROL;
                                out_h->seq_num = hdr->seq_num;
                                out_h->ack_num = hdr->seq_num;
                                out_h->detection_scheme = scheme_code;
                                out_h->payload_len = static_cast<uint16_t>(reply_bytes.size());
                                out_h->fcs = compute_fcs(reply_bytes.data(), reply_bytes.size(), scheme_code);

                                std::memcpy(send_frame.data() + sizeof(FrameHeader), reply_bytes.data(), reply_bytes.size());
                                send(client_sock, send_frame.data(), send_frame.size(), 0);

                                stats.record_packet_sent(PROTO_TELNET_CONTROL, send_frame.size(), reply_bytes.size());

                                if (cmd == "exit" || cmd == "quit") {
                                    session_active = false;
                                }
                                continue;
                            }
                        }
                    }

                    // Otherwise fallback raw NVT parsing
                    parser.reset();
                    parser.parse(recv_buf, n);

                    if (!parser.clean_data.empty()) {
                        std::cout << color::YELLOW << "  [TELNET RAW]: " << parser.clean_data << color::RESET << "\n";
                        std::string reply = execute_remote_command(parser.clean_data, telnet_ctx);
                        send(client_sock, reply.data(), reply.size(), 0);
                        if (parser.clean_data.find("exit") != std::string::npos) {
                            session_active = false;
                        }
                    }
                }

                if (uploaded_file.is_open()) {
                    uploaded_file.close();
                    std::cout << color::BOLD_GREEN << "  [TELNET] File successfully received and written to server_received_payload.txt\n" << color::RESET;
                }

                close(client_sock);
                std::cout << color::BOLD_YELLOW << "  [TELNET] Connection closed for " << cli_ip << "\n" << color::RESET;
            }
        }
    }

    stats.stop();
    stats.print_summary("SERVER DAEMON SHUTDOWN REPORT");

    close(bootp_sock);
    close(dhcp_sock);
    close(telnet_listen_sock);

    std::cout << color::BOLD_GREEN << "[Server] All sockets cleanly closed. Goodbye!\n" << color::RESET;
    return 0;
}

