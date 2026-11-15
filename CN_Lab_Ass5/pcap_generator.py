#!/usr/bin/env python3
"""
pcap_generator.py - Synthetic PCAP Generator for Assignment 5 Traffic Analysis
Generates valid standard Libpcap capture files containing:
- ARP Request / Reply
- ICMP Echo (Ping) Request / Reply
- DNS Query / Response (gaia.cs.umass.edu)
- TCP 3-Way Handshake (SYN, SYN-ACK, ACK)
- HTTP GET Request and HTTP 200 OK Response
- TCP Teardown (FIN-ACK exchanges)
- Background UDP and TCP traffic for statistical distribution analysis
"""

import struct
import socket
import os
import time

def compute_checksum(data: bytes) -> int:
    if len(data) % 2 == 1:
        data += b'\x00'
    s = 0
    for i in range(0, len(data), 2):
        w = (data[i] << 8) + data[i+1]
        s += w
        while (s >> 16) > 0:
            s = (s & 0xffff) + (s >> 16)
    return ~s & 0xffff

def mac_to_bytes(mac_str: str) -> bytes:
    return bytes.fromhex(mac_str.replace(':', '').replace('-', ''))

def ip_to_bytes(ip_str: str) -> bytes:
    return socket.inet_aton(ip_str)

class PcapWriter:
    def __init__(self, filename: str):
        self.filename = filename
        self.file = open(filename, 'wb')
        # Libpcap global header
        # magic: 0xa1b2c3d4, v2.4, thiszone=0, sigfigs=0, snaplen=65535, linktype=1 (Ethernet)
        global_header = struct.pack('<IHHIIII', 0xa1b2c3d4, 2, 4, 0, 0, 65535, 1)
        self.file.write(global_header)

    def write_packet(self, raw_bytes: bytes, ts_sec: int, ts_usec: int):
        length = len(raw_bytes)
        packet_header = struct.pack('<IIII', ts_sec, ts_usec, length, length)
        self.file.write(packet_header)
        self.file.write(raw_bytes)

    def close(self):
        self.file.close()

def build_ethernet_frame(dst_mac: str, src_mac: str, ethertype: int, payload: bytes) -> bytes:
    return mac_to_bytes(dst_mac) + mac_to_bytes(src_mac) + struct.pack('>H', ethertype) + payload

def build_arp_packet(op: int, sender_mac: str, sender_ip: str, target_mac: str, target_ip: str) -> bytes:
    # HW Type: 1 (Ethernet), Proto Type: 0x0800 (IPv4), HW Size: 6, Proto Size: 4, Op: 1 (Req) / 2 (Reply)
    arp = struct.pack('>HHBBH', 1, 0x0800, 6, 4, op)
    arp += mac_to_bytes(sender_mac)
    arp += ip_to_bytes(sender_ip)
    arp += mac_to_bytes(target_mac)
    arp += ip_to_bytes(target_ip)
    return arp

def build_ipv4_packet(src_ip: str, dst_ip: str, proto: int, payload: bytes, ident: int = 0x5432, ttl: int = 64) -> bytes:
    version_ihl = (4 << 4) | 5
    tos = 0
    total_len = 20 + len(payload)
    flags_frag = 0x4000  # Don't fragment
    cksum = 0
    hdr_no_cksum = struct.pack('>BBHHHBBH4s4s', version_ihl, tos, total_len, ident, flags_frag, ttl, proto, cksum,
                               ip_to_bytes(src_ip), ip_to_bytes(dst_ip))
    cksum = compute_checksum(hdr_no_cksum)
    hdr = struct.pack('>BBHHHBBH4s4s', version_ihl, tos, total_len, ident, flags_frag, ttl, proto, cksum,
                      ip_to_bytes(src_ip), ip_to_bytes(dst_ip))
    return hdr + payload

def build_icmp_echo(icmp_type: int, code: int, identifier: int, seq_num: int, data: bytes) -> bytes:
    hdr = struct.pack('>BBHHH', icmp_type, code, 0, identifier, seq_num)
    cksum = compute_checksum(hdr + data)
    hdr = struct.pack('>BBHHH', icmp_type, code, cksum, identifier, seq_num)
    return hdr + data

