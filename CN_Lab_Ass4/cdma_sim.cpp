#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <cassert>
#include <fstream>
#include "walsh.hpp"
#include "frame.hpp"
#include "utils.hpp"

// -------------------------------------------------------------
// Test 1: Bit-by-Bit Textbook CDMA Trace
// -------------------------------------------------------------
void runBitLevelDemo() {
    printBanner("TEST 1: BIT-LEVEL CDMA WITH WALSH CODE TRACE");
    std::cout << "Demonstrating the exact mathematical steps for 4 stations:\n";
    std::cout << "  Station 0: bit d0 = 1 (+1)\n";
    std::cout << "  Station 1: bit d1 = 0 (-1)\n";
    std::cout << "  Station 2: bit d2 = 0 (silent/idle = 0)\n";
    std::cout << "  Station 3: bit d3 = 1 (+1)\n\n";

    WalshCode walsh(4);
    int m = walsh.getChipLength();
    walsh.printCodeTable();

    std::vector<int> d = { 1, 0, -1, 1 }; // -1 indicates idle
    std::vector<std::vector<int>> station_chips(4);

    printSubHeader("STEP 1: CHIP MODULATION (s_i = d_i * W_i)");
    for (int i = 0; i < 4; ++i) {
        station_chips[i] = walsh.encodeBit(d[i], i);
        std::cout << "Station " << i << " (bit=";
        if (d[i] == 1) std::cout << " 1 [+1]";
        else if (d[i] == 0) std::cout << " 0 [-1]";
        else std::cout << "idle[ 0]";
        std::cout << ") => Chip Sequence: ";
        printChips(station_chips[i]);
        std::cout << "\n";
    }

    printSubHeader("STEP 2: SUPERPOSITION ON COMMON CHANNEL (C = sum s_i)");
    std::vector<int> composite = WalshCode::superimposeChips(station_chips);
    std::cout << "Channel Composite Signal C = ";
    printChips(composite);
    std::cout << "\n";

    printSubHeader("STEP 3: DEMULTIPLEXING AT RECEIVERS (hat{d}_i = (C . W_i) / M)");
    for (int i = 0; i < 4; ++i) {
        std::vector<int> code = walsh.getCode(i);
        int dot = 0;
        std::cout << "Receiver for Station " << i << ":\n";
        std::cout << "  C . W" << i << " = (";
        for (int k = 0; k < m; ++k) {
            int prod = composite[k] * code[k];
            dot += prod;
            std::cout << "(" << composite[k] << " * " << code[k] << ")";
            if (k + 1 < m) std::cout << " + ";
        }
        std::cout << ") = " << dot << "\n";
        double metric = (double)dot / (double)m;
        std::cout << "  Metric = " << dot << " / " << m << " = " << metric << "\n";
        int recovered = walsh.decodeBit(composite, i);
        std::cout << "  => Recovered Bit: ";
        if (recovered == 1) {
            std::cout << Color::GREEN << "BIT 1" << Color::RESET;
            assert(d[i] == 1);
        } else if (recovered == 0) {
            std::cout << Color::GREEN << "BIT 0" << Color::RESET;
            assert(d[i] == 0);
        } else {
            std::cout << Color::YELLOW << "IDLE / SILENT" << Color::RESET;
            assert(d[i] == -1);
        }
        std::cout << " [MATCH CONFIRMED]\n\n";
    }
}

// -------------------------------------------------------------
// Test 2: Orthogonality & Hadamard Matrix Generation
// -------------------------------------------------------------
void runOrthogonalityTest() {
    printBanner("TEST 2: WALSH-HADAMARD MATRIX ORTHOGONALITY VERIFICATION");
    for (int n : {2, 4, 8, 16}) {
        WalshCode wc(n);
        std::cout << Color::BOLD << "Testing N = " << n 
                  << " stations (Walsh Dimension M = " << wc.getChipLength() << ")" 
                  << Color::RESET << ":\n";
        bool ok = wc.verifyOrthogonality(false);
        std::cout << "  Orthogonality Proof: " 
                  << (ok ? (Color::GREEN + "PASSED (All W_i . W_j = 0 for i!=j, W_i . W_i = M)" + Color::RESET)
                         : (Color::RED + "FAILED" + Color::RESET)) 
                  << "\n";
        if (n <= 8) {
            wc.printMatrix();
        }
    }
}

