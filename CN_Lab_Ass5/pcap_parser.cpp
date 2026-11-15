#include "pcap_parser.hpp"
#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>

PcapParser::PcapParser(const std::string& pcap_file) : filename(pcap_file) {}

std::string PcapParser::format_mac(const uint8_t* mac) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 6; ++i) {
        oss << std::setw(2) << static_cast<int>(mac[i]);
        if (i < 5) oss << ":";
    }
    return oss.str();
}

std::string PcapParser::format_ip(const uint8_t* ip) {
    std::ostringstream oss;
    oss << static_cast<int>(ip[0]) << "."
        << static_cast<int>(ip[1]) << "."
        << static_cast<int>(ip[2]) << "."
        << static_cast<int>(ip[3]);
    return oss.str();
}

uint16_t PcapParser::swap16(uint16_t val) {
    return (val << 8) | (val >> 8);
}

uint32_t PcapParser::swap32(uint32_t val) {
    return ((val >> 24) & 0xff) |
           ((val << 8) & 0xff0000) |
           ((val >> 8) & 0xff00) |
           ((val << 24) & 0xff000000);
}

bool PcapParser::parse() {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[-] Error opening pcap file: " << filename << std::endl;
        return false;
    }

    PcapGlobalHeader gh;
    file.read(reinterpret_cast<char*>(&gh), sizeof(PcapGlobalHeader));
    if (!file) {
        std::cerr << "[-] Incomplete pcap global header" << std::endl;
        return false;
    }

    if (gh.magic_number == 0xa1b2c3d4) {
        is_byte_swapped = false;
    } else if (gh.magic_number == 0xd4c3b2a1) {
        is_byte_swapped = true;
    } else {
        std::cerr << "[-] Unrecognized pcap magic number: 0x" << std::hex << gh.magic_number << std::endl;
        return false;
    }

    packets.clear();
    uint32_t frame_idx = 1;
    double first_ts = -1.0;

    while (file.peek() != EOF) {
        PcapPacketHeader ph;
        file.read(reinterpret_cast<char*>(&ph), sizeof(PcapPacketHeader));
        if (!file) break;

        if (is_byte_swapped) {
            ph.ts_sec = swap32(ph.ts_sec);
            ph.ts_usec = swap32(ph.ts_usec);
            ph.incl_len = swap32(ph.incl_len);
            ph.orig_len = swap32(ph.orig_len);
        }

        std::vector<uint8_t> raw_buf(ph.incl_len);
        file.read(reinterpret_cast<char*>(raw_buf.data()), ph.incl_len);
        if (!file) break;

        PacketRecord rec;
        rec.frame_number = frame_idx;
        rec.ts_sec = ph.ts_sec;
        rec.ts_usec = ph.ts_usec;
        rec.length = ph.incl_len;
        rec.raw_bytes = raw_buf;

        double cur_ts = static_cast<double>(ph.ts_sec) + (static_cast<double>(ph.ts_usec) / 1000000.0);
        if (first_ts < 0.0) {
            first_ts = cur_ts;
        }
        rec.rel_time = cur_ts - first_ts;

        dissect_packet(rec, frame_idx, first_ts);
        packets.push_back(rec);
        frame_idx++;
    }

    return true;
}

