#include "walsh.hpp"
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>

int WalshCode::nextPowerOfTwo(int n) {
    if (n <= 1) return 2; // Minimum dimension 2 for CDMA
    int power = 1;
    while (power < n) {
        power <<= 1;
    }
    return power;
}

std::vector<std::vector<int>> WalshCode::generateHadamard(int m) {
    if (m <= 1) {
        return {{1}};
    }

    int half = m / 2;
    std::vector<std::vector<int>> h_half = generateHadamard(half);
    std::vector<std::vector<int>> h(m, std::vector<int>(m));

    for (int i = 0; i < half; ++i) {
        for (int j = 0; j < half; ++j) {
            h[i][j]               =  h_half[i][j]; // Top-left
            h[i][j + half]        =  h_half[i][j]; // Top-right
            h[i + half][j]        =  h_half[i][j]; // Bottom-left
            h[i + half][j + half] = -h_half[i][j]; // Bottom-right
        }
    }
    return h;
}

WalshCode::WalshCode(int n) {
    if (n < 1) n = 1;
    num_stations = n;
    chip_length = nextPowerOfTwo(n);
    matrix = generateHadamard(chip_length);
}

std::vector<int> WalshCode::getCode(int station_id) const {
    if (station_id < 0 || station_id >= chip_length) {
        throw std::out_of_range("Station ID out of range for generated Walsh matrix.");
    }
    return matrix[station_id];
}

bool WalshCode::verifyOrthogonality(bool verbose) const {
    bool orthogonal = true;
    if (verbose) {
        std::cout << "\n--- Walsh Orthogonality Matrix (W_i . W_j) ---\n";
        std::cout << "      ";
        for (int j = 0; j < chip_length; ++j) {
            std::cout << std::setw(5) << ("W" + std::to_string(j));
        }
        std::cout << "\n";
    }

    for (int i = 0; i < chip_length; ++i) {
        if (verbose) std::cout << "W" << std::setw(2) << i << " : ";
        for (int j = 0; j < chip_length; ++j) {
            int dot = 0;
            for (int k = 0; k < chip_length; ++k) {
                dot += matrix[i][k] * matrix[j][k];
            }
            if (verbose) {
                std::cout << std::setw(5) << dot;
            }
            if (i == j) {
                if (dot != chip_length) orthogonal = false;
            } else {
                if (dot != 0) orthogonal = false;
            }
        }
        if (verbose) std::cout << "\n";
    }

    if (verbose) {
        std::cout << "Orthogonality Check: " << (orthogonal ? "PASSED (Perfect)" : "FAILED") << "\n\n";
    }
    return orthogonal;
}

void WalshCode::printMatrix(std::ostream& os) const {
    os << "\n=======================================================\n";
    os << "   Walsh-Hadamard Matrix H_" << chip_length << " (Dimension: " << chip_length << "x" << chip_length << ")\n";
    os << "=======================================================\n";
    for (int i = 0; i < chip_length; ++i) {
        os << "W" << std::setw(2) << i << " | ";
        for (int j = 0; j < chip_length; ++j) {
            if (matrix[i][j] > 0) os << " +1 ";
            else os << " -1 ";
        }
        os << " |\n";
    }
    os << "=======================================================\n";
}

void WalshCode::printCodeTable(std::ostream& os) const {
    os << "\n+------------+--------------------------------------------+\n";
    os << "| Station ID | Walsh Chip Sequence (Code Word)             |\n";
    os << "+------------+--------------------------------------------+\n";
    for (int i = 0; i < num_stations; ++i) {
        os << "| Station " << std::setw(2) << i << " | [";
        for (int k = 0; k < chip_length; ++k) {
            if (matrix[i][k] > 0) os << "+1";
            else os << "-1";
            if (k + 1 < chip_length) os << ", ";
        }
        os << "] |\n";
    }
    os << "+------------+--------------------------------------------+\n";
}

std::vector<int> WalshCode::encodeBit(int bit_val, int station_id) const {
    std::vector<int> code = getCode(station_id);
    std::vector<int> chips(chip_length, 0);

    if (bit_val == 1) {
        // Data bit 1 -> (+1) * W_i
        for (int k = 0; k < chip_length; ++k) chips[k] = code[k];
    } else if (bit_val == 0) {
        // Data bit 0 -> (-1) * W_i
        for (int k = 0; k < chip_length; ++k) chips[k] = -code[k];
    } else {
        // Idle/silent (-1) -> 0 * W_i
        for (int k = 0; k < chip_length; ++k) chips[k] = 0;
    }
    return chips;
}

