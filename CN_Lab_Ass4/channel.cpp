#include "channel.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <chrono>
#include <iomanip>
#include <poll.h>

CDMAChannel::CDMAChannel(int p, int num_stations, double noise)
    : port(p), expected_stations(num_stations), noise_prob(noise), walsh(num_stations), sock_fd(-1) {}

CDMAChannel::~CDMAChannel() {
    if (sock_fd >= 0) {
        close(sock_fd);
    }
}

bool CDMAChannel::init() {
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        std::cerr << Color::RED << "[Channel] Failed to create UDP socket.\n" << Color::RESET;
        return false;
    }

    int opt = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << Color::RED << "[Channel] Bind failed on port " << port << ".\n" << Color::RESET;
        close(sock_fd);
        sock_fd = -1;
        return false;
    }

    printBanner("CDMA SHARED MEDIUM CHANNEL COORDINATOR");
    std::cout << Color::GREEN << "Listening on UDP Port: " << port << Color::RESET << "\n";
    std::cout << "Configured Stations  : " << expected_stations << "\n";
    std::cout << "Walsh Code Dimension : " << walsh.getChipLength() << " (M = " << walsh.getChipLength() << " chips/bit)\n";
    if (noise_prob > 0.0) {
        std::cout << Color::YELLOW << "Channel Noise Prob   : " << (noise_prob * 100.0) << "%\n" << Color::RESET;
    } else {
        std::cout << "Channel Noise Prob   : 0.0% (Ideal Channel)\n";
    }

    walsh.printCodeTable();
    walsh.verifyOrthogonality(true);

    return true;
}

void CDMAChannel::processSuperposition() {
    if (station_buffers.empty()) return;

    printSubHeader("COMPUTING LINEAR SUPERPOSITION OVER COMMON CHANNEL");
    std::cout << "[Channel] Active Stations in this round: " << station_buffers.size() << " / " << expected_stations << "\n";

    // Prepare streams for all stations (pad silent stations with 0 chips)
    std::vector<std::vector<std::vector<int>>> all_streams;
    size_t max_slots = 0;

    for (const auto& [st_id, stream] : station_buffers) {
        max_slots = std::max(max_slots, stream.size());
    }

    int m = walsh.getChipLength();
    for (int st = 0; st < expected_stations; ++st) {
        if (station_buffers.count(st)) {
            all_streams.push_back(station_buffers[st]);
            std::cout << "  - Station " << st << ": Transmitting " 
                      << station_buffers[st].size() << " bit slots ("
                      << (station_buffers[st].size() / 8) << " bytes)\n";
        } else {
            // Station is silent -> 0 chips
            std::vector<std::vector<int>> silent_stream(max_slots, std::vector<int>(m, 0));
            all_streams.push_back(silent_stream);
            std::cout << "  - Station " << st << ": " << Color::YELLOW << "SILENT / IDLE (0 chips transmitted)" << Color::RESET << "\n";
        }
    }

    // Superimpose
    std::vector<std::vector<int>> composite_stream = WalshCode::superimposeStreams(all_streams);

    // Optional noise injection
    if (noise_prob > 0.0) {
        for (auto& slot : composite_stream) {
            slot = WalshCode::addNoise(slot, noise_prob);
        }
        std::cout << Color::YELLOW << "[Channel] Injected channel noise into composite signal.\n" << Color::RESET;
    }

    // Visual breakdown of initial slots (e.g. first 3 slots)
    std::cout << "\n" << Color::CYAN << "--- Real-Time Chip Superposition (Initial Slots) ---" << Color::RESET << "\n";
    size_t preview_slots = std::min((size_t)4, composite_stream.size());
    for (size_t s = 0; s < preview_slots; ++s) {
        std::cout << Color::BOLD << "Slot " << s << ":" << Color::RESET << "\n";
        for (int st = 0; st < expected_stations; ++st) {
            std::vector<int> chips(m, 0);
            if (station_buffers.count(st) && s < station_buffers[st].size()) {
                chips = station_buffers[st][s];
                int bit = walsh.decodeBit(chips, st);
                std::cout << "  Station " << st << " (bit " << (bit == 1 ? "1" : "0") << "): ";
            } else {
                std::cout << "  Station " << st << " (idle ): ";
            }
            printChips(chips);
            std::cout << "\n";
        }
        std::cout << Color::GREEN << "  => Superimposed C[" << s << "] = ";
        printChips(composite_stream[s]);
        std::cout << Color::RESET << "\n\n";
    }

    // Broadcast composite stream to all registered receivers and transmitters
    printSubHeader("BROADCASTING COMPOSITE SIGNAL TO RECEIVERS");
    std::cout << "[Channel] Broadcasting to " << receivers.size() << " registered receivers...\n";

    NetPacketHeader hdr;
    hdr.magic = CDMA_NET_MAGIC;
    hdr.msg_type = static_cast<uint8_t>(NetMsgType::COMPOSITE_DATA);
    hdr.station_id = 0xFF; // broadcast
    hdr.num_slots = static_cast<uint32_t>(composite_stream.size());
    hdr.chip_len = static_cast<uint16_t>(m);
    hdr.total_chips = hdr.num_slots * hdr.chip_len;

    std::vector<uint8_t> packet = serializeChips(hdr, composite_stream);

    for (const auto& rx_addr : receivers) {
        ssize_t sent = sendto(sock_fd, packet.data(), packet.size(), 0, (struct sockaddr*)&rx_addr, sizeof(rx_addr));
        char ip_buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &rx_addr.sin_addr, ip_buf, INET_ADDRSTRLEN);
        std::cout << "[Channel] Broadcast to " << ip_buf << ":" << ntohs(rx_addr.sin_port)
                  << " sent " << sent << " bytes.\n";
    }

    std::cout << Color::GREEN << Color::BOLD 
              << "[Channel] Broadcast successful! Sent " << composite_stream.size() 
              << " slots (" << (composite_stream.size() * m) << " total chips) to all receivers.\n" 
              << Color::RESET << std::flush;

    // Reset station buffers for subsequent transmission rounds
    station_buffers.clear();
}

