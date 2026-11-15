#include "error_detection.hpp"
#include <iostream>

namespace bdtp {

SchemeDetails get_scheme_details(int scheme_code) {
    switch (scheme_code) {
        case SCHEME_CHECKSUM16:
            return {SCHEME_CHECKSUM16, "16-bit Checksum", 16, 0, "RFC 1071 1's complement sum"};
        case SCHEME_CRC8:
            return {SCHEME_CRC8, "CRC-8", 8, 0xD5, "x^8 + x^7 + x^6 + x^4 + x^2 + 1"};
        case SCHEME_CRC10:
            return {SCHEME_CRC10, "CRC-10", 10, 0x233, "x^10 + x^9 + x^5 + x^4 + x + 1"};
        case SCHEME_CRC16:
            return {SCHEME_CRC16, "CRC-16", 16, 0x8005, "x^16 + x^15 + x^2 + 1 (USB/IBM)"};
        case SCHEME_CRC32:
            return {SCHEME_CRC32, "CRC-32", 32, 0x04C11DB7, "IEEE 802.3 Ethernet Standard"};
        case SCHEME_HAMMING:
            return {SCHEME_HAMMING, "Hamming SEC-DED", 32, 0, "Extended Multi-Block (7,4)/(8,4)"};
        case SCHEME_PARITY:
            return {SCHEME_PARITY, "2D Parity (LRC+VRC)", 32, 0, "Longitudinal + Vertical Redundancy"};
        default:
            return {SCHEME_NONE, "None", 0, 0, "No error detection"};
    }
}

std::string get_scheme_name(int scheme_code) {
    return get_scheme_details(scheme_code).name;
}

uint32_t compute_checksum16(const uint8_t* data, size_t len) {
    uint32_t sum = 0;
    size_t i = 0;
    
    while (i + 1 < len) {
        uint16_t word = (static_cast<uint16_t>(data[i]) << 8) | static_cast<uint16_t>(data[i + 1]);
        sum += word;
        if (sum > 0xFFFF) {
            sum = (sum & 0xFFFF) + 1;
        }
        i += 2;
    }
    
    if (i < len) {
        uint16_t word = static_cast<uint16_t>(data[i]) << 8;
        sum += word;
        if (sum > 0xFFFF) {
            sum = (sum & 0xFFFF) + 1;
        }
    }
    
    return static_cast<uint32_t>(~sum & 0xFFFF);
}

uint32_t compute_crc(const uint8_t* data, size_t len, int width, uint32_t poly) {
    uint32_t mask = (width == 32) ? 0xFFFFFFFFu : ((1u << width) - 1u);
    uint32_t topbit = 1u << (width - 1);
    uint32_t reg = 0;

    for (size_t byte_idx = 0; byte_idx < len; ++byte_idx) {
        uint8_t cur_byte = data[byte_idx];
        for (int bit_idx = 7; bit_idx >= 0; --bit_idx) {
            uint32_t bit = (cur_byte >> bit_idx) & 1u;
            uint32_t carry = ((reg & topbit) != 0) ? 1 : 0;
            reg = ((reg << 1) | bit) & mask;
            if (carry) {
                reg ^= poly;
            }
        }
    }
    
    // Append width zero-bits to flush register
    for (int i = 0; i < width; ++i) {
        uint32_t carry = ((reg & topbit) != 0) ? 1 : 0;
        reg = (reg << 1) & mask;
        if (carry) {
            reg ^= poly;
        }
    }

    return reg;
}

uint32_t compute_hamming(const uint8_t* data, size_t len) {
    // 32-bit composite Hamming SEC-DED check word across blocks
    uint32_t parity_accum = 0;
    uint32_t overall_parity = 0;

    for (size_t i = 0; i < len; ++i) {
        uint8_t b = data[i];
        // Position index 1-based
        uint32_t pos = static_cast<uint32_t>(i + 1);
        
        // Count ones in byte
        int ones = 0;
        for (int k = 0; k < 8; ++k) {
            if ((b >> k) & 1) {
                ones++;
                // XOR position into parity syndrome
                parity_accum ^= (pos * 8 + k);
            }
        }
        overall_parity ^= (ones & 1);
    }

    // High 16 bits: syndrome accumulator, Low 16 bits: overall parity & linear weight
    uint32_t checksum_part = (parity_accum & 0xFFFF) << 16;
    uint32_t linear_part = (static_cast<uint32_t>(len) ^ (overall_parity * 0xAA55)) & 0xFFFF;
    return checksum_part | linear_part;
}

uint32_t compute_parity2d(const uint8_t* data, size_t len) {
    uint8_t lrc = 0; // Longitudinal Redundancy Check (XOR byte)
    uint16_t vrc_accum = 0; // Vertical Redundancy Check (even parity count)
    
    for (size_t i = 0; i < len; ++i) {
        uint8_t b = data[i];
        lrc ^= b;
        
        // Count set bits for even parity
        int parity = 0;
        for (int k = 0; k < 8; ++k) {
            if ((b >> k) & 1) parity ^= 1;
        }
        vrc_accum = (vrc_accum << 1) | parity;
        if (vrc_accum & 0x8000) vrc_accum ^= 0x1021; // Feedback polynomial
    }

    uint32_t fcs = (static_cast<uint32_t>(lrc) << 24) |
                   (static_cast<uint32_t>(vrc_accum & 0xFFFF) << 8) |
                   (static_cast<uint32_t>(len & 0xFF));
    return fcs;
}

uint32_t compute_fcs(const uint8_t* data, size_t len, int scheme_code) {
    switch (scheme_code) {
        case SCHEME_CHECKSUM16:
            return compute_checksum16(data, len);
        case SCHEME_CRC8:
            return compute_crc(data, len, 8, 0xD5);
        case SCHEME_CRC10:
            return compute_crc(data, len, 10, 0x233);
        case SCHEME_CRC16:
            return compute_crc(data, len, 16, 0x8005);
        case SCHEME_CRC32:
            return compute_crc(data, len, 32, 0x04C11DB7);
        case SCHEME_HAMMING:
            return compute_hamming(data, len);
        case SCHEME_PARITY:
            return compute_parity2d(data, len);
        default:
            return 0;
    }
}

bool verify_fcs(const uint8_t* data, size_t len, uint32_t received_fcs, int scheme_code) {
    if (scheme_code == SCHEME_NONE) return true;
    uint32_t calculated = compute_fcs(data, len, scheme_code);
    return calculated == received_fcs;
}

} // namespace bdtp