std::vector<std::vector<int>> WalshCode::encodeBytes(const std::vector<uint8_t>& data, int station_id) const {
    std::vector<std::vector<int>> result;
    result.reserve(data.size() * 8);

    for (uint8_t byte : data) {
        for (int b = 7; b >= 0; --b) {
            int bit = (byte >> b) & 1;
            result.push_back(encodeBit(bit, station_id));
        }
    }
    return result;
}

std::vector<std::vector<int>> WalshCode::encodeString(const std::string& str, int station_id) const {
    std::vector<uint8_t> data(str.begin(), str.end());
    return encodeBytes(data, station_id);
}

int WalshCode::decodeBit(const std::vector<int>& composite_chips, int station_id) const {
    if ((int)composite_chips.size() != chip_length) {
        throw std::invalid_argument("Composite chips dimension does not match Walsh chip length.");
    }

    std::vector<int> code = getCode(station_id);
    int dot_product = 0;
    for (int k = 0; k < chip_length; ++k) {
        dot_product += composite_chips[k] * code[k];
    }

    double metric = (double)dot_product / (double)chip_length;

    // Decision rule with noise tolerance:
    // Metric approx +1 -> bit 1
    // Metric approx -1 -> bit 0
    // Metric approx  0 -> idle (silent)
    if (metric > 0.5) {
        return 1;
    } else if (metric < -0.5) {
        return 0;
    } else {
        return -1; // Idle
    }
}

std::vector<uint8_t> WalshCode::decodeBytes(const std::vector<std::vector<int>>& composite_stream, 
                                            int station_id, 
                                            size_t expected_bytes) const {
    std::vector<uint8_t> recovered;
    uint8_t current_byte = 0;
    int bit_count = 0;

    for (const auto& chips : composite_stream) {
        int bit = decodeBit(chips, station_id);
        if (bit == -1) {
            // Idle bit encountered
            if (expected_bytes == 0) {
                // If no expected length specified, stop decoding on idle
                break;
            } else {
                // Treat as 0 or stop
                bit = 0;
            }
        }

        current_byte = (current_byte << 1) | (bit & 1);
        bit_count++;

        if (bit_count == 8) {
            recovered.push_back(current_byte);
            current_byte = 0;
            bit_count = 0;

            if (expected_bytes > 0 && recovered.size() >= expected_bytes) {
                break;
            }
        }
    }
    return recovered;
}

std::string WalshCode::decodeString(const std::vector<std::vector<int>>& composite_stream, 
                                    int station_id, 
                                    size_t expected_bytes) const {
    std::vector<uint8_t> bytes = decodeBytes(composite_stream, station_id, expected_bytes);
    return std::string(bytes.begin(), bytes.end());
}

std::vector<int> WalshCode::superimposeChips(const std::vector<std::vector<int>>& station_chips) {
    if (station_chips.empty()) return {};
    size_t m = station_chips[0].size();
    std::vector<int> composite(m, 0);

    for (const auto& chips : station_chips) {
        for (size_t k = 0; k < m; ++k) {
            composite[k] += chips[k];
        }
    }
    return composite;
}

std::vector<std::vector<int>> WalshCode::superimposeStreams(const std::vector<std::vector<std::vector<int>>>& station_streams) {
    if (station_streams.empty()) return {};

    size_t max_slots = 0;
    size_t m = 0;
    for (const auto& stream : station_streams) {
        max_slots = std::max(max_slots, stream.size());
        if (!stream.empty() && m == 0) {
            m = stream[0].size();
        }
    }

    if (m == 0) return {};

    std::vector<std::vector<int>> composite_stream(max_slots, std::vector<int>(m, 0));

    for (size_t slot = 0; slot < max_slots; ++slot) {
        for (const auto& stream : station_streams) {
            if (slot < stream.size()) {
                for (size_t k = 0; k < m; ++k) {
                    composite_stream[slot][k] += stream[slot][k];
                }
            }
            // If station finished earlier, it transmits 0 (silent), adding nothing
        }
    }

    return composite_stream;
}

std::vector<int> WalshCode::addNoise(const std::vector<int>& composite_chips, double noise_prob) {
    std::vector<int> noisy = composite_chips;
    for (int& chip : noisy) {
        double r = (double)rand() / RAND_MAX;
        if (r < noise_prob) {
            // Add +-1 noise perturbation
            int perturb = (rand() % 2 == 0) ? 1 : -1;
            chip += perturb;
        }
    }
    return noisy;
}
