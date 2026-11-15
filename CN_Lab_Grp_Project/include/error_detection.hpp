#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace bdtp {

enum DetectionScheme : uint8_t {
    SCHEME_NONE       = 0,
    SCHEME_CHECKSUM16 = 1,
    SCHEME_CRC8       = 2,
    SCHEME_CRC10      = 3,
    SCHEME_CRC16      = 4,
    SCHEME_CRC32      = 5,
    SCHEME_HAMMING    = 6,
    SCHEME_PARITY     = 7
};

struct SchemeDetails {
    DetectionScheme scheme;
    std::string name;
    int width_bits;
    uint32_t poly;
    std::string description;
};

SchemeDetails get_scheme_details(int scheme_code);

// Calculation and Verification
uint32_t compute_checksum16(const uint8_t* data, size_t len);
uint32_t compute_crc(const uint8_t* data, size_t len, int width, uint32_t poly);
uint32_t compute_hamming(const uint8_t* data, size_t len);
uint32_t compute_parity2d(const uint8_t* data, size_t len);

uint32_t compute_fcs(const uint8_t* data, size_t len, int scheme_code);
bool verify_fcs(const uint8_t* data, size_t len, uint32_t received_fcs, int scheme_code);

std::string get_scheme_name(int scheme_code);

} // namespace bdtp

