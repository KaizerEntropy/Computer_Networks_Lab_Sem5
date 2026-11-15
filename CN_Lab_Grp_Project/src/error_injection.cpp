#include "error_injection.hpp"
#include "error_detection.hpp"
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <random>

namespace bdtp {

static std::mt19937& get_rng() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

static int rand_int(int lo, int hi) {
    if (lo >= hi) return lo;
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(get_rng());
}

std::string get_error_type_name(int error_type) {
    switch (error_type) {
        case ERR_NONE:           return "No error (clean)";
        case ERR_SINGLE_BIT:     return "Single-bit error";
        case ERR_TWO_ISOLATED:   return "Two isolated bit errors";
        case ERR_ODD_BITS:       return "Odd number of bit errors (3, 5, 7)";
        case ERR_BURST:          return "Burst error (8-bit span)";
        case ERR_CHECKSUM_BLIND: return "Checksum-blind error (CRC detects, Checksum misses)";
        case ERR_CRC_BLIND:      return "CRC-blind error (Checksum detects, CRC misses)";
        case ERR_RANDOM_BYTE:    return "Multi-byte random corruption";
        default:                 return "Unknown error";
    }
}

void flip_bit(uint8_t* data, size_t bit_pos) {
    size_t byte_idx = bit_pos / 8;
    int bit_offset = 7 - (bit_pos % 8);
    data[byte_idx] ^= (1u << bit_offset);
}

InjectionResult inject_error(uint8_t* data, size_t len, int error_type, int scheme_code) {
    InjectionResult res;
    res.error_type = error_type;
    if (len == 0 || error_type == ERR_NONE) {
        res.injected = false;
        res.description = "Clean transmission (no error injected)";
        return res;
    }

    size_t total_bits = len * 8;
    std::ostringstream oss;

    switch (error_type) {
        case ERR_SINGLE_BIT: {
            int pos = rand_int(0, total_bits - 1);
            flip_bit(data, pos);
            res.flipped_bits.push_back(pos);
            oss << "Single-bit flipped at bit pos " << pos << " (byte " << pos / 8 << ", bit " << pos % 8 << ")";
            res.injected = true;
            break;
        }

        case ERR_TWO_ISOLATED: {
            int p1 = rand_int(0, total_bits - 1);
            int p2 = rand_int(0, total_bits - 1);
            while (std::abs(p2 - p1) < 2) {
                p2 = rand_int(0, total_bits - 1);
            }
            flip_bit(data, p1);
            flip_bit(data, p2);
            res.flipped_bits.push_back(p1);
            res.flipped_bits.push_back(p2);
            oss << "Two isolated bits flipped at pos " << p1 << " and " << p2
                << " (separation distance = " << std::abs(p2 - p1) << " bits)";
            res.injected = true;
            break;
        }

        case ERR_ODD_BITS: {
            int counts[] = {3, 5, 7};
            int count = counts[rand_int(0, 2)];
            std::vector<int> positions;
            while (static_cast<int>(positions.size()) < count) {
                int p = rand_int(0, total_bits - 1);
                if (std::find(positions.begin(), positions.end(), p) == positions.end()) {
                    positions.push_back(p);
                }
            }
            for (int p : positions) {
                flip_bit(data, p);
                res.flipped_bits.push_back(p);
            }
            oss << "Odd number (" << count << ") of bits flipped";
            res.injected = true;
            break;
        }

        case ERR_BURST: {
            int burst_len = 8;
            if (total_bits <= static_cast<size_t>(burst_len)) {
                burst_len = static_cast<int>(total_bits);
            }
            int start = rand_int(0, total_bits - burst_len);
            // Flip first and last bit to define burst span
            flip_bit(data, start);
            flip_bit(data, start + burst_len - 1);
            res.flipped_bits.push_back(start);
            res.flipped_bits.push_back(start + burst_len - 1);
            
            // Randomly flip bits inside burst
            for (int i = 1; i < burst_len - 1; ++i) {
                if (rand_int(0, 1) == 1) {
                    flip_bit(data, start + i);
                    res.flipped_bits.push_back(start + i);
                }
            }
            oss << burst_len << "-bit burst error injected from bit " << start << " to " << (start + burst_len - 1);
            res.injected = true;
            break;
        }

        case ERR_CHECKSUM_BLIND: {
            // Checksum-blind error: Increment one 16-bit word and decrement another 16-bit word.
            // Sum invariant => Checksum unchanged! But CRC catches it.
            bool applied = false;
            if (len >= 4) {
                for (size_t off = 0; off + 3 < len; off += 2) {
                    uint16_t w1 = (static_cast<uint16_t>(data[off]) << 8) | static_cast<uint16_t>(data[off + 1]);
                    uint16_t w2 = (static_cast<uint16_t>(data[off + 2]) << 8) | static_cast<uint16_t>(data[off + 3]);
                    
                    if (w1 < 0xFFFE && w2 > 1) {
                        w1 += 1;
                        w2 -= 1;
                        data[off]     = static_cast<uint8_t>(w1 >> 8);
                        data[off + 1] = static_cast<uint8_t>(w1 & 0xFF);
                        data[off + 2] = static_cast<uint8_t>(w2 >> 8);
                        data[off + 3] = static_cast<uint8_t>(w2 & 0xFF);
                        oss << "Checksum-blind applied: balanced 16-bit word pair +/-1 at bytes ["
                            << off << ".." << off + 3 << "]";
                        applied = true;
                        res.injected = true;
                        break;
                    }
                }
            }
            if (!applied) {
                int p = rand_int(0, total_bits - 1);
                flip_bit(data, p);
                res.flipped_bits.push_back(p);
                oss << "Fallback single-bit flip at " << p;
                res.injected = true;
            }
            break;
        }

        case ERR_CRC_BLIND: {
            // CRC-blind error: Invert bits corresponding to the CRC generator polynomial.
            // If the error pattern E(x) is a multiple of G(x), CRC(E) = 0 => CRC misses!
            // But 16-bit Checksum easily detects it.
            SchemeDetails details = get_scheme_details(scheme_code);
            int width = details.width_bits;
            uint32_t poly = details.poly;

            if (width == 0 || width > 32) {
                width = 16;
                poly = 0x8005;
            }

            int poly_bits = width + 1; // e.g. 9 bits for CRC-8, 33 bits for CRC-32
            if (total_bits > static_cast<size_t>(poly_bits + 8)) {
                int start_bit = rand_int(0, total_bits - poly_bits - 2);
                
                // Flip top bit (x^width)
                flip_bit(data, start_bit);
                res.flipped_bits.push_back(start_bit);
                int flips = 1;

                // Flip coefficients of poly
                for (int b = 0; b < width; ++b) {
                    if ((poly & (1u << (width - 1 - b))) != 0) {
                        int pos = start_bit + 1 + b;
                        flip_bit(data, pos);
                        res.flipped_bits.push_back(pos);
                        flips++;
                    }
                }
                oss << "CRC-blind applied: injected generator poly pattern (" << flips << " bits flipped) at bit " << start_bit;
                res.injected = true;
            } else {
                int p = rand_int(0, total_bits - 1);
                flip_bit(data, p);
                res.flipped_bits.push_back(p);
                oss << "Fallback single-bit flip at " << p;
                res.injected = true;
            }
            break;
        }

        case ERR_RANDOM_BYTE: {
            int num_bytes = rand_int(1, std::min<int>(4, static_cast<int>(len)));
            for (int i = 0; i < num_bytes; ++i) {
                int byte_idx = rand_int(0, len - 1);
                uint8_t noise = static_cast<uint8_t>(rand_int(1, 255));
                data[byte_idx] ^= noise;
            }
            oss << "Multi-byte random XOR noise across " << num_bytes << " bytes";
            res.injected = true;
            break;
        }
    }

    res.description = oss.str();
    return res;
}

} // namespace bdtp