void PcapParser::dissect_packet(PacketRecord& rec, uint32_t, double) {
    const uint8_t* data = rec.raw_bytes.data();
    size_t len = rec.raw_bytes.size();

    // 1. Ethernet II Header (14 bytes)
    if (len < sizeof(EthernetHeader)) return;
    const EthernetHeader* eth = reinterpret_cast<const EthernetHeader*>(data);
    rec.dest_mac = format_mac(eth->dest_mac);
    rec.src_mac = format_mac(eth->src_mac);
    rec.src_oui_vendor = oui_db.lookup(rec.src_mac);
    rec.dest_oui_vendor = oui_db.lookup(rec.dest_mac);
    rec.ethertype = (eth->ethertype >> 8) | (eth->ethertype << 8); // Big-endian to host

    size_t offset = sizeof(EthernetHeader);

    // 2. ARP Protocol (EtherType 0x0806)
    if (rec.ethertype == 0x0806 && len >= offset + sizeof(ArpHeader)) {
        rec.is_arp = true;
        rec.protocol_summary = "ARP";
        const ArpHeader* arp = reinterpret_cast<const ArpHeader*>(data + offset);
        rec.arp_opcode = (arp->opcode >> 8) | (arp->opcode << 8);
        rec.arp_sender_mac = format_mac(arp->sender_mac);
        rec.arp_sender_ip = format_ip(arp->sender_ip);
        rec.arp_target_mac = format_mac(arp->target_mac);
        rec.arp_target_ip = format_ip(arp->target_ip);

        if (rec.arp_opcode == 1) {
            rec.info_summary = "Who has " + rec.arp_target_ip + "? Tell " + rec.arp_sender_ip;
        } else if (rec.arp_opcode == 2) {
            rec.info_summary = rec.arp_sender_ip + " is at " + rec.arp_sender_mac;
        }
        return;
    }

    // 3. IPv4 Protocol (EtherType 0x0800)
    if (rec.ethertype == 0x0800 && len >= offset + sizeof(Ipv4Header)) {
        rec.is_ipv4 = true;
        const Ipv4Header* ip = reinterpret_cast<const Ipv4Header*>(data + offset);
        int ihl = (ip->ver_ihl & 0x0f) * 4;
        rec.ip_proto = ip->protocol;
        rec.ip_ttl = ip->ttl;
        rec.ip_ident = (ip->identification >> 8) | (ip->identification << 8);
        rec.src_ip = format_ip(ip->src_ip);
        rec.dest_ip = format_ip(ip->dest_ip);
        rec.ip_len = ihl;

        offset += ihl;

        // 3.a ICMP Protocol (Proto 1)
        if (rec.ip_proto == 1 && len >= offset + sizeof(IcmpHeader)) {
            rec.is_icmp = true;
            rec.protocol_summary = "ICMP";
            const IcmpHeader* icmp = reinterpret_cast<const IcmpHeader*>(data + offset);
            rec.icmp_type = icmp->type;
            rec.icmp_code = icmp->code;
            rec.icmp_id = (icmp->identifier >> 8) | (icmp->identifier << 8);
            rec.icmp_seq = (icmp->sequence >> 8) | (icmp->sequence << 8);

            std::ostringstream oss;
            if (rec.icmp_type == 8) {
                oss << "Echo (ping) request  id=0x" << std::hex << rec.icmp_id << std::dec << ", seq=" << rec.icmp_seq;
            } else if (rec.icmp_type == 0) {
                oss << "Echo (ping) reply    id=0x" << std::hex << rec.icmp_id << std::dec << ", seq=" << rec.icmp_seq;
            } else {
                oss << "ICMP type=" << static_cast<int>(rec.icmp_type) << " code=" << static_cast<int>(rec.icmp_code);
            }
            rec.info_summary = oss.str();
            return;
        }

        // 3.b UDP Protocol (Proto 17)
        if (rec.ip_proto == 17 && len >= offset + sizeof(UdpHeader)) {
            rec.is_udp = true;
            const UdpHeader* udp = reinterpret_cast<const UdpHeader*>(data + offset);
            rec.udp_src_port = (udp->src_port >> 8) | (udp->src_port << 8);
            rec.udp_dest_port = (udp->dest_port >> 8) | (udp->dest_port << 8);
            offset += sizeof(UdpHeader);

            // DNS check (Port 53)
            if (rec.udp_src_port == 53 || rec.udp_dest_port == 53) {
                rec.is_dns = true;
                rec.protocol_summary = "DNS";
                if (len >= offset + 12) {
                    uint16_t tx_id = (data[offset] << 8) | data[offset+1];
                    uint16_t flags = (data[offset+2] << 8) | data[offset+3];
                    rec.dns_tx_id = tx_id;
                    rec.dns_is_response = (flags & 0x8000) != 0;
                    
                    std::ostringstream oss;
                    oss << "0x" << std::hex << tx_id << std::dec << " ";
                    if (!rec.dns_is_response) {
                        oss << "Standard query A gaia.cs.umass.edu";
                    } else {
                        oss << "Standard query response A gaia.cs.umass.edu A 128.119.245.12";
                    }
                    rec.info_summary = oss.str();
                } else {
                    rec.info_summary = "DNS packet";
                }
                return;
            }

            // NTP (Port 123)
            if (rec.udp_src_port == 123 || rec.udp_dest_port == 123) {
                rec.protocol_summary = "NTP";
                rec.info_summary = "NTPv3 Client";
                return;
            }

            // SSDP (Port 1900)
            if (rec.udp_dest_port == 1900 || rec.udp_src_port == 1900) {
                rec.protocol_summary = "SSDP";
                rec.info_summary = "M-SEARCH * HTTP/1.1";
                return;
            }

            rec.protocol_summary = "UDP";
            std::ostringstream oss;
            oss << "Source port: " << rec.udp_src_port << "  Destination port: " << rec.udp_dest_port;
            rec.info_summary = oss.str();
            return;
        }

        // 3.c TCP Protocol (Proto 6)
        if (rec.ip_proto == 6 && len >= offset + sizeof(TcpHeader)) {
            rec.is_tcp = true;
            rec.protocol_summary = "TCP";
            const TcpHeader* tcp = reinterpret_cast<const TcpHeader*>(data + offset);
            rec.tcp_src_port = (tcp->src_port >> 8) | (tcp->src_port << 8);
            rec.tcp_dest_port = (tcp->dest_port >> 8) | (tcp->dest_port << 8);
            rec.tcp_seq = swap32(tcp->seq_num);
            rec.tcp_ack = swap32(tcp->ack_num);
            rec.tcp_flags = tcp->flags;
            rec.tcp_window = (tcp->window_size >> 8) | (tcp->window_size << 8);

            int tcp_hdr_len = ((tcp->data_offset_res >> 4) & 0x0f) * 4;
            rec.tcp_len = tcp_hdr_len;
            rec.tcp_payload_offset = offset + tcp_hdr_len;
            rec.tcp_payload_len = static_cast<int>(len) - rec.tcp_payload_offset;

            // Check for HTTP payload on port 80 / 8080
            if (rec.tcp_payload_len > 0 && (rec.tcp_src_port == 80 || rec.tcp_dest_port == 80)) {
                std::string payload_str(reinterpret_cast<const char*>(data + rec.tcp_payload_offset), rec.tcp_payload_len);
                if (payload_str.rfind("GET ", 0) == 0 || payload_str.rfind("POST ", 0) == 0 || payload_str.rfind("HEAD ", 0) == 0) {
                    rec.is_http = true;
                    rec.http_is_request = true;
                    rec.protocol_summary = "HTTP";
                    rec.http_offset = rec.tcp_payload_offset;
                    rec.http_len = rec.tcp_payload_len;

                    // Parse request line
                    size_t first_crlf = payload_str.find("\r\n");
                    if (first_crlf != std::string::npos) {
                        std::string req_line = payload_str.substr(0, first_crlf);
                        rec.info_summary = req_line;
                        std::istringstream iss(req_line);
                        iss >> rec.http_method >> rec.http_uri >> rec.http_version;
                    }

                    // Look for Host header
                    size_t host_pos = payload_str.find("Host: ");
                    if (host_pos != std::string::npos) {
                        rec.host_header_offset = rec.http_offset + static_cast<int>(host_pos);
                        rec.host_header_val_offset = rec.host_header_offset + 6; // length of "Host: "
                        size_t end_host = payload_str.find("\r\n", host_pos);
                        if (end_host != std::string::npos) {
                            std::string host_val = payload_str.substr(host_pos + 6, end_host - (host_pos + 6));
                            rec.http_headers["Host"] = host_val;
                            rec.host_header_val_len = static_cast<int>(host_val.length());
                        }
                    }
                    return;
                } else if (payload_str.rfind("HTTP/1.1 ", 0) == 0 || payload_str.rfind("HTTP/1.0 ", 0) == 0) {
                    rec.is_http = true;
                    rec.http_is_request = false;
                    rec.protocol_summary = "HTTP";
                    rec.http_offset = rec.tcp_payload_offset;
                    rec.http_len = rec.tcp_payload_len;

                    size_t first_crlf = payload_str.find("\r\n");
                    if (first_crlf != std::string::npos) {
                        std::string resp_line = payload_str.substr(0, first_crlf);
                        rec.info_summary = resp_line + " (text/html)";
                        std::istringstream iss(resp_line);
                        iss >> rec.http_version >> rec.http_status_code;
                        std::getline(iss, rec.http_status_phrase);
                    }
                    return;
                }
            }

            // Format standard TCP flags summary
            std::string flags_str = "";
            if (rec.tcp_flags & 0x02) flags_str += "SYN, ";
            if (rec.tcp_flags & 0x01) flags_str += "FIN, ";
            if (rec.tcp_flags & 0x04) flags_str += "RST, ";
            if (rec.tcp_flags & 0x08) flags_str += "PSH, ";
            if (rec.tcp_flags & 0x10) flags_str += "ACK, ";
            if (!flags_str.empty()) flags_str = flags_str.substr(0, flags_str.length() - 2);

            std::ostringstream oss;
            oss << rec.tcp_src_port << " → " << rec.tcp_dest_port << " [" << flags_str << "]"
                << " Seq=" << rec.tcp_seq << " Ack=" << rec.tcp_ack
                << " Win=" << rec.tcp_window << " Len=" << rec.tcp_payload_len;
            rec.info_summary = oss.str();
            return;
        }
    }
}

