#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "walsh.hpp"
#include "protocol.hpp"
#include "utils.hpp"
#include "frame.hpp"

int main(int argc, char* argv[]) {
    std::string channel_ip = "127.0.0.1";
    int port = 8080;
    int station_id = 0;
    std::string input_source = "";
    int num_stations = 4;

    if (argc >= 4) {
        channel_ip = argv[1];
        port = std::stoi(argv[2]);
        station_id = std::stoi(argv[3]);
        if (argc >= 5) input_source = argv[4];
        if (argc >= 6) num_stations = std::stoi(argv[5]);
    } else {
        printBanner("CDMA DISTRIBUTED STATION NODE");
        std::cout << "Enter Channel IP [default 127.0.0.1]: ";
        std::string ip_input;
        std::getline(std::cin, ip_input);
        if (!ip_input.empty()) channel_ip = ip_input;

        std::cout << "Enter Channel Port [default 8080]: ";
        std::string port_str;
        std::getline(std::cin, port_str);
        if (!port_str.empty()) port = std::stoi(port_str);

        std::cout << "Enter Station ID (0 to " << (num_stations - 1) << "): ";
        std::cin >> station_id;

        std::cout << "Enter Total Stations in System [default 4]: ";
        std::cin >> num_stations;

        std::cin.ignore();
        std::cout << "Enter Input Message or Filepath (leave empty to be silent receiver): ";
        std::getline(std::cin, input_source);
    }

    WalshCode walsh(num_stations);
    int m = walsh.getChipLength();
    std::vector<int> my_code = walsh.getCode(station_id);

    printBanner("STATION " + std::to_string(station_id) + " INITIALIZATION");
    std::cout << "Walsh Dimension M : " << m << "\n";
    std::cout << "Assigned Code W" << station_id << " : ";
    printChips(my_code);
    std::cout << "\n";

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        std::cerr << Color::RED << "Socket creation failed.\n" << Color::RESET;
        return 1;
    }

    struct sockaddr_in channel_addr;
    std::memset(&channel_addr, 0, sizeof(channel_addr));
    channel_addr.sin_family = AF_INET;
    channel_addr.sin_port = htons(port);
    inet_pton(AF_INET, channel_ip.c_str(), &channel_addr.sin_addr);

    // If input data is provided, transmit
    if (!input_source.empty()) {
        std::vector<uint8_t> payload;
        std::ifstream test_file(input_source, std::ios::binary);
        if (test_file.good()) {
            payload = readBinaryFile(input_source);
            std::cout << "Read " << payload.size() << " bytes from file: " << input_source << "\n";
        } else {
            payload.assign(input_source.begin(), input_source.end());
            std::cout << "Input string: \"" << input_source << "\" (" << payload.size() << " bytes)\n";
        }

        CDMAFrame tx_frame(station_id, 0xFF, CDMAFrameType::DATA, 1, payload);
        tx_frame.calculate_fcs(2); // CRC-32

        std::vector<uint8_t> frame_bytes = tx_frame.serialize();
        std::vector<std::vector<int>> chip_stream = walsh.encodeBytes(frame_bytes, station_id);

        NetPacketHeader hdr;
        hdr.magic = CDMA_NET_MAGIC;
        hdr.msg_type = static_cast<uint8_t>(NetMsgType::CHIP_DATA);
        hdr.station_id = static_cast<uint8_t>(station_id);
        hdr.num_slots = static_cast<uint32_t>(chip_stream.size());
        hdr.chip_len = static_cast<uint16_t>(m);
        hdr.total_chips = hdr.num_slots * hdr.chip_len;

        std::vector<uint8_t> packet = serializeChips(hdr, chip_stream);

        std::cout << "[Station " << station_id << "] Transmitting " << hdr.total_chips 
                  << " chips to Channel...\n";
        sendto(sock_fd, packet.data(), packet.size(), 0, (struct sockaddr*)&channel_addr, sizeof(channel_addr));
    } else {
        // Register as passive receiver
        NetPacketHeader reg_pkt;
        reg_pkt.magic = CDMA_NET_MAGIC;
        reg_pkt.msg_type = static_cast<uint8_t>(NetMsgType::REGISTER_RECEIVER);
        reg_pkt.station_id = static_cast<uint8_t>(station_id);
        reg_pkt.num_slots = 0;
        reg_pkt.chip_len = m;
        reg_pkt.total_chips = 0;
        sendto(sock_fd, &reg_pkt, sizeof(reg_pkt), 0, (struct sockaddr*)&channel_addr, sizeof(channel_addr));
        std::cout << "[Station " << station_id << "] Registered as silent listener.\n";
    }

    // Now listen for channel composite broadcast
    std::cout << "[Station " << station_id << "] Waiting for composite broadcast from channel...\n";

    uint8_t buffer[65535];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    while (true) {
        int n = recvfrom(sock_fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&from_addr, &from_len);
        if (n < (int)sizeof(NetPacketHeader)) {
            std::cout << Color::YELLOW << "[Station " << station_id << "] Timed out waiting for composite signal.\n" << Color::RESET;
            break;
        }

        NetPacketHeader* hdr = reinterpret_cast<NetPacketHeader*>(buffer);
        if (hdr->magic == CDMA_NET_MAGIC && hdr->msg_type == static_cast<uint8_t>(NetMsgType::COMPOSITE_DATA)) {
            std::vector<std::vector<int>> composite = deserializeChips(buffer + sizeof(NetPacketHeader), 
                                                                       hdr->num_slots, hdr->chip_len);
            std::cout << Color::GREEN << "[Station " << station_id << "] Received composite channel signal with " 
                      << hdr->num_slots << " slots!\n" << Color::RESET;

            // Reconstruct own message
            std::vector<uint8_t> my_decoded = walsh.decodeBytes(composite, station_id, 0);
            if (!my_decoded.empty() && my_decoded.size() >= 11) {
                try {
                    CDMAFrame frame = CDMAFrame::deserialize(my_decoded);
                    std::string payload_str(frame.payload.begin(), frame.payload.end());
                    std::cout << Color::GREEN << Color::BOLD 
                              << "[Station " << station_id << "] Successfully Reconstructed OWN Transmission:\n"
                              << "  \"" << payload_str << "\"\n"
                              << "  CRC32 Status: " << (frame.verify_fcs(2) ? "VALID (Verified)" : "CORRUPT")
                              << Color::RESET << "\n";
                } catch (...) {}
            }

            // Also check all other stations
            std::cout << "\n[Station " << station_id << "] Scanning composite medium for other stations:\n";
            for (int other = 0; other < num_stations; ++other) {
                if (other == station_id) continue;
                std::vector<uint8_t> other_bytes = walsh.decodeBytes(composite, other, 0);
                if (!other_bytes.empty() && other_bytes.size() >= 11) {
                    try {
                        CDMAFrame other_f = CDMAFrame::deserialize(other_bytes);
                        std::string msg(other_f.payload.begin(), other_f.payload.end());
                        std::cout << "  * Station " << other << " heard: \"" << Color::CYAN << msg << Color::RESET << "\"\n";
                    } catch (...) {}
                }
            }
            break;
        }
    }

    close(sock_fd);
    return 0;
}
