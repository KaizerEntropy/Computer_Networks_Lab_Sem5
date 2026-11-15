#include "channel.hpp"
#include <iostream>
#include <random>

namespace bdtp {

static std::mt19937& get_channel_rng() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

static double rand_prob() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(get_channel_rng());
}

static int rand_range(int lo, int hi) {
    if (lo >= hi) return lo;
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(get_channel_rng());
}

ChannelOutcome SimulatedChannel::process_outgoing(uint8_t* buffer, size_t len, int seq_num, uint8_t proto_id) {
    ChannelOutcome outcome;
    std::string proto_str = protocol_to_string(proto_id);

    // 1. Check for packet loss
    if (prob_loss > 0.0 && rand_prob() < prob_loss) {
        outcome.status = ChannelStatus::LOST;
        outcome.was_lost = true;
        outcome.summary = "[Channel] " + color::BOLD_RED + "PACKET LOST" + color::RESET +
                          " | Seq #" + std::to_string(seq_num) + " (" + proto_str + ") dropped in transit";
        return outcome;
    }

    // 2. Check for corruption / bit error
    if (prob_error > 0.0 && rand_prob() < prob_error) {
        int err_type = fixed_error_type;
        if (err_type < 0) {
            // Cycle through error types (1 through 7)
            err_type = (round_robin_counter % 7) + 1;
            round_robin_counter++;
        }

        // Apply error injection
        outcome.injection_info = inject_error(buffer, len, err_type, detection_scheme);
        outcome.was_corrupted = true;
        outcome.status = ChannelStatus::CORRUPTED;
        outcome.summary = "[Channel] " + color::BOLD_YELLOW + "PACKET CORRUPTED" + color::RESET +
                          " | Seq #" + std::to_string(seq_num) + " (" + proto_str + ") -> " +
                          outcome.injection_info.description;
    }

    // 3. Check for transmission delay / jitter
    if (prob_delay > 0.0 && rand_prob() < prob_delay) {
        int delay = rand_range(delay_min_ms, delay_max_ms);
        outcome.was_delayed = true;
        outcome.delay_ms = delay;
        if (outcome.status != ChannelStatus::CORRUPTED) {
            outcome.status = ChannelStatus::DELAYED;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        std::string delay_msg = " [Delayed " + std::to_string(delay) + " ms]";
        if (outcome.summary.empty()) {
            outcome.summary = "[Channel] " + color::BOLD_BLUE + "PACKET DELAYED" + color::RESET +
                              " | Seq #" + std::to_string(seq_num) + " (" + proto_str + ")" + delay_msg;
        } else {
            outcome.summary += delay_msg;
        }
    }

    if (outcome.status == ChannelStatus::DELIVERED && outcome.summary.empty()) {
        outcome.summary = "[Channel] " + color::BOLD_GREEN + "CLEAN" + color::RESET +
                          " | Seq #" + std::to_string(seq_num) + " (" + proto_str + ") forwarded successfully";
    }

    return outcome;
}

bool SimulatedChannel::should_drop_ack(int ack_seq) {
    if (prob_ack_loss > 0.0 && rand_prob() < prob_ack_loss) {
        std::cout << color::RED << "[Channel] ACK #" << ack_seq << " DROPPED (ACK Loss Simulation)\n" << color::RESET;
        return true;
    }
    return false;
}

} // namespace bdtp

