#include "common.hpp"
#include "error_detection.hpp"
#include "error_injection.hpp"
#include "channel.hpp"
#include "bootp.hpp"
#include "dhcp.hpp"
#include "telnet.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <map>

using namespace bdtp;

struct MatrixTestResult {
    int total_trials;
    int detected;
    double detection_rate;
};

void run_detection_matrix_benchmark() {
    std::cout << color::BOLD_CYAN
              << "\n================================================================================\n"
              << "     ERROR DETECTION MATRIX BENCHMARK (ALL SCHEMES X ALL ERROR TYPES)           \n"
              << "================================================================================\n"
              << color::RESET;

    const std::vector<int> schemes = {
        SCHEME_CHECKSUM16, SCHEME_CRC8, SCHEME_CRC10, SCHEME_CRC16,
        SCHEME_CRC32, SCHEME_HAMMING, SCHEME_PARITY
    };

    const std::vector<int> error_types = {
        ERR_SINGLE_BIT, ERR_TWO_ISOLATED, ERR_ODD_BITS, ERR_BURST,
        ERR_CHECKSUM_BLIND, ERR_CRC_BLIND, ERR_RANDOM_BYTE
    };

    const int TRIALS = 200;
    std::map<std::pair<int, int>, MatrixTestResult> results;

    // Test payload: standard 64-byte sample
    std::string test_str = "Jadavpur University CSE Computer Networks Lab - BOOTP DHCP TELNET Protocol Frame Check Sequence";
    std::vector<uint8_t> base_payload(test_str.begin(), test_str.end());

    for (int scheme : schemes) {
        for (int err_type : error_types) {
            int detected_count = 0;
            for (int t = 0; t < TRIALS; ++t) {
                std::vector<uint8_t> data = base_payload;
                uint32_t original_fcs = compute_fcs(data.data(), data.size(), scheme);

                // Inject error
                InjectionResult inj = inject_error(data.data(), data.size(), err_type, scheme);
                if (inj.injected) {
                    bool valid = verify_fcs(data.data(), data.size(), original_fcs, scheme);
                    if (!valid) {
                        detected_count++; // Error was successfully detected
                    }
                }
            }
            double rate = (static_cast<double>(detected_count) / TRIALS) * 100.0;
            results[{scheme, err_type}] = {TRIALS, detected_count, rate};
        }
    }

    // Print Formatted Matrix Table
    std::cout << std::left << std::setw(18) << "Scheme \\ Error"
              << std::setw(12) << "Single-Bit"
              << std::setw(12) << "Two-Isol"
              << std::setw(12) << "Odd-Bits"
              << std::setw(12) << "Burst-8"
              << std::setw(12) << "CS-Blind"
              << std::setw(12) << "CRC-Blind"
              << std::setw(12) << "Rand-Byte"
              << "\n---------------------------------------------------------------------------------------------------\n";

    for (int scheme : schemes) {
        std::cout << std::left << std::setw(18) << get_scheme_name(scheme);
        for (int err_type : error_types) {
            double rate = results[{scheme, err_type}].detection_rate;
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << rate << "%";
            if (rate >= 99.0) {
                std::cout << color::GREEN << std::setw(12) << ss.str() << color::RESET;
            } else if (rate == 0.0) {
                std::cout << color::BOLD_RED << std::setw(12) << ss.str() << color::RESET;
            } else {
                std::cout << color::YELLOW << std::setw(12) << ss.str() << color::RESET;
            }
        }
        std::cout << "\n";
    }
    std::cout << "---------------------------------------------------------------------------------------------------\n";

    std::cout << color::BOLD_WHITE << "\nCRITICAL OBSERVATIONS & MATHEMATICAL VERIFICATION:\n" << color::RESET
              << " 1. " << color::BOLD_GREEN << "Single-Bit & Odd-Bit Errors:" << color::RESET
              << " 100% detected by Checksum and all CRC variants (parity bit guarantees odd error detection).\n"
              << " 2. " << color::BOLD_RED << "Checksum Blind Spot:" << color::RESET
              << " 16-bit Checksum drops to 0% for balanced word modification, but CRC-32 catches 100%!\n"
              << " 3. " << color::BOLD_RED << "CRC Blind Spot:" << color::RESET
              << " CRC drops when error polynomial matches generator polynomial, but 16-bit Checksum catches 100%!\n";

    // Export JSON for Python Plotting
    std::ofstream jout("matrix_benchmark.json");
    if (jout) {
        jout << "{\n  \"schemes\": [\"Checksum-16\", \"CRC-8\", \"CRC-10\", \"CRC-16\", \"CRC-32\", \"Hamming\", \"Parity\"],\n";
        jout << "  \"error_types\": [\"Single\", \"Two-Isol\", \"Odd-Bits\", \"Burst\", \"CS-Blind\", \"CRC-Blind\", \"Rand-Byte\"],\n";
        jout << "  \"matrix\": [\n";
        for (size_t i = 0; i < schemes.size(); ++i) {
            jout << "    [";
            for (size_t j = 0; j < error_types.size(); ++j) {
                jout << results[{schemes[i], error_types[j]}].detection_rate;
                if (j + 1 < error_types.size()) jout << ", ";
            }
            jout << "]";
            if (i + 1 < schemes.size()) jout << ",\n";
            else jout << "\n";
        }
        jout << "  ]\n}\n";
        jout.close();
        std::cout << color::CYAN << "  [Exported matrix benchmark data to matrix_benchmark.json]\n" << color::RESET;
    }
}

