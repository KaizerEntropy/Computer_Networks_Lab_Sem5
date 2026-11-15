#include "problems.hpp"
#include <iostream>
#include <string>

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [problem_number | all] [--verbose]\n"
              << "Options:\n"
              << "  1          Run Problem 1 (Crossover Cable Host-to-Host)\n"
              << "  2          Run Problem 2 (LAN-A with 3 Hosts using Hub)\n"
              << "  3          Run Problem 3 (LAN-B with 3 Hosts using Switch & ARP/MAC)\n"
              << "  4          Run Problem 4 (Interconnect Hub & Switch via Crossover)\n"
              << "  5          Run Problem 5 (Dual-LAN JU-Main & JU-SL with Routers)\n"
              << "  6          Run Problem 6 (DHCP Server Deployment on JU LANs)\n"
              << "  7          Run Problem 7 (Integrated CSE LAN: DHCP, DNS, Web, FTP)\n"
              << "  all        Run all problems sequentially (Default)\n"
              << "  --verbose  Enable detailed packet encapsulation & trace logging\n";
}

int main(int argc, char* argv[]) {
    std::string target = "all";
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else {
            target = arg;
        }
    }

    if (target == "1") {
        net::run_problem_1(std::cout, verbose);
    } else if (target == "2") {
        net::run_problem_2(std::cout, verbose);
    } else if (target == "3") {
        net::run_problem_3(std::cout, verbose);
    } else if (target == "4") {
        net::run_problem_4(std::cout, verbose);
    } else if (target == "5") {
        net::run_problem_5(std::cout, verbose);
    } else if (target == "6") {
        net::run_problem_6(std::cout, verbose);
    } else if (target == "7") {
        net::run_problem_7(std::cout, verbose);
    } else if (target == "all") {
        net::run_all_problems(std::cout, verbose);
    } else {
        std::cerr << "Unknown target: " << target << "\n";
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}

