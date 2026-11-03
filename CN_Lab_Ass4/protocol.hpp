#pragma once

#include <cstdint>
#include <vector>
#include <cstring>

constexpr uint16_t CDMA_NET_MAGIC = 0xCD44;

enum class NetMsgType : uint8_t {
    REGISTER_SENDER   = 1, // Sender announces its transmission
    CHIP_DATA         = 2, // Sender sends chip sequence
    REGISTER_RECEIVER = 3, // Receiver registers to receive composite signal
    COMPOSITE_DATA    = 4, // Channel broadcasts composite signal
    ACK_OK            = 5, // Acknowledgment
    RESET             = 6  // Reset channel for next run
};

#pragma pack(push, 1)
struct NetPacketHeader {
    uint16_t magic;          // CDMA_NET_MAGIC
    uint8_t  msg_type;       // NetMsgType
    uint8_t  station_id;     // Source Station ID (or target for receiver)
    uint32_t num_slots;      // Number of bit slots (each slot has M chips)
    uint16_t chip_len;       // Walsh dimension M
    uint32_t total_chips;    // num_slots * chip_len
};
#pragma pack(pop)

inline std::vector<uint8_t> serializeChips(const NetPacketHeader& hdr, const std::vector<std::vector<int>>& stream) {
    std::vector<uint8_t> buf;
    size_t payload_bytes = hdr.total_chips * sizeof(int16_t);
    buf.resize(sizeof(NetPacketHeader) + payload_bytes);

    std::memcpy(buf.data(), &hdr, sizeof(NetPacketHeader));

    int16_t* chip_ptr = reinterpret_cast<int16_t*>(buf.data() + sizeof(NetPacketHeader));
    size_t idx = 0;
    for (const auto& slot : stream) {
        for (int chip : slot) {
            chip_ptr[idx++] = static_cast<int16_t>(chip);
        }
    }
    return buf;
}

inline std::vector<std::vector<int>> deserializeChips(const uint8_t* payload, uint32_t num_slots, uint16_t chip_len) {
    std::vector<std::vector<int>> stream(num_slots, std::vector<int>(chip_len, 0));
    const int16_t* chip_ptr = reinterpret_cast<const int16_t*>(payload);

    size_t idx = 0;
    for (uint32_t s = 0; s < num_slots; ++s) {
        for (uint16_t k = 0; k < chip_len; ++k) {
            stream[s][k] = static_cast<int>(chip_ptr[idx++]);
        }
    }
    return stream;
}
