#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <iostream>

/**
 * @brief Class representing Walsh-Hadamard codes and CDMA modulation/demodulation engine.
 * 
 * Generates orthogonal Walsh codes of dimension M = 2^k >= N stations,
 * where H_1 = [1]
 *       H_{2k} = [ H_k   H_k ]
 *                [ H_k  -H_k ]
 */
class WalshCode {
private:
    int num_stations;                      // Number of logical stations (n)
    int chip_length;                       // Dimension of Walsh matrix (M = 2^k >= n)
    std::vector<std::vector<int>> matrix;  // M x M Walsh-Hadamard matrix (+1 and -1)

    // Helper to generate recursive Hadamard matrix of size M
    static std::vector<std::vector<int>> generateHadamard(int m);

    // Compute smallest power of 2 >= n
    static int nextPowerOfTwo(int n);

public:
    /**
     * @brief Construct WalshCode for given number of stations.
     * @param n Number of stations (default 4)
     */
    explicit WalshCode(int n = 4);

    // Getters
    int getNumStations() const { return num_stations; }
    int getChipLength() const { return chip_length; }
    const std::vector<std::vector<int>>& getMatrix() const { return matrix; }

    /**
     * @brief Get the unique Walsh code assigned to station_id.
     * @param station_id 0-indexed station ID (0 <= station_id < chip_length)
     * @return Vector of M chips (+1 or -1)
     */
    std::vector<int> getCode(int station_id) const;

    /**
     * @brief Verify orthogonality of all generated codes.
     * Checks that:
     *   W_i . W_j = M  (if i == j)
     *   W_i . W_j = 0  (if i != j)
     * @param verbose Print full dot-product matrix if true
     * @return true if all pairs satisfy orthogonality
     */
    bool verifyOrthogonality(bool verbose = false) const;

    /**
     * @brief Print the Walsh matrix in formatted table form.
     */
    void printMatrix(std::ostream& os = std::cout) const;

    /**
     * @brief Print the station-to-code assignment table.
     */
    void printCodeTable(std::ostream& os = std::cout) const;

    // -------------------------------------------------------------
    // Modulation / Encoding
    // -------------------------------------------------------------

    /**
     * @brief Encode a single bit into a chip vector.
     * @param bit_val 1 -> (+1) * W_i
     *                0 -> (-1) * W_i
     *               -1 (idle/silent) -> (0) * W_i = [0, 0, ..., 0]
     * @param station_id Station transmitting the bit
     * @return Chip vector of length M
     */
    std::vector<int> encodeBit(int bit_val, int station_id) const;

    /**
     * @brief Encode a byte sequence into a sequence of chip vectors.
     * Each byte produces 8 chip vectors (MSB first).
     * @param data Byte payload to encode
     * @param station_id Station transmitting
     * @return Vector of chip vectors (size: data.size() * 8)
     */
    std::vector<std::vector<int>> encodeBytes(const std::vector<uint8_t>& data, int station_id) const;

    /**
     * @brief Encode a string into a sequence of chip vectors.
     */
    std::vector<std::vector<int>> encodeString(const std::string& str, int station_id) const;

    // -------------------------------------------------------------
    // Demodulation / Decoding
    // -------------------------------------------------------------

    /**
     * @brief Decode a single bit from a composite channel chip vector for a station.
     * Computes:
     *   sum = composite_chips . W_{station_id}
     *   metric = sum / M
     * 
     * @param composite_chips Superimposed chip vector from channel
     * @param station_id Station whose bit is being recovered
     * @return 1 (bit 1), 0 (bit 0), or -1 (idle/silent)
     */
    int decodeBit(const std::vector<int>& composite_chips, int station_id) const;

    /**
     * @brief Decode a stream of composite chip vectors into recovered bytes.
     * @param composite_stream Vector of composite chip vectors
     * @param station_id Target station ID
     * @param expected_bytes Maximum bytes to decode (0 = decode all available)
     * @return Recovered byte vector
     */
    std::vector<uint8_t> decodeBytes(const std::vector<std::vector<int>>& composite_stream, 
                                     int station_id, 
                                     size_t expected_bytes = 0) const;

    /**
     * @brief Decode a stream of composite chip vectors into a recovered string.
     */
    std::string decodeString(const std::vector<std::vector<int>>& composite_stream, 
                             int station_id, 
                             size_t expected_bytes = 0) const;

    // -------------------------------------------------------------
    // Channel Operations (Superposition & Noise)
    // -------------------------------------------------------------

    /**
     * @brief Superimpose chip vectors from multiple stations for one bit slot.
     * Computes element-wise sum: C[k] = sum_{s=0}^{N-1} station_chips[s][k]
     * @param station_chips Vector of chip vectors (one per station)
     * @return Composite chip vector of length M
     */
    static std::vector<int> superimposeChips(const std::vector<std::vector<int>>& station_chips);

    /**
     * @brief Superimpose full streams from multiple stations.
     * Automatically handles streams of different lengths by zero-padding (idle).
     * @param station_streams Vector of station streams (each stream is vector of chip vectors)
     * @return Composite stream of chip vectors
     */
    static std::vector<std::vector<int>> superimposeStreams(const std::vector<std::vector<std::vector<int>>>& station_streams);

    /**
     * @brief Add random noise to composite chip vector.
     * @param composite_chips Input chip vector
     * @param noise_prob Probability of a chip being perturbed by noise (+-1)
     * @return Noisy chip vector
     */
    static std::vector<int> addNoise(const std::vector<int>& composite_chips, double noise_prob);
};