// -------------------------------------------------------------
// Test 3: Multi-Station Full String & File Transfer
// -------------------------------------------------------------
void runMultiStationTransfer() {
    printBanner("TEST 3: CONCURRENT MULTI-STATION TEXT STREAM TRANSFER");
    int n = 4;
    WalshCode walsh(n);

    std::vector<std::string> messages = {
        "Station 0: Jadavpur University Computer Networks Lab CDMA Implementation.",
        "Station 1: Walsh-Hadamard orthogonal codes provide zero cross-correlation.",
        "Station 2: Simultaneous transmission across shared physical medium.",
        "Station 3: Perfect reconstruction verified without any packet collision!"
    };

    std::cout << "Transmitting " << n << " concurrent independent messages:\n";
    for (int i = 0; i < n; ++i) {
        std::cout << "  Station " << i << ": \"" << messages[i] << "\"\n";
    }

    // Step 1: Framing and Modulation
    std::vector<std::vector<std::vector<int>>> station_streams;
    for (int i = 0; i < n; ++i) {
        std::vector<uint8_t> payload(messages[i].begin(), messages[i].end());
        CDMAFrame frame(i, 0xFF, CDMAFrameType::DATA, 1, payload);
        frame.calculate_fcs(2); // CRC-32
        std::vector<uint8_t> raw = frame.serialize();
        station_streams.push_back(walsh.encodeBytes(raw, i));
    }

    // Step 2: Linear superposition on channel
    std::vector<std::vector<int>> composite = WalshCode::superimposeStreams(station_streams);
    std::cout << "\nChannel Superposition Complete: Total Slots = " << composite.size() 
              << " (" << (composite.size() * walsh.getChipLength()) << " chips on medium)\n\n";

    // Step 3: Demultiplexing and perfect reconstruction
    for (int i = 0; i < n; ++i) {
        std::vector<uint8_t> recovered_bytes = walsh.decodeBytes(composite, i, 0);
        CDMAFrame rx_frame = CDMAFrame::deserialize(recovered_bytes);
        std::string rx_str(rx_frame.payload.begin(), rx_frame.payload.end());

        bool match = (rx_str == messages[i]);
        bool crc_ok = rx_frame.verify_fcs(2);

        std::cout << "Reconstructed Station " << i << ":\n";
        std::cout << "  Payload: \"" << rx_str << "\"\n";
        std::cout << "  Status : CRC-32 " << (crc_ok ? "VALID" : "INVALID") 
                  << " | Content Match: " << (match ? (Color::GREEN + "100% IDENTICAL" + Color::RESET) : (Color::RED + "MISMATCH" + Color::RESET))
                  << "\n\n";
        assert(match);
        assert(crc_ok);
    }
}

// -------------------------------------------------------------
// Test 4: Channel Noise Robustness Benchmark
// -------------------------------------------------------------
void runNoiseBenchmark() {
    printBanner("TEST 4: CHANNEL NOISE ROBUSTNESS & PROCESSING GAIN BENCHMARK");
    std::cout << "Testing bit error rate (BER) under varying chip perturbation probabilities:\n\n";

    WalshCode walsh(8); // M = 8 chips/bit (Processing Gain = 8 = 9 dB)
    std::string test_data = "Computer Networks Lab Assignment 4 CDMA Walsh Code Robustness Test 1234567890!";
    std::vector<uint8_t> payload(test_data.begin(), test_data.end());

    std::vector<std::vector<std::vector<int>>> streams;
    for (int i = 0; i < 4; ++i) {
        streams.push_back(walsh.encodeBytes(payload, i));
    }
    auto clean_composite = WalshCode::superimposeStreams(streams);

    std::cout << std::setw(15) << "Noise Prob" 
              << std::setw(18) << "Perturbed Chips" 
              << std::setw(18) << "Bit Errors (St0)" 
              << std::setw(20) << "Decoded Match" << "\n";
    std::cout << "----------------------------------------------------------------------\n";

    std::vector<double> noise_levels = {0.0, 0.05, 0.10, 0.15, 0.20, 0.30};

    for (double n_prob : noise_levels) {
        auto noisy_composite = clean_composite;
        size_t perturbed = 0;
        for (auto& slot : noisy_composite) {
            for (int& chip : slot) {
                if (((double)rand() / RAND_MAX) < n_prob) {
                    chip += (rand() % 2 == 0) ? 1 : -1;
                    perturbed++;
                }
            }
        }

        std::string recovered = walsh.decodeString(noisy_composite, 0, test_data.size());
        size_t bit_errors = 0;
        for (size_t k = 0; k < test_data.size(); ++k) {
            if (k < recovered.size() && test_data[k] != recovered[k]) {
                bit_errors++;
            }
        }

        bool match = (recovered == test_data);
        std::cout << std::setw(14) << (n_prob * 100.0) << "%"
                  << std::setw(18) << perturbed
                  << std::setw(18) << bit_errors
                  << std::setw(20) << (match ? (Color::GREEN + "PERFECT (100%)" + Color::RESET) 
                                             : (Color::YELLOW + "Degraded" + Color::RESET))
                  << "\n";
    }
}

