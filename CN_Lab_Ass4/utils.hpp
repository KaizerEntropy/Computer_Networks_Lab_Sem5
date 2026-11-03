#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <iostream>
#include <netinet/in.h>

namespace Color {
    const std::string RESET   = "\033[0m";
    const std::string BOLD    = "\033[1m";
    const std::string RED     = "\033[31m";
    const std::string GREEN   = "\033[32m";
    const std::string YELLOW  = "\033[33m";
    const std::string BLUE    = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN    = "\033[36m";
    const std::string WHITE   = "\033[37m";
}

// Banner & section headers
void printBanner(const std::string& title);
void printSubHeader(const std::string& title);

// Visualizers
void printChips(const std::vector<int>& chips, std::ostream& os = std::cout);
void printBits(const std::vector<int>& bits, std::ostream& os = std::cout);
std::string chipsToString(const std::vector<int>& chips);

// File I/O
std::vector<uint8_t> readBinaryFile(const std::string& filepath);
bool writeBinaryFile(const std::string& filepath, const std::vector<uint8_t>& data);
bool compareFiles(const std::string& path1, const std::string& path2, bool print_report = true);

// Checksum and CRC helpers (continuity with Ass1 & Ass2)
uint16_t computeChecksum16(const uint8_t* data, size_t len);
uint32_t computeCRC32(const uint8_t* data, size_t len);

// Socket address comparator for UDP maps/sets
struct AddrComparator {
    bool operator()(const struct sockaddr_in& a, const struct sockaddr_in& b) const {
        if (a.sin_addr.s_addr != b.sin_addr.s_addr) return a.sin_addr.s_addr < b.sin_addr.s_addr;
        return a.sin_port < b.sin_port;
    }
};
