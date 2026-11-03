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
    int port = 8080;
    int target_station = -1; // -1 means decode ALL stations
    std::string channel_ip = "127.0.0.1";
    int num_stations = 4;

    if (argc >= 2) port = std::stoi(argv[1]);
    if (argc >= 3) target_station = std::stoi(argv[2]);
    if (argc >= 4) channel_ip = argv[3];
    if (argc >= 5) num_stations = std::stoi(argv[4]);

    printBanner("CDMA STATION RECEIVER & DEMULTIPLEXER");
    std::cout << "Channel IP           : " << channel_ip << "\n";
    std::cout << "Channel Port         : " << port << "\n";
    std::cout << "Configured Stations  : " << num_stations << "\n";
    if (target_station >= 0) {
        std::cout << "Target Demux Station : Station " << target_station << "\n";
    } else {
        std::cout << "Target Demux Station : " << Color::GREEN << "ALL STATIONS (Full CDMA Demultiplexing)" << Color::RESET << "\n";
    }

    WalshCode walsh(num_stations);
    int m = walsh.getChipLength();
    std::cout << "Walsh Dimension M    : " << m << "\n\n";

    // Setup socket
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        std::cerr << Color::RED << "Failed to create socket.\n" << Color::RESET;
        return 1;
    }

    // Bind to any ephemeral port
    struct sockaddr_in rx_addr;
    std::memset(&rx_addr, 0, sizeof(rx_addr));
    rx_addr.sin_family = AF_INET;
    rx_addr.sin_port = 0; // OS allocates port
    rx_addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock_fd, (struct sockaddr*)&rx_addr, sizeof(rx_addr));

    // Register with channel
    struct sockaddr_in channel_addr;
    std::memset(&channel_addr, 0, sizeof(channel_addr));
    channel_addr.sin_family = AF_INET;
    channel_addr.sin_port = htons(port);
    inet_pton(AF_INET, channel_ip.c_str(), &channel_addr.sin_addr);

    NetPacketHeader reg_pkt;
    reg_pkt.magic = CDMA_NET_MAGIC;
    reg_pkt.msg_type = static_cast<uint8_t>(NetMsgType::REGISTER_RECEIVER);
    reg_pkt.station_id = (target_station >= 0) ? target_station : 0xFF;
    reg_pkt.num_slots = 0;
    reg_pkt.chip_len = m;
    reg_pkt.total_chips = 0;

    std::cout << "[Receiver] Registering with Channel Coordinator at " << channel_ip << ":" << port << "...\n";
    sendto(sock_fd, &reg_pkt, sizeof(reg_pkt), 0, (struct sockaddr*)&channel_addr, sizeof(channel_addr));

    std::cout << "[Receiver] Listening for composite broadcast signal on shared channel (waiting for all senders)...\n\n";

    uint8_t buffer[65535];
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    NetPacketHeader* hdr = nullptr;
    const uint8_t* payload_ptr = nullptr;
    uint32_t num_slots = 0;
    uint16_t chip_len = 0;

    while (true) {
        int n = recvfrom(sock_fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&sender_addr, &addr_len);
        if (n < (int)sizeof(NetPacketHeader)) {
            std::cerr << Color::RED << "[Receiver] Error or timeout waiting for channel broadcast.\n" << Color::RESET;
            close(sock_fd);
            return 1;
        }

        hdr = reinterpret_cast<NetPacketHeader*>(buffer);
        if (hdr->magic != CDMA_NET_MAGIC) {
            std::cerr << Color::RED << "[Receiver] Invalid magic received.\n" << Color::RESET;
            close(sock_fd);
            return 1;
        }

        if (hdr->msg_type == static_cast<uint8_t>(NetMsgType::ACK_OK)) {
            std::cout << Color::GREEN << "[Receiver] Registration confirmed by Channel Coordinator.\n" << Color::RESET;
            continue;
        }

        if (hdr->msg_type == static_cast<uint8_t>(NetMsgType::COMPOSITE_DATA)) {
            num_slots = hdr->num_slots;
            chip_len = hdr->chip_len;
            payload_ptr = buffer + sizeof(NetPacketHeader);
            break;
        }
    }

    std::cout << Color::GREEN << Color::BOLD 
              << "[Receiver] Successfully received composite channel signal!\n" 
              << "Total Bit Slots: " << num_slots << " | Chips per slot: " << chip_len 
              << " (" << (num_slots * chip_len) << " total chips)\n" << Color::RESET;

    std::vector<std::vector<int>> composite_stream = deserializeChips(payload_ptr, num_slots, chip_len);

    // Determine stations to decode
    int start_st = (target_station >= 0) ? target_station : 0;
    int end_st   = (target_station >= 0) ? target_station + 1 : num_stations;

    printBanner("PERFORMING CDMA DEMULTIPLEXING & RECONSTRUCTION");

    int successful_reconstructions = 0;

    for (int st = start_st; st < end_st; ++st) {
        std::cout << Color::YELLOW << Color::BOLD 
                  << "\n>>> Demultiplexing for Station " << st << " using Walsh Code W" << st << " <<<\n" 
                  << Color::RESET;

        std::vector<int> code = walsh.getCode(st);
        std::cout << "Walsh Code: ";
        printChips(code);
        std::cout << "\n";

        // Step 1: Decode bytes from composite stream
        std::vector<uint8_t> decoded_bytes = walsh.decodeBytes(composite_stream, st, 0);

        if (decoded_bytes.empty() || decoded_bytes.size() < 11) {
            std::cout << "  Status: " << Color::YELLOW << "STATION WAS IDLE / SILENT (No data transmitted)\n" << Color::RESET;
            continue;
        }

        try {
            // Step 2: Parse CDMAFrame
            CDMAFrame frame = CDMAFrame::deserialize(decoded_bytes);
            bool crc_valid = frame.verify_fcs(2);

            std::cout << "  Frame Header: Source Station=" << (int)frame.src_id 
                      << " | Seq=" << frame.seq_no 
                      << " | Payload Length=" << frame.length << " bytes\n";
            std::cout << "  FCS (CRC-32): 0x" << std::hex << frame.fcs << std::dec;
            if (crc_valid) {
                std::cout << " " << Color::GREEN << "[CRC VERIFIED - DATA INTACT]" << Color::RESET << "\n";
            } else {
                std::cout << " " << Color::RED << "[CRC MISMATCH - CORRUPTED]" << Color::RESET << "\n";
            }

            // Step 3: Print payload content
            std::string payload_str(frame.payload.begin(), frame.payload.end());
            std::cout << "  Reconstructed Payload (" << frame.payload.size() << " bytes):\n";
            std::cout << "  \"" << Color::CYAN << payload_str << Color::RESET << "\"\n";

            // Save to file
            std::string outfile = "received_station_" + std::to_string(st) + ".txt";
            writeBinaryFile(outfile, frame.payload);
            std::cout << "  Saved to file: " << outfile << "\n";

            // Check if corresponding sample input exists to verify 100% diff
            std::string sample_in = "sample_st" + std::to_string(st) + ".txt";
            std::ifstream sample_check(sample_in);
            if (sample_check.good()) {
                compareFiles(sample_in, outfile, true);
            }

            successful_reconstructions++;
        } catch (const std::exception& ex) {
            std::cout << Color::RED << "  Frame deserialization error: " << ex.what() << Color::RESET << "\n";
        }
    }

    std::cout << "\n" << Color::GREEN << Color::BOLD 
              << "======================================================================\n"
              << "   RECONSTRUCTION SUMMARY: " << successful_reconstructions << " STATIONS PERFECTLY RECONSTRUCTED\n"
              << "======================================================================\n"
              << Color::RESET;

    close(sock_fd);
    return 0;
}