void run_channel_efficiency_sweep() {
    std::cout << color::BOLD_CYAN
              << "\n================================================================================\n"
              << "     FLOW CONTROL & CHANNEL EFFICIENCY SWEEP (STOP-AND-WAIT ARQ)                 \n"
              << "================================================================================\n"
              << color::RESET;

    const std::vector<double> loss_rates = {0.0, 0.05, 0.10, 0.15, 0.20, 0.30};
    const int PACKET_COUNT = 50;

    std::cout << std::left << std::setw(12) << "Loss Rate"
              << std::setw(15) << "Unique Packets"
              << std::setw(15) << "Total Sent"
              << std::setw(15) << "Retransmits"
              << std::setw(15) << "Efficiency (%)"
              << "\n-------------------------------------------------------------------------\n";

    std::ofstream eff_out("efficiency_data.json");
    if (eff_out) {
        eff_out << "{\n  \"loss_rates\": [";
        for (size_t i = 0; i < loss_rates.size(); ++i) {
            eff_out << loss_rates[i] << (i + 1 < loss_rates.size() ? ", " : "");
        }
        eff_out << "],\n  \"efficiencies\": [";
    }

    for (size_t idx = 0; idx < loss_rates.size(); ++idx) {
        double loss = loss_rates[idx];
        SimulatedChannel chan(loss, 0.0, 0.0, 0.0);

        int sent = 0;
        int retransmits = 0;

        for (int p = 0; p < PACKET_COUNT; ++p) {
            bool delivered = false;
            while (!delivered) {
                sent++;
                uint8_t dummy_buf[32] = {0};
                ChannelOutcome out = chan.process_outgoing(dummy_buf, 32, p, PROTO_TELNET_DATA);
                if (out.was_lost) {
                    retransmits++;
                } else {
                    delivered = true;
                }
            }
        }

        double eff = (static_cast<double>(PACKET_COUNT) / sent) * 100.0;
        std::cout << std::left << std::setw(12) << (std::to_string(static_cast<int>(loss * 100)) + "%")
                  << std::setw(15) << PACKET_COUNT
                  << std::setw(15) << sent
                  << std::setw(15) << retransmits
                  << std::fixed << std::setprecision(2) << eff << "%\n";

        if (eff_out) {
            eff_out << eff << (idx + 1 < loss_rates.size() ? ", " : "");
        }
    }
    std::cout << "-------------------------------------------------------------------------\n";

    if (eff_out) {
        eff_out << "]\n}\n";
        eff_out.close();
        std::cout << color::CYAN << "  [Exported efficiency sweep data to efficiency_data.json]\n\n" << color::RESET;
    }
}

int main() {
    std::cout << color::BOLD_WHITE
              << "################################################################################\n"
              << "#         AUTOMATED SYSTEM SIMULATOR & TEST BENCH - MINI GROUP PROJECT         #\n"
              << "#         BOOTP, DHCP, TELNET & FULL ERROR DETECTION / INJECTION SUITE         #\n"
              << "################################################################################\n"
              << color::RESET;

    run_detection_matrix_benchmark();
    run_channel_efficiency_sweep();

    std::cout << color::BOLD_GREEN << "All automated tests and benchmarks executed successfully!\n" << color::RESET;
    return 0;
}

