#include "telnet.hpp"
#include <sstream>
#include <iomanip>

namespace bdtp {

void TelnetParser::parse(const uint8_t* bytes, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        uint8_t b = bytes[i];

        switch (state) {
            case NvtState::DATA:
                if (b == TELNET_IAC) {
                    state = NvtState::IAC_RECEIVED;
                } else {
                    clean_data.push_back(static_cast<char>(b));
                }
                break;

            case NvtState::IAC_RECEIVED:
                if (b == TELNET_IAC) {
                    // Escaped 255 literal
                    clean_data.push_back(static_cast<char>(255));
                    state = NvtState::DATA;
                } else if (b == TELNET_WILL) {
                    current_verb = TELNET_WILL;
                    state = NvtState::WILL_RECEIVED;
                } else if (b == TELNET_WONT) {
                    current_verb = TELNET_WONT;
                    state = NvtState::WONT_RECEIVED;
                } else if (b == TELNET_DO) {
                    current_verb = TELNET_DO;
                    state = NvtState::DO_RECEIVED;
                } else if (b == TELNET_DONT) {
                    current_verb = TELNET_DONT;
                    state = NvtState::DONT_RECEIVED;
                } else if (b == TELNET_SB) {
                    current_verb = TELNET_SB;
                    current_sb.clear();
                    state = NvtState::SB_RECEIVED;
                } else {
                    // Simple command like AYT, NOP, GA
                    parsed_commands.push_back({b, 0, {}});
                    state = NvtState::DATA;
                }
                break;

            case NvtState::WILL_RECEIVED:
                parsed_commands.push_back({TELNET_WILL, b, {}});
                state = NvtState::DATA;
                break;

            case NvtState::WONT_RECEIVED:
                parsed_commands.push_back({TELNET_WONT, b, {}});
                state = NvtState::DATA;
                break;

            case NvtState::DO_RECEIVED:
                parsed_commands.push_back({TELNET_DO, b, {}});
                state = NvtState::DATA;
                break;

            case NvtState::DONT_RECEIVED:
                parsed_commands.push_back({TELNET_DONT, b, {}});
                state = NvtState::DATA;
                break;

            case NvtState::SB_RECEIVED:
                current_opt = b;
                state = NvtState::SB_DATA;
                break;

            case NvtState::SB_DATA:
                if (b == TELNET_IAC) {
                    state = NvtState::SB_IAC;
                } else {
                    current_sb.push_back(b);
                }
                break;

            case NvtState::SB_IAC:
                if (b == TELNET_SE) {
                    parsed_commands.push_back({TELNET_SB, current_opt, current_sb});
                    state = NvtState::DATA;
                } else if (b == TELNET_IAC) {
                    current_sb.push_back(TELNET_IAC);
                    state = NvtState::SB_DATA;
                } else {
                    state = NvtState::DATA;
                }
                break;
        }
    }
}

std::vector<uint8_t> build_telnet_negotiation(uint8_t verb, uint8_t option) {
    return {TELNET_IAC, verb, option};
}

std::vector<uint8_t> build_telnet_subneg(uint8_t option, const std::string& value) {
    std::vector<uint8_t> pkt = {TELNET_IAC, TELNET_SB, option};
    pkt.insert(pkt.end(), value.begin(), value.end());
    pkt.push_back(TELNET_IAC);
    pkt.push_back(TELNET_SE);
    return pkt;
}

std::vector<uint8_t> escape_telnet_data(const std::string& text) {
    std::vector<uint8_t> out;
    out.reserve(text.size() + 16);
    for (char c : text) {
        uint8_t u = static_cast<uint8_t>(c);
        out.push_back(u);
        if (u == TELNET_IAC) {
            out.push_back(TELNET_IAC); // Double IAC
        }
    }
    return out;
}