void CDMAChannel::run() {
    uint8_t recv_buf[65535];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    std::cout << "\n[Channel] Ready to receive station transmissions.\n"
              << "          - Waiting for all " << expected_stations << " stations to transmit.\n"
              << "          - Or press [ENTER] at any time to trigger superposition with current stations.\n"
              << "          - Press Ctrl+C to terminate.\n\n" << std::flush;

    struct pollfd fds[2];
    fds[0].fd = sock_fd;
    fds[0].events = POLLIN;
    fds[1].fd = isatty(STDIN_FILENO) ? STDIN_FILENO : -1;
    fds[1].events = POLLIN;

    while (true) {
        int ret = poll(fds, 2, -1); // Block indefinitely without timeout!
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }

        // Handle user keyboard input (Enter key to force trigger)
        if (fds[1].fd >= 0 && (fds[1].revents & (POLLIN | POLLHUP | POLLERR))) {
            std::string line;
            if (std::getline(std::cin, line)) {
                if (!station_buffers.empty()) {
                    std::cout << Color::YELLOW << "[Channel] Manual trigger requested via Enter key. Processing currently received stations...\n" 
                              << Color::RESET << std::flush;
                    processSuperposition();
                } else {
                    std::cout << Color::YELLOW << "[Channel] No station transmissions received yet in this round.\n" 
                              << Color::RESET << std::flush;
                }
            } else {
                // Stdin closed or at EOF (e.g. background process), ignore stdin from now on
                fds[1].fd = -1;
            }
        }

        // Handle incoming UDP datagrams
        if (fds[0].revents & POLLIN) {
            client_len = sizeof(client_addr);
            int n = recvfrom(sock_fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&client_addr, &client_len);
            if (n >= (int)sizeof(NetPacketHeader)) {
                NetPacketHeader* hdr = reinterpret_cast<NetPacketHeader*>(recv_buf);
                if (hdr->magic == CDMA_NET_MAGIC) {
                    if (hdr->msg_type == static_cast<uint8_t>(NetMsgType::REGISTER_RECEIVER)) {
                        receivers.insert(client_addr);
                        char ip_str[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
                        std::cout << Color::BLUE << "[Channel] Registered new receiver from " 
                                  << ip_str << ":" << ntohs(client_addr.sin_port) 
                                  << " (Total receivers: " << receivers.size() << ")" << Color::RESET << "\n" << std::flush;

                        // Send ACK
                        NetPacketHeader ack;
                        ack.magic = CDMA_NET_MAGIC;
                        ack.msg_type = static_cast<uint8_t>(NetMsgType::ACK_OK);
                        ack.station_id = hdr->station_id;
                        ack.num_slots = 0;
                        ack.chip_len = walsh.getChipLength();
                        ack.total_chips = 0;
                        sendto(sock_fd, &ack, sizeof(ack), 0, (struct sockaddr*)&client_addr, client_len);
                    }
                    else if (hdr->msg_type == static_cast<uint8_t>(NetMsgType::CHIP_DATA)) {
                        int st_id = hdr->station_id;
                        uint32_t num_slots = hdr->num_slots;
                        uint16_t chip_len = hdr->chip_len;

                        const uint8_t* payload = recv_buf + sizeof(NetPacketHeader);
                        std::vector<std::vector<int>> stream = deserializeChips(payload, num_slots, chip_len);

                        station_buffers[st_id] = stream;
                        receivers.insert(client_addr); // Senders also receive the composite broadcast

                        std::cout << Color::GREEN << "[Channel] Received " << num_slots << " bit slots ("
                                  << (num_slots * chip_len) << " chips) from Station " << st_id 
                                  << " [Current active: " << station_buffers.size() << "/" << expected_stations << "]"
                                  << Color::RESET << "\n" << std::flush;

                        // Send ACK to sender
                        NetPacketHeader ack;
                        ack.magic = CDMA_NET_MAGIC;
                        ack.msg_type = static_cast<uint8_t>(NetMsgType::ACK_OK);
                        ack.station_id = st_id;
                        ack.num_slots = num_slots;
                        ack.chip_len = chip_len;
                        ack.total_chips = num_slots * chip_len;
                        sendto(sock_fd, &ack, sizeof(ack), 0, (struct sockaddr*)&client_addr, client_len);

                        // If all expected stations have transmitted, superimpose immediately!
                        if ((int)station_buffers.size() >= expected_stations) {
                            processSuperposition();
                        }
                    }
                    else if (hdr->msg_type == static_cast<uint8_t>(NetMsgType::RESET)) {
                        station_buffers.clear();
                        std::cout << "[Channel] Buffers cleared.\n" << std::flush;
                    }
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    int port = 8080;
    int stations = 4;
    double noise = 0.0;

    if (argc >= 2) port = std::stoi(argv[1]);
    if (argc >= 3) stations = std::stoi(argv[2]);
    if (argc >= 4) noise = std::stod(argv[3]);

    CDMAChannel channel(port, stations, noise);
    if (!channel.init()) {
        return 1;
    }

    channel.run();
    return 0;
}
