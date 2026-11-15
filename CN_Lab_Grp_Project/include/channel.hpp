#pragma once

#include "common.hpp"
#include "error_injection.hpp"
#include <random>
#include <chrono>
#include <thread>

namespace bdtp {

enum class ChannelStatus {
    DELIVERED,
    LOST,
    CORRUPTED,
    DELAYED
};

struct ChannelOutcome {
    ChannelStatus status;
    bool was_lost;
    bool was_corrupted;
    bool was_delayed;
    int delay_ms;
    InjectionResult injection_info;
    std::string summary;

    ChannelOutcome()
        : status(ChannelStatus::DELIVERED), was_lost(false),
          was_corrupted(false), was_delayed(false), delay_ms(0) {}
};

class SimulatedChannel {
public:
    double prob_loss;
    double prob_error;
    double prob_delay;
    double prob_ack_loss;
    int delay_min_ms;
    int delay_max_ms;
    int fixed_error_type; // -1 for auto / round-robin
    int detection_scheme;

    SimulatedChannel(double loss = 0.0, double error = 0.0, double delay = 0.0, double ack_loss = 0.0)
        : prob_loss(loss), prob_error(error), prob_delay(delay), prob_ack_loss(ack_loss),
          delay_min_ms(60), delay_max_ms(180), fixed_error_type(-1),
          detection_scheme(5) {} // Default CRC-32

    ChannelOutcome process_outgoing(uint8_t* buffer, size_t len, int seq_num, uint8_t proto_id);
    bool should_drop_ack(int ack_seq);

private:
    int round_robin_counter = 1;
};

} // namespace bdtp

