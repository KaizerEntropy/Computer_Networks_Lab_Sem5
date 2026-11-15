#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace bdtp {

enum ErrorType : uint8_t {
    ERR_NONE            = 0,
    ERR_SINGLE_BIT      = 1,
    ERR_TWO_ISOLATED    = 2,
    ERR_ODD_BITS        = 3,
    ERR_BURST           = 4,
    ERR_CHECKSUM_BLIND  = 5,
    ERR_CRC_BLIND       = 6,
    ERR_RANDOM_BYTE     = 7
};

struct InjectionResult {
    bool injected;
    int error_type;
    std::string description;
    std::vector<int> flipped_bits;

    InjectionResult() : injected(false), error_type(0), description("Clean") {}
};

std::string get_error_type_name(int error_type);

InjectionResult inject_error(uint8_t* data, size_t len, int error_type, int scheme_code);

void flip_bit(uint8_t* data, size_t bit_pos);

} // namespace bdtp