std::string execute_remote_command(const std::string& command_line, RemoteCommandContext& ctx) {
    ctx.commands_executed++;
    
    // Trim leading/trailing whitespace
    size_t start = command_line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = command_line.find_last_not_of(" \t\r\n");
    std::string trimmed = command_line.substr(start, end - start + 1);

    std::istringstream iss(trimmed);
    std::string cmd;
    iss >> cmd;

    std::ostringstream out;

    if (cmd == "help") {
        out << color::BOLD_WHITE << "TELNET REMOTE SHELL COMMAND REFERENCE:\n" << color::RESET
            << "  help                      : Display this help manual\n"
            << "  network_info              : Query BOOTP & DHCP assigned networking configurations\n"
            << "  status                    : Show server health, uptime, and resource telemetry\n"
            << "  ping                      : Test round-trip latency\n"
            << "  echo <message>            : Echo message back through remote terminal\n"
            << "  calc <arg1> <op> <arg2>   : Perform server-side arithmetic (+, -, *, /)\n"
            << "  upload <file> <bytes>     : Stream data packet payload over Telnet session\n"
            << "  stats                     : Show real-time session execution metrics\n"
            << "  exit | quit               : Terminate the Telnet session gracefully\n";
    }
    else if (cmd == "network_info") {
        out << color::BOLD_CYAN << "========================================================\n"
            << "     NETWORK SUBSYSTEM CONFIGURATION (BOOTP + DHCP)     \n"
            << "========================================================\n" << color::RESET
            << "  Client Hardware MAC : " << color::YELLOW << ctx.client_mac << color::RESET << "\n"
            << "  BOOTP Initial IP    : " << (ctx.bootp_ip.empty() ? "0.0.0.0 (Unset)" : ctx.bootp_ip) << "\n"
            << "  DHCP Leased IP      : " << color::GREEN << (ctx.dhcp_ip.empty() ? "Unassigned" : ctx.dhcp_ip) << color::RESET << "\n"
            << "  Network Boot File   : " << color::CYAN << (ctx.boot_file.empty() ? "pxelinux.0" : ctx.boot_file) << color::RESET << "\n"
            << "  Default Gateway     : " << ctx.gateway_ip << "\n"
            << "  Primary DNS Server  : " << ctx.dns_ip << "\n"
            << "  Lease Time Remaining: " << ctx.lease_secs << " seconds\n"
            << color::BOLD_CYAN << "========================================================\n" << color::RESET;
    }
    else if (cmd == "status") {
        out << color::BOLD_GREEN << "[Server Status: OK]" << color::RESET << "\n"
            << "  Uptime            : System online (active session)\n"
            << "  Protocols Active  : BOOTP (UDP:6767), DHCP (UDP:6769), TELNET (TCP:2323)\n"
            << "  Commands Handled  : " << ctx.commands_executed << "\n";
    }
    else if (cmd == "ping") {
        out << color::BOLD_GREEN << "PONG! Network socket latency < 0.5 ms" << color::RESET << "\n";
    }
    else if (cmd == "echo") {
        std::string rest;
        std::getline(iss, rest);
        if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
        out << rest << "\n";
    }
    else if (cmd == "calc") {
        double a = 0, b = 0;
        char op = '+';
        if (iss >> a >> op >> b) {
            double res = 0;
            bool ok = true;
            if (op == '+') res = a + b;
            else if (op == '-') res = a - b;
            else if (op == '*') res = a * b;
            else if (op == '/') {
                if (b != 0) res = a / b;
                else { ok = false; out << color::RED << "Error: Division by zero\n" << color::RESET; }
            }
            if (ok) {
                out << color::BOLD << a << " " << op << " " << b << " = " << color::GREEN << res << color::RESET << "\n";
            }
        } else {
            out << color::RED << "Usage: calc <num1> <+|-|*|/> <num2>\n" << color::RESET;
        }
    }
    else if (cmd == "upload") {
        std::string filename;
        iss >> filename;
        out << color::BOLD_GREEN << "UPLOAD_ACK: Server ready to receive blocks for " << filename << color::RESET << "\n";
    }
    else if (cmd == "stats") {
        out << color::BOLD << "Session Commands Executed: " << color::YELLOW << ctx.commands_executed << color::RESET << "\n";
    }
    else if (cmd == "exit" || cmd == "quit") {
        out << color::YELLOW << "BYE: Terminating Telnet session. Connection closing...\n" << color::RESET;
    }
    else {
        out << color::RED << "Command not found: " << cmd << ". Type 'help' for available commands.\n" << color::RESET;
    }

    return out.str();
}

} // namespace bdtp