def build_udp_packet(src_ip: str, dst_ip: str, src_port: int, dst_port: int, payload: bytes) -> bytes:
    length = 8 + len(payload)
    cksum = 0
    # Pseudo header for UDP checksum
    pseudo = ip_to_bytes(src_ip) + ip_to_bytes(dst_ip) + struct.pack('>BBH', 0, 17, length)
    udp_hdr = struct.pack('>HHHH', src_port, dst_port, length, cksum)
    cksum = compute_checksum(pseudo + udp_hdr + payload)
    if cksum == 0:
        cksum = 0xffff
    udp_hdr = struct.pack('>HHHH', src_port, dst_port, length, cksum)
    return udp_hdr + payload

def build_tcp_packet(src_ip: str, dst_ip: str, src_port: int, dst_port: int, seq: int, ack: int,
                     flags: int, window: int, payload: bytes = b'', urg_ptr: int = 0) -> bytes:
    data_offset = (5 << 4) # 20 bytes
    tcp_hdr_no_cksum = struct.pack('>HHIIBBHHH', src_port, dst_port, seq, ack, data_offset, flags, window, 0, urg_ptr)
    pseudo = ip_to_bytes(src_ip) + ip_to_bytes(dst_ip) + struct.pack('>BBH', 0, 6, 20 + len(payload))
    cksum = compute_checksum(pseudo + tcp_hdr_no_cksum + payload)
    tcp_hdr = struct.pack('>HHIIBBHHH', src_port, dst_port, seq, ack, data_offset, flags, window, cksum, urg_ptr)
    return tcp_hdr + payload

def build_dns_query(tx_id: int, domain: str) -> bytes:
    # Flags: standard query (0x0100)
    flags = 0x0100
    qdcount = 1
    ancount = 0
    nscount = 0
    arcount = 0
    hdr = struct.pack('>HHHHHH', tx_id, flags, qdcount, ancount, nscount, arcount)
    qname = b''
    for part in domain.split('.'):
        qname += bytes([len(part)]) + part.encode('ascii')
    qname += b'\x00'
    qtype = 1 # Type A
    qclass = 1 # Class IN
    return hdr + qname + struct.pack('>HH', qtype, qclass)

def build_dns_response(tx_id: int, domain: str, answer_ip: str) -> bytes:
    flags = 0x8180 # Standard query response, No error
    qdcount = 1
    ancount = 1
    nscount = 0
    arcount = 0
    hdr = struct.pack('>HHHHHH', tx_id, flags, qdcount, ancount, nscount, arcount)
    qname = b''
    for part in domain.split('.'):
        qname += bytes([len(part)]) + part.encode('ascii')
    qname += b'\x00'
    question = qname + struct.pack('>HH', 1, 1)
    
    # Answer: pointer to domain name (0xc00c)
    ans_name = b'\xc0\x0c'
    ans_type = 1
    ans_class = 1
    ans_ttl = 300
    ans_rdlen = 4
    ans_rdata = ip_to_bytes(answer_ip)
    answer = ans_name + struct.pack('>HHIH', ans_type, ans_class, ans_ttl, ans_rdlen) + ans_rdata
    return hdr + question + answer

