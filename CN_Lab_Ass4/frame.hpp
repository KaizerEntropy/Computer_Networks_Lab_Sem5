#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include "walsh.hpp"

enum class CDMAFrameType : uint8_t {
    REGISTER = 0,
    DATA     = 1,
    ACK      = 2,
    FIN      = 3
};

/**
 * @brief Structured frame for CDMA transmission.
 * 
 * Header:
 *   [0]   Source Station ID (uint8_t)
 *   [1]   Destination ID    (uint8_t, 0xFF = broadcast / channel)
 *   [2]   Frame Type        (uint8_t)
 *   [3-4] Sequence Number   (uint16_t)
 *   [5-6] Payload Length    (uint16_t)
 * Payload:
 *   [7 .. 7+len-1] Data bytes
 * Trailer:
 *   [7+len .. 7+len+3] FCS (CRC-32 or Checksum-16)
 */
struct CDMAFrame {
    uint8_t src_id;
    uint8_t dest_id;
    CDMAFrameType type;
    uint16_t seq_no;
    uint16_t length;
    std::vector<uint8_t> payload;
    uint32_t fcs;

    CDMAFrame();
    CDMAFrame(uint8_t src, uint8_t dest, CDMAFrameType ftype, uint16_t seq, const std::vector<uint8_t>& data);

    // Serialization & Deserialization
    std::vector<uint8_t> serialize() const;
    static CDMAFrame deserialize(const std::vector<uint8_t>& buffer);

    // FCS calculation (1 = Checksum-16, 2 = CRC-32)
    void calculate_fcs(int scheme = 2);
    bool verify_fcs(int scheme = 2) const;

    // CDMA Chip modulation & demodulation
    std::vector<std::vector<int>> toChips(const WalshCode& wc) const;
    static CDMAFrame fromChips(const std::vector<std::vector<int>>& composite_chips, 
                               int station_id, 
                               const WalshCode& wc, 
                               size_t total_frame_bytes);
};
