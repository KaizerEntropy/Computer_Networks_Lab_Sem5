#pragma once

#include "common.hpp"
#include <string>
#include <vector>
#include <functional>

namespace bdtp {

// RFC 854 Telnet Commands
constexpr uint8_t TELNET_SE    = 240; // Subnegotiation End
constexpr uint8_t TELNET_NOP   = 241; // No Operation
constexpr uint8_t TELNET_DM    = 242; // Data Mark
constexpr uint8_t TELNET_BREAK = 243; // Break
constexpr uint8_t TELNET_IP    = 244; // Interrupt Process
constexpr uint8_t TELNET_AO    = 245; // Abort Output
constexpr uint8_t TELNET_AYT   = 246; // Are You There
constexpr uint8_t TELNET_EC    = 247; // Erase Character
constexpr uint8_t TELNET_EL    = 248; // Erase Line
constexpr uint8_t TELNET_GA    = 249; // Go Ahead
constexpr uint8_t TELNET_SB    = 250; // Subnegotiation Begin
constexpr uint8_t TELNET_WILL  = 251; // Will
constexpr uint8_t TELNET_WONT  = 252; // Won't
constexpr uint8_t TELNET_DO    = 253; // Do
constexpr uint8_t TELNET_DONT  = 254; // Don't
constexpr uint8_t TELNET_IAC   = 255; // Interpret As Command

// Telnet Options
constexpr uint8_t TELOPT_ECHO   = 1;  // RFC 857 Echo
constexpr uint8_t TELOPT_SGA    = 3;  // RFC 858 Suppress Go Ahead
constexpr uint8_t TELOPT_STATUS = 5;  // RFC 859 Status
constexpr uint8_t TELOPT_TTYPE  = 24; // RFC 1091 Terminal Type
constexpr uint8_t TELOPT_NAWS   = 31; // RFC 1073 Window Size

inline std::string telnet_cmd_to_string(uint8_t cmd) {
    switch (cmd) {
        case TELNET_SE:    return "SE";
        case TELNET_NOP:   return "NOP";
        case TELNET_DM:    return "DM";
        case TELNET_BREAK: return "BREAK";
        case TELNET_IP:    return "IP";
        case TELNET_AO:    return "AO";
        case TELNET_AYT:   return "AYT";
        case TELNET_EC:    return "EC";
        case TELNET_EL:    return "EL";
        case TELNET_GA:    return "GA";
        case TELNET_SB:    return "SB";
        case TELNET_WILL:  return "WILL";
        case TELNET_WONT:  return "WONT";
        case TELNET_DO:    return "DO";
        case TELNET_DONT:  return "DONT";
        case TELNET_IAC:   return "IAC";
        default:           return std::to_string(static_cast<int>(cmd));
    }
}

inline std::string telnet_opt_to_string(uint8_t opt) {
    switch (opt) {
        case TELOPT_ECHO:   return "ECHO(1)";
        case TELOPT_SGA:    return "SUPPRESS-GO-AHEAD(3)";
        case TELOPT_STATUS: return "STATUS(5)";
        case TELOPT_TTYPE:  return "TERMINAL-TYPE(24)";
        case TELOPT_NAWS:   return "NAWS(31)";
        default:            return "OPT(" + std::to_string(static_cast<int>(opt)) + ")";
    }
}

// ============================================================================
// Telnet Session State Machine
// ============================================================================
enum class NvtState {
    DATA,
    IAC_RECEIVED,
    WILL_RECEIVED,
    WONT_RECEIVED,
    DO_RECEIVED,
    DONT_RECEIVED,
    SB_RECEIVED,
    SB_DATA,
    SB_IAC
};

struct TelnetCommand {
    uint8_t verb; // WILL, WONT, DO, DONT, AYT, etc.
    uint8_t option;
    std::vector<uint8_t> subneg_data;
};

class TelnetParser {
public:
    NvtState state;
    std::vector<TelnetCommand> parsed_commands;
    std::string clean_data;

    TelnetParser() : state(NvtState::DATA) {}

    void parse(const uint8_t* bytes, size_t len);
    void reset() {
        state = NvtState::DATA;
        parsed_commands.clear();
        clean_data.clear();
    }

private:
    uint8_t current_verb = 0;
    uint8_t current_opt = 0;
    std::vector<uint8_t> current_sb;
};

// ============================================================================
// Telnet Packet & Shell Helpers
// ============================================================================
std::vector<uint8_t> build_telnet_negotiation(uint8_t verb, uint8_t option);
std::vector<uint8_t> build_telnet_subneg(uint8_t option, const std::string& value);
std::vector<uint8_t> escape_telnet_data(const std::string& text);

// Remote Command Execution Handler
struct RemoteCommandContext {
    std::string client_mac;
    std::string bootp_ip;
    std::string dhcp_ip;
    std::string boot_file;
    std::string gateway_ip;
    std::string dns_ip;
    uint32_t lease_secs;
    size_t commands_executed;
};

std::string execute_remote_command(const std::string& command_line, RemoteCommandContext& ctx);

} // namespace bdtp