def generate_assignment5_pcap(filepath: str):
    pcap = PcapWriter(filepath)

    # Base configuration matching user environment
    HOST_MAC = "dc:4b:a1:1f:76:7c"     # Intel Corporate OUI dc:4b:a1
    ROUTER_MAC = "f4:ca:e7:ba:a3:e4"   # Gateway OUI f4:ca:e7
    BROADCAST_MAC = "ff:ff:ff:ff:ff:ff"
    
    HOST_IP = "192.168.29.118"
    ROUTER_IP = "192.168.29.1"
    DNS_IP = "192.168.29.1"
    SERVER_IP = "128.119.245.12"      # gaia.cs.umass.edu
    
    base_epoch = 1793081112 # 2026-10-27 10:45:10 UTC / local
    
    # helper to write packet with offset in seconds
    def add_pkt(pkt_bytes: bytes, delta_sec: float):
        sec = base_epoch + int(delta_sec)
        usec = int((delta_sec - int(delta_sec)) * 1_000_000)
        pcap.write_packet(pkt_bytes, sec, usec)

    # 1. ARP Request (Who has 192.168.29.1? Tell 192.168.29.118)
    arp_req = build_arp_packet(1, HOST_MAC, HOST_IP, "00:00:00:00:00:00", ROUTER_IP)
    frame1 = build_ethernet_frame(BROADCAST_MAC, HOST_MAC, 0x0806, arp_req)
    add_pkt(frame1, 0.000000)

    # 2. ARP Reply (192.168.29.1 is at f4:ca:e7:ba:a3:e4)
    arp_rep = build_arp_packet(2, ROUTER_MAC, ROUTER_IP, HOST_MAC, HOST_IP)
    frame2 = build_ethernet_frame(HOST_MAC, ROUTER_MAC, 0x0806, arp_rep)
    add_pkt(frame2, 0.001420)

    # 3. ICMP Echo Request (Ping 1)
    ping_payload = b"abcdefghijklmnopqrstuvwabcdefghi" # 32 bytes standard ping payload
    icmp_req1 = build_icmp_echo(8, 0, 0x1a2b, 1, ping_payload)
    ip_ping1 = build_ipv4_packet(HOST_IP, ROUTER_IP, 1, icmp_req1, ident=0x3a01)
    frame3 = build_ethernet_frame(ROUTER_MAC, HOST_MAC, 0x0800, ip_ping1)
    add_pkt(frame3, 0.002150)

    # 4. ICMP Echo Reply (Ping 1)
    icmp_rep1 = build_icmp_echo(0, 0, 0x1a2b, 1, ping_payload)
    ip_prep1 = build_ipv4_packet(ROUTER_IP, HOST_IP, 1, icmp_rep1, ident=0x0000, ttl=64)
    frame4 = build_ethernet_frame(HOST_MAC, ROUTER_MAC, 0x0800, ip_prep1)
    add_pkt(frame4, 0.003980)

    # 5. ICMP Echo Request (Ping 2)
    icmp_req2 = build_icmp_echo(8, 0, 0x1a2b, 2, ping_payload)
    ip_ping2 = build_ipv4_packet(HOST_IP, ROUTER_IP, 1, icmp_req2, ident=0x3a02)
    frame5 = build_ethernet_frame(ROUTER_MAC, HOST_MAC, 0x0800, ip_ping2)
    add_pkt(frame5, 1.004120)

    # 6. ICMP Echo Reply (Ping 2)
    icmp_rep2 = build_icmp_echo(0, 0, 0x1a2b, 2, ping_payload)
    ip_prep2 = build_ipv4_packet(ROUTER_IP, HOST_IP, 1, icmp_rep2, ident=0x0000, ttl=64)
    frame6 = build_ethernet_frame(HOST_MAC, ROUTER_MAC, 0x0800, ip_prep2)
    add_pkt(frame6, 1.005890)

    # 7. DNS Query: gaia.cs.umass.edu
    dns_q = build_dns_query(0x4a12, "gaia.cs.umass.edu")
    udp_dns_q = build_udp_packet(HOST_IP, DNS_IP, 54123, 53, dns_q)
    ip_dns_q = build_ipv4_packet(HOST_IP, DNS_IP, 17, udp_dns_q, ident=0x3b01)
    frame7 = build_ethernet_frame(ROUTER_MAC, HOST_MAC, 0x0800, ip_dns_q)
    add_pkt(frame7, 2.105400)

    # 8. DNS Response: gaia.cs.umass.edu -> 128.119.245.12
    dns_r = build_dns_response(0x4a12, "gaia.cs.umass.edu", SERVER_IP)
    udp_dns_r = build_udp_packet(DNS_IP, HOST_IP, 53, 54123, dns_r)
    ip_dns_r = build_ipv4_packet(DNS_IP, HOST_IP, 17, udp_dns_r, ident=0x8c12, ttl=60)
    frame8 = build_ethernet_frame(HOST_MAC, ROUTER_MAC, 0x0800, ip_dns_r)
    add_pkt(frame8, 2.128750)

    # 9. TCP 3-Way Handshake: SYN (Client -> Server)
    # Flags: SYN = 0x02
    tcp_syn = build_tcp_packet(HOST_IP, SERVER_IP, 49152, 80, seq=100000, ack=0, flags=0x02, window=64240)
    ip_tcp_syn = build_ipv4_packet(HOST_IP, SERVER_IP, 6, tcp_syn, ident=0x4c01)
    frame9 = build_ethernet_frame(ROUTER_MAC, HOST_MAC, 0x0800, ip_tcp_syn)
    add_pkt(frame9, 2.135200)

    # 10. TCP 3-Way Handshake: SYN-ACK (Server -> Client)
    # Flags: SYN|ACK = 0x12
    tcp_synack = build_tcp_packet(SERVER_IP, HOST_IP, 80, 49152, seq=200000, ack=100001, flags=0x12, window=65535)
    ip_tcp_synack = build_ipv4_packet(SERVER_IP, HOST_IP, 6, tcp_synack, ident=0x9001, ttl=52)
    frame10 = build_ethernet_frame(HOST_MAC, ROUTER_MAC, 0x0800, ip_tcp_synack)
    add_pkt(frame10, 2.215430)

    # 11. TCP 3-Way Handshake: ACK (Client -> Server)
    # Flags: ACK = 0x10
    tcp_ack = build_tcp_packet(HOST_IP, SERVER_IP, 49152, 80, seq=100001, ack=200001, flags=0x10, window=64240)
    ip_tcp_ack = build_ipv4_packet(HOST_IP, SERVER_IP, 6, tcp_ack, ident=0x4c02)
    frame11 = build_ethernet_frame(ROUTER_MAC, HOST_MAC, 0x0800, ip_tcp_ack)
    add_pkt(frame11, 2.215580)

    # 12. HTTP GET Request
    http_req_text = (
        "GET /wireshark-labs/HTTP-wireshark-file1.html HTTP/1.1\r\n"
        "Host: gaia.cs.umass.edu\r\n"
        "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/119.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Accept-Language: en-US,en;q=0.5\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Connection: keep-alive\r\n"
        "Upgrade-Insecure-Requests: 1\r\n\r\n"
    ).encode('ascii')
    
    tcp_http_get = build_tcp_packet(HOST_IP, SERVER_IP, 49152, 80, seq=100001, ack=200001, flags=0x18, window=64240, payload=http_req_text)
    ip_http_get = build_ipv4_packet(HOST_IP, SERVER_IP, 6, tcp_http_get, ident=0x4c03)
    frame12 = build_ethernet_frame(ROUTER_MAC, HOST_MAC, 0x0800, ip_http_get)
    # HTTP GET sent at t = 2.216300
    add_pkt(frame12, 2.216300)

    # 13. TCP ACK from Server for GET
    tcp_srv_ack = build_tcp_packet(SERVER_IP, HOST_IP, 80, 49152, seq=200001, ack=100001 + len(http_req_text), flags=0x10, window=65535)
    ip_srv_ack = build_ipv4_packet(SERVER_IP, HOST_IP, 6, tcp_srv_ack, ident=0x9002, ttl=52)
    frame13 = build_ethernet_frame(HOST_MAC, ROUTER_MAC, 0x0800, ip_srv_ack)
    add_pkt(frame13, 2.296540)

    # 14. HTTP 200 OK Response
    http_body = "<html>\nCongratulations! You have downloaded the file HTTP-wireshark-file1.html!\n</html>\n"
    http_resp_text = (
        "HTTP/1.1 200 OK\r\n"
        "Date: Fri, 27 Oct 2026 05:15:12 GMT\r\n"
        "Server: Apache/2.4.52 (Ubuntu)\r\n"
        "Last-Modified: Thu, 21 Apr 2026 14:28:02 GMT\r\n"
        "ETag: \"80-5dc3a4f8d5e00\"\r\n"
        "Accept-Ranges: bytes\r\n"
        f"Content-Length: {len(http_body)}\r\n"
        "Keep-Alive: timeout=5, max=100\r\n"
        "Connection: Keep-Alive\r\n"
        "Content-Type: text/html\r\n\r\n" + http_body
    ).encode('ascii')
    
    tcp_http_resp = build_tcp_packet(SERVER_IP, HOST_IP, 80, 49152, seq=200001, ack=100001 + len(http_req_text), flags=0x18, window=65535, payload=http_resp_text)
    ip_http_resp = build_ipv4_packet(SERVER_IP, HOST_IP, 6, tcp_http_resp, ident=0x9003, ttl=52)
    frame14 = build_ethernet_frame(HOST_MAC, ROUTER_MAC, 0x0800, ip_http_resp)
    # HTTP OK received at t = 2.312840 (duration = 2.312840 - 2.216300 = 0.096540 s = 96.54 ms)
    add_pkt(frame14, 2.312840)

    # 15. TCP Client ACK for HTTP 200 OK
    tcp_cli_ack = build_tcp_packet(HOST_IP, SERVER_IP, 49152, 80, seq=100001 + len(http_req_text), ack=200001 + len(http_resp_text), flags=0x10, window=64240)
    ip_cli_ack = build_ipv4_packet(HOST_IP, SERVER_IP, 6, tcp_cli_ack, ident=0x4c04)
    frame15 = build_ethernet_frame(ROUTER_MAC, HOST_MAC, 0x0800, ip_cli_ack)
    add_pkt(frame15, 2.313020)

    # 16. TCP Teardown FIN-ACK from Client
    tcp_cli_fin = build_tcp_packet(HOST_IP, SERVER_IP, 49152, 80, seq=100001 + len(http_req_text), ack=200001 + len(http_resp_text), flags=0x11, window=64240)
    ip_cli_fin = build_ipv4_packet(HOST_IP, SERVER_IP, 6, tcp_cli_fin, ident=0x4c05)
    frame16 = build_ethernet_frame(ROUTER_MAC, HOST_MAC, 0x0800, ip_cli_fin)
    add_pkt(frame16, 3.514200)

    # 17. TCP Server ACK for FIN
    tcp_srv_ack_fin = build_tcp_packet(SERVER_IP, HOST_IP, 80, 49152, seq=200001 + len(http_resp_text), ack=100002 + len(http_req_text), flags=0x10, window=65535)
    ip_srv_ack_fin = build_ipv4_packet(SERVER_IP, HOST_IP, 6, tcp_srv_ack_fin, ident=0x9004, ttl=52)
    frame17 = build_ethernet_frame(HOST_MAC, ROUTER_MAC, 0x0800, ip_srv_ack_fin)
    add_pkt(frame17, 3.594320)

    # 18. TCP Server FIN-ACK
    tcp_srv_fin = build_tcp_packet(SERVER_IP, HOST_IP, 80, 49152, seq=200001 + len(http_resp_text), ack=100002 + len(http_req_text), flags=0x11, window=65535)
    ip_srv_fin = build_ipv4_packet(SERVER_IP, HOST_IP, 6, tcp_srv_fin, ident=0x9005, ttl=52)
    frame18 = build_ethernet_frame(HOST_MAC, ROUTER_MAC, 0x0800, ip_srv_fin)
    add_pkt(frame18, 3.594500)

    # 19. TCP Client ACK for Server FIN
    tcp_cli_ack2 = build_tcp_packet(HOST_IP, SERVER_IP, 49152, 80, seq=100002 + len(http_req_text), ack=200002 + len(http_resp_text), flags=0x10, window=64240)
    ip_cli_ack2 = build_ipv4_packet(HOST_IP, SERVER_IP, 6, tcp_cli_ack2, ident=0x4c06)
    frame19 = build_ethernet_frame(ROUTER_MAC, HOST_MAC, 0x0800, ip_cli_ack2)
    add_pkt(frame19, 3.594680)

    # Background packets to simulate realistic protocol hierarchy statistics
    # Add additional TCP packets (TLS / HTTPS exchanges, SSH keepalive)
    cur_t = 4.0
    for i in range(15):
        cur_t += 0.15
        # Client -> Remote HTTPS server
        pkt = build_tcp_packet(HOST_IP, "142.250.190.46", 52100 + i, 443, seq=1000*i, ack=2000*i, flags=0x10, window=64240, payload=b'HTTPS_PAYLOAD_SIM'*4)
        ip_p = build_ipv4_packet(HOST_IP, "142.250.190.46", 6, pkt, ident=0x5000+i)
        fr = build_ethernet_frame(ROUTER_MAC, HOST_MAC, 0x0800, ip_p)
        add_pkt(fr, cur_t)

    # Add background UDP packets (NTP, DNS queries, SSDP)
    for i in range(8):
        cur_t += 0.12
        if i % 2 == 0:
            # NTP
            ntp_data = b'\x1b' + b'\x00'*47
            udp_p = build_udp_packet(HOST_IP, "216.239.35.0", 123, 123, ntp_data)
            ip_p = build_ipv4_packet(HOST_IP, "216.239.35.0", 17, udp_p, ident=0x6000+i)
            fr = build_ethernet_frame(ROUTER_MAC, HOST_MAC, 0x0800, ip_p)
        else:
            # SSDP broadcast
            ssdp_data = b'M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: "ssdp:discover"\r\nMX: 1\r\nST: ssdp:all\r\n\r\n'
            udp_p = build_udp_packet(HOST_IP, "239.255.255.250", 41234, 1900, ssdp_data)
            ip_p = build_ipv4_packet(HOST_IP, "239.255.255.250", 17, udp_p, ident=0x7000+i)
            fr = build_ethernet_frame("01:00:5e:7f:ff:fa", HOST_MAC, 0x0800, ip_p)
        add_pkt(fr, cur_t)

    pcap.close()
    print(f"[+] Successfully generated capture file: {filepath}")

if __name__ == "__main__":
    import sys
    out_path = "network_capture.pcap"
    if len(sys.argv) > 1:
        out_path = sys.argv[1]
    else:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        out_path = os.path.join(script_dir, "network_capture.pcap")
    generate_assignment5_pcap(out_path)
