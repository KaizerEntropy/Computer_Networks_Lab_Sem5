#include "utils.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

void printBanner(const std::string& title) {
    std::cout << "\n" << Color::CYAN << Color::BOLD;
    std::cout << "======================================================================\n";
    std::cout << "   " << title << "\n";
    std::cout << "======================================================================\n";
    std::cout << Color::RESET;
}

void printSubHeader(const std::string& title) {
    std::cout << "\n" << Color::YELLOW << Color::BOLD;
    std::cout << "--- " << title << " ---\n";
    std::cout << Color::RESET;
}

void printChips(const std::vector<int>& chips, std::ostream& os) {
    os << "[";
    for (size_t i = 0; i < chips.size(); ++i) {
        if (chips[i] > 0) os << "+" << chips[i];
        else if (chips[i] < 0) os << chips[i];
        else os << " 0";
        if (i + 1 < chips.size()) os << ", ";
    }
    os << "]";
}

std::string chipsToString(const std::vector<int>& chips) {
    std::ostringstream oss;
    printChips(chips, oss);
    return oss.str();
}

void printBits(const std::vector<int>& bits, std::ostream& os) {
    for (int b : bits) {
        if (b == 1) os << "1";
        else if (b == 0) os << "0";
        else os << "-";
    }
}

std::vector<uint8_t> readBinaryFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << Color::RED << "Error opening file for reading: " << filepath << Color::RESET << "\n";
        return {};
    }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
}

bool writeBinaryFile(const std::string& filepath, const std::vector<uint8_t>& data) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << Color::RED << "Error opening file for writing: " << filepath << Color::RESET << "\n";
        return false;
    }
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

bool compareFiles(const std::string& path1, const std::string& path2, bool print_report) {
    auto data1 = readBinaryFile(path1);
    auto data2 = readBinaryFile(path2);

    if (data1.empty() && data2.empty()) {
        if (print_report) {
            std::cout << Color::YELLOW << "[Compare] Warning: Both files are empty or unreadable.\n" << Color::RESET;
        }
        return true;
    }

    bool match = (data1 == data2);
    if (print_report) {
        std::cout << "\n---------------- File Verification Report ----------------\n";
        std::cout << "Original File : " << path1 << " (" << data1.size() << " bytes)\n";
        std::cout << "Decoded  File : " << path2 << " (" << data2.size() << " bytes)\n";
        if (match) {
            std::cout << Color::GREEN << Color::BOLD 
                      << "[SUCCESS] Files are 100% BIT-FOR-BIT IDENTICAL! Perfect Reconstruction Verified.\n" 
                      << Color::RESET;
        } else {
            std::cout << Color::RED << Color::BOLD 
                      << "[FAILURE] Files differ! Size: " << data1.size() << " vs " << data2.size() << "\n" 
                      << Color::RESET;
        }
        std::cout << "----------------------------------------------------------\n";
    }
    return match;
}

uint16_t computeChecksum16(const uint8_t* data, size_t len) {
    uint32_t sum = 0;
    size_t i = 0;
    while (i + 1 < len) {
        uint16_t word = (uint16_t)((data[i] << 8) | data[i + 1]);
        sum += word;
        if (sum > 0xFFFF) {
            sum = (sum & 0xFFFF) + 1;
        }
        i += 2;
    }
    if (i < len) {
        sum += (uint16_t)(data[i] << 8);
        if (sum > 0xFFFF) {
            sum = (sum & 0xFFFF) + 1;
        }
    }
    return (uint16_t)(~sum & 0xFFFF);
}

uint32_t computeCRC32(const uint8_t* data, size_t len) {
    uint32_t poly = 0x04C11DB7;
    uint32_t mask = 0xFFFFFFFFu;
    uint32_t topbit = 1u << 31;
    uint32_t reg = 0;

    for (size_t i = 0; i < len * 8; ++i) {
        size_t byte_idx = i / 8;
        int bit_idx = 7 - (int)(i % 8);
        uint32_t bit = (data[byte_idx] >> bit_idx) & 1u;

        uint32_t carry = ((reg & topbit) != 0) ? 1 : 0;
        reg = ((reg << 1) | bit) & mask;
        if (carry == 1) {
            reg ^= poly;
        }
    }
    return reg;
}
