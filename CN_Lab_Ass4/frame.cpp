#include "frame.hpp"
#include "utils.hpp"
#include <cstring>
#include <stdexcept>

CDMAFrame::CDMAFrame() 
    : src_id(0), dest_id(0xFF), type(CDMAFrameType::DATA), seq_no(0), length(0), fcs(0) {}

CDMAFrame::CDMAFrame(uint8_t src, uint8_t dest, CDMAFrameType ftype, uint16_t seq, const std::vector<uint8_t>& data)
    : src_id(src), dest_id(dest), type(ftype), seq_no(seq), length((uint16_t)data.size()), payload(data), fcs(0) {}

std::vector<uint8_t> CDMAFrame::serialize() const {
    std::vector<uint8_t> buf;
    buf.reserve(7 + length + 4);

    buf.push_back(src_id);
    buf.push_back(dest_id);
    buf.push_back(static_cast<uint8_t>(type));

    // Big-endian seq_no
    buf.push_back(static_cast<uint8_t>((seq_no >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(seq_no & 0xFF));

    // Big-endian length
    buf.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(length & 0xFF));

    // Payload
    buf.insert(buf.end(), payload.begin(), payload.end());

    // Big-endian fcs
    buf.push_back(static_cast<uint8_t>((fcs >> 24) & 0xFF));
    buf.push_back(static_cast<uint8_t>((fcs >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((fcs >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(fcs & 0xFF));

    return buf;
}

CDMAFrame CDMAFrame::deserialize(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < 11) { // 7 header + 4 fcs minimum
        throw std::invalid_argument("Buffer too small for CDMAFrame header.");
    }

    CDMAFrame f;
    f.src_id = buffer[0];
    f.dest_id = buffer[1];
    f.type = static_cast<CDMAFrameType>(buffer[2]);

    f.seq_no = (static_cast<uint16_t>(buffer[3]) << 8) | buffer[4];
    f.length = (static_cast<uint16_t>(buffer[5]) << 8) | buffer[6];

    if (buffer.size() < (size_t)(7 + f.length + 4)) {
        throw std::invalid_argument("Buffer truncated: expected payload and fcs missing.");
    }

    f.payload.assign(buffer.begin() + 7, buffer.begin() + 7 + f.length);

    size_t fcs_idx = 7 + f.length;
    f.fcs = (static_cast<uint32_t>(buffer[fcs_idx]) << 24) |
            (static_cast<uint32_t>(buffer[fcs_idx + 1]) << 16) |
            (static_cast<uint32_t>(buffer[fcs_idx + 2]) << 8) |
            static_cast<uint32_t>(buffer[fcs_idx + 3]);

    return f;
}

void CDMAFrame::calculate_fcs(int scheme) {
    // FCS covers header fields (bytes 0 to 6) and payload
    std::vector<uint8_t> cover_data;
    cover_data.reserve(7 + length);

    cover_data.push_back(src_id);
    cover_data.push_back(dest_id);
    cover_data.push_back(static_cast<uint8_t>(type));
    cover_data.push_back(static_cast<uint8_t>((seq_no >> 8) & 0xFF));
    cover_data.push_back(static_cast<uint8_t>(seq_no & 0xFF));
    cover_data.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
    cover_data.push_back(static_cast<uint8_t>(length & 0xFF));
    cover_data.insert(cover_data.end(), payload.begin(), payload.end());

    if (scheme == 1) {
        fcs = computeChecksum16(cover_data.data(), cover_data.size());
    } else {
        fcs = computeCRC32(cover_data.data(), cover_data.size());
    }
}

bool CDMAFrame::verify_fcs(int scheme) const {
    std::vector<uint8_t> cover_data;
    cover_data.reserve(7 + length);

    cover_data.push_back(src_id);
    cover_data.push_back(dest_id);
    cover_data.push_back(static_cast<uint8_t>(type));
    cover_data.push_back(static_cast<uint8_t>((seq_no >> 8) & 0xFF));
    cover_data.push_back(static_cast<uint8_t>(seq_no & 0xFF));
    cover_data.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
    cover_data.push_back(static_cast<uint8_t>(length & 0xFF));
    cover_data.insert(cover_data.end(), payload.begin(), payload.end());

    uint32_t expected_fcs = 0;
    if (scheme == 1) {
        expected_fcs = computeChecksum16(cover_data.data(), cover_data.size());
    } else {
        expected_fcs = computeCRC32(cover_data.data(), cover_data.size());
    }

    return (fcs == expected_fcs);
}

std::vector<std::vector<int>> CDMAFrame::toChips(const WalshCode& wc) const {
    std::vector<uint8_t> bytes = serialize();
    return wc.encodeBytes(bytes, src_id);
}

CDMAFrame CDMAFrame::fromChips(const std::vector<std::vector<int>>& composite_chips, 
                               int station_id, 
                               const WalshCode& wc, 
                               size_t total_frame_bytes) {
    std::vector<uint8_t> bytes = wc.decodeBytes(composite_chips, station_id, total_frame_bytes);
    return deserialize(bytes);
}
