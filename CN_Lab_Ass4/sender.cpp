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
        printBanner("CDMA STATION TRANSMITTER / SENDER");
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
        std::cout << "Enter Input Message or Filepath: ";
        std::getline(std::cin, input_source);
    }

    if (station_id < 0 || station_id >= num_stations) {
        std::cerr << Color::RED << "Error: Station ID " << station_id 
                  << " out of bounds for " << num_stations << " stations.\n" << Color::RESET;
        return 1;
    }

    WalshCode walsh(num_stations);
    int m = walsh.getChipLength();
    std::vector<int> code = walsh.getCode(station_id);

    printBanner("CDMA SENDER - STATION " + std::to_string(station_id));
    std::cout << "Walsh Code Dimension M: " << m << "\n";
    std::cout << "Assigned Walsh Code   : ";
    printChips(code);
    std::cout << "\n";

    // Read payload
    std::vector<uint8_t> payload;
    // Check if input_source is a file
    std::ifstream test_file(input_source, std::ios::binary);
    if (test_file.good()) {
        payload = readBinaryFile(input_source);
        std::cout << "Loaded from file: " << input_source << " (" << payload.size() << " bytes)\n";
    } else {
        if (input_source.empty()) {
            input_source = "Station-" + std::to_string(station_id) + " Hello CDMA World!";
        }
        payload.assign(input_source.begin(), input_source.end());
        std::cout << "Message string: \"" << input_source << "\" (" << payload.size() << " bytes)\n";
    }

    // Build structured CDMA frame with CRC-32 (from Ass1/Ass2 design)
    CDMAFrame frame(station_id, 0xFF, CDMAFrameType::DATA, 1, payload);
    frame.calculate_fcs(2); // CRC-32

    std::vector<uint8_t> frame_bytes = frame.serialize();
    std::cout << "Serialized CDMA Frame: " << frame_bytes.size() << " bytes (CRC32: 0x" 
              << std::hex << frame.fcs << std::dec << ")\n";

    // Encode frame bytes into CDMA chip stream
    std::vector<std::vector<int>> chip_stream = walsh.encodeBytes(frame_bytes, station_id);
    std::cout << "Modulated into " << chip_stream.size() << " bit slots (" 
              << (chip_stream.size() * m) << " total chips)\n";

    // Socket setup
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        std::cerr << Color::RED << "Failed to create socket.\n" << Color::RESET;
        return 1;
    }

    // Set receive timeout for ACK
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in channel_addr;
    std::memset(&channel_addr, 0, sizeof(channel_addr));
    channel_addr.sin_family = AF_INET;
    channel_addr.sin_port = htons(port);
    inet_pton(AF_INET, channel_ip.c_str(), &channel_addr.sin_addr);

    // Prepare packet
    NetPacketHeader hdr;
    hdr.magic = CDMA_NET_MAGIC;
    hdr.msg_type = static_cast<uint8_t>(NetMsgType::CHIP_DATA);
    hdr.station_id = static_cast<uint8_t>(station_id);
    hdr.num_slots = static_cast<uint32_t>(chip_stream.size());
    hdr.chip_len = static_cast<uint16_t>(m);
    hdr.total_chips = hdr.num_slots * hdr.chip_len;

    std::vector<uint8_t> packet = serializeChips(hdr, chip_stream);

    printSubHeader("TRANSMITTING TO CHANNEL");
    std::cout << "Sending " << packet.size() << " bytes (" << hdr.total_chips << " chips) to "
              << channel_ip << ":" << port << "...\n";

    ssize_t sent = sendto(sock_fd, packet.data(), packet.size(), 0, 
                          (struct sockaddr*)&channel_addr, sizeof(channel_addr));
    if (sent < 0) {
        std::cerr << Color::RED << "sendto failed.\n" << Color::RESET;
        close(sock_fd);
        return 1;
    }

    std::cout << Color::GREEN << "[Sender " << station_id << "] Transmission sent successfully!\n" << Color::RESET;

    // Wait for ACK
    uint8_t ack_buf[1024];
    socklen_t addr_len = sizeof(channel_addr);
    int n = recvfrom(sock_fd, ack_buf, sizeof(ack_buf), 0, (struct sockaddr*)&channel_addr, &addr_len);
    if (n >= (int)sizeof(NetPacketHeader)) {
        NetPacketHeader* ack_hdr = reinterpret_cast<NetPacketHeader*>(ack_buf);
        if (ack_hdr->magic == CDMA_NET_MAGIC && ack_hdr->msg_type == static_cast<uint8_t>(NetMsgType::ACK_OK)) {
            std::cout << Color::GREEN << "[Sender " << station_id << "] ACK received from Channel Coordinator.\n" << Color::RESET;
        }
    } else {
        std::cout << Color::YELLOW << "[Sender " << station_id << "] Warning: ACK wait timed out.\n" << Color::RESET;
    }

    close(sock_fd);
    return 0;
}