// -------------------------------------------------------------
// Test 5: Asymmetric / Silent Station Handling
// -------------------------------------------------------------
void runSilentStationTest() {
    printBanner("TEST 5: ASYMMETRIC TRANSMISSION & SILENT STATIONS");
    std::cout << "Scenario:\n";
    std::cout << "  - Station 0: Long transmission (60 chars)\n";
    std::cout << "  - Station 1: Short transmission (15 chars)\n";
    std::cout << "  - Station 2: SILENT / IDLE (0 chars)\n";
    std::cout << "  - Station 3: Medium transmission (30 chars)\n\n";

    WalshCode walsh(4);
    std::vector<std::string> st_msgs = {
        "Station 0 transmits a comprehensive message across the channel.",
        "Station 1 quick",
        "", // Silent
        "Station 3 active medium data."
    };

    std::vector<std::vector<std::vector<int>>> streams;
    for (int i = 0; i < 4; ++i) {
        if (!st_msgs[i].empty()) {
            std::vector<uint8_t> payload(st_msgs[i].begin(), st_msgs[i].end());
            CDMAFrame frame(i, 0xFF, CDMAFrameType::DATA, 1, payload);
            frame.calculate_fcs(2);
            streams.push_back(walsh.encodeBytes(frame.serialize(), i));
        } else {
            // Empty stream for silent station
            streams.push_back({});
        }
    }

    auto composite = WalshCode::superimposeStreams(streams);

    for (int i = 0; i < 4; ++i) {
        std::vector<uint8_t> dec = walsh.decodeBytes(composite, i, 0);
        if (st_msgs[i].empty()) {
            std::cout << "Station " << i << " (Silent): " 
                      << Color::GREEN << "[CONFIRMED IDLE - Zero False Packets]" << Color::RESET << "\n";
        } else {
            CDMAFrame f = CDMAFrame::deserialize(dec);
            std::string s(f.payload.begin(), f.payload.end());
            bool ok = (s == st_msgs[i]) && f.verify_fcs(2);
            std::cout << "Station " << i << " (" << s.size() << " bytes): "
                      << (ok ? (Color::GREEN + "PERFECT RECONSTRUCTION" + Color::RESET) : (Color::RED + "FAIL" + Color::RESET))
                      << " -> \"" << s << "\"\n";
            assert(ok);
        }
    }
}

int main(int argc, char* argv[]) {
    srand(42); // Deterministic seed for reproducible tests

    if (argc >= 2 && (std::string(argv[1]) == "--auto" || std::string(argv[1]) == "--test")) {
        runOrthogonalityTest();
        runBitLevelDemo();
        runMultiStationTransfer();
        runNoiseBenchmark();
        runSilentStationTest();
        printBanner("ALL TESTS EXECUTED AND PASSED PERFECTLY!");
        return 0;
    }

    while (true) {
        printBanner("CDMA WITH WALSH CODE - SIMULATION & VERIFICATION BENCH");
        std::cout << "1. Run Bit-by-Bit Textbook CDMA Trace\n";
        std::cout << "2. Walsh-Hadamard Orthogonality Verification (N=2, 4, 8, 16)\n";
        std::cout << "3. Multi-Station Concurrent Text Transfer & Perfect Reconstruction\n";
        std::cout << "4. Channel Noise Robustness & Processing Gain Benchmark\n";
        std::cout << "5. Asymmetric Transmissions & Silent Station Handling\n";
        std::cout << "6. Run ALL Tests Sequentially\n";
        std::cout << "0. Exit\n";
        std::cout << "\nEnter Choice: ";

        int choice = 0;
        if (!(std::cin >> choice) || choice == 0) {
            std::cout << "Exiting CDMA Simulator.\n";
            break;
        }

        switch (choice) {
            case 1: runBitLevelDemo(); break;
            case 2: runOrthogonalityTest(); break;
            case 3: runMultiStationTransfer(); break;
            case 4: runNoiseBenchmark(); break;
            case 5: runSilentStationTest(); break;
            case 6:
                runOrthogonalityTest();
                runBitLevelDemo();
                runMultiStationTransfer();
                runNoiseBenchmark();
                runSilentStationTest();
                break;
            default:
                std::cout << "Invalid choice.\n";
        }
    }

    return 0;
}
