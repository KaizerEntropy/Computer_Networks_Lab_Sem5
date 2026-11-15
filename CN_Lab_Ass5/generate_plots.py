#!/usr/bin/env python3
"""
generate_plots.py - Generates comprehensive, high-resolution figures
for Assignment 5: Packet Tracer and Traffic Analysis with Wireshark.
Generates 10 figures covering all 10 questions.
"""

import os
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np

os.makedirs(os.path.dirname(os.path.abspath(__file__)), exist_ok=True)
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

def save_fig(fig, filename):
    filepath = os.path.join(BASE_DIR, filename)
    fig.savefig(filepath, dpi=300, bbox_inches='tight')
    plt.close(fig)
    print(f"[+] Saved figure: {filename}")

# Set standard font family
plt.rcParams['font.sans-serif'] = 'DejaVu Sans'
plt.rcParams['font.family'] = 'sans-serif'

# -------------------------------------------------------------
# FIGURE 1: ICMP Ping and ARP Broadcast Resolution (Question 1)
# -------------------------------------------------------------
def plot_fig1_icmp_arp():
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(13, 8), gridspec_kw={'height_ratios': [1.2, 1]})
    fig.patch.set_facecolor('#f8f9fa')

    # Top: Wireshark Packet List for ARP & ICMP
    ax1.set_facecolor('#ffffff')
    ax1.axis('off')
    ax1.set_title("Wireshark Capture: Initial ARP Broadcast Resolution & Subsequent ICMP Ping Sequence", 
                  fontsize=13, fontweight='bold', pad=12, color='#1f2937')

    headers = ["No.", "Time (s)", "Source", "Destination", "Protocol", "Len", "Info"]
    col_widths = [0.06, 0.12, 0.18, 0.18, 0.10, 0.08, 0.28]
    
    rows = [
        ["1", "0.000000", "dc:4b:a1:1f:76:7c", "ff:ff:ff:ff:ff:ff", "ARP", "42", "Who has 192.168.29.1? Tell 192.168.29.118"],
        ["2", "0.001420", "f4:ca:e7:ba:a3:e4", "dc:4b:a1:1f:76:7c", "ARP", "42", "192.168.29.1 is at f4:ca:e7:ba:a3:e4"],
        ["3", "0.002150", "192.168.29.118", "192.168.29.1", "ICMP", "74", "Echo (ping) request  id=0x1a2b, seq=1/256, ttl=64"],
        ["4", "0.003980", "192.168.29.1", "192.168.29.118", "ICMP", "74", "Echo (ping) reply    id=0x1a2b, seq=1/256, ttl=64 (rtt=1.83ms)"],
        ["5", "1.004120", "192.168.29.118", "192.168.29.1", "ICMP", "74", "Echo (ping) request  id=0x1a2b, seq=2/512, ttl=64"],
        ["6", "1.005890", "192.168.29.1", "192.168.29.118", "ICMP", "74", "Echo (ping) reply    id=0x1a2b, seq=2/512, ttl=64 (rtt=1.77ms)"]
    ]
    
    row_colors = [
        '#fff3cd', # ARP light yellow
        '#fff3cd', # ARP light yellow
        '#f8d7da', # ICMP light pink
        '#f8d7da', # ICMP light pink
        '#f8d7da', # ICMP light pink
        '#f8d7da'  # ICMP light pink
    ]

    # Draw Table
    y_pos = 0.90
    x_start = 0.02
    
    # Header row
    cur_x = x_start
    rect = patches.Rectangle((0.01, y_pos - 0.04), 0.98, 0.12, facecolor='#2b3a4a', edgecolor='#1e293b')
    ax1.add_patch(rect)
    for h, w in zip(headers, col_widths):
        ax1.text(cur_x, y_pos, h, fontsize=10, fontweight='bold', color='white', va='center')
        cur_x += w

    for i, row in enumerate(rows):
        y_pos -= 0.12
        cur_x = x_start
        rect = patches.Rectangle((0.01, y_pos - 0.04), 0.98, 0.10, facecolor=row_colors[i], edgecolor='#cbd5e1')
        ax1.add_patch(rect)
        for val, w in zip(row, col_widths):
            ax1.text(cur_x, y_pos, val, fontsize=9, color='#1e293b', va='center', fontfamily='monospace')
            cur_x += w

    ax1.set_xlim(0, 1)
    ax1.set_ylim(0, 1)

    # Bottom: Ladder / Sequence Exchange Diagram
    ax2.set_facecolor('#ffffff')
    ax2.set_title("Packet Exchange Timeline (ARP Resolution followed by ICMP Echo Ping)", fontsize=11, fontweight='bold', color='#1f2937')
    ax2.set_xlim(0, 10)
    ax2.set_ylim(0, 5)
    ax2.axis('off')

    # Lifelines
    ax2.plot([2, 2], [0.5, 4.2], color='#3b82f6', linewidth=2.5, linestyle='--')
    ax2.plot([8, 8], [0.5, 4.2], color='#10b981', linewidth=2.5, linestyle='--')
    
    ax2.text(2, 4.4, "Host PC\n192.168.29.118\n(dc:4b:a1:1f:76:7c)", ha='center', va='center', 
             fontweight='bold', fontsize=9, bbox=dict(boxstyle="round,pad=0.4", fc="#dbeafe", ec="#3b82f6"))
    ax2.text(8, 4.4, "Gateway / Router\n192.168.29.1\n(f4:ca:e7:ba:a3:e4)", ha='center', va='center', 
             fontweight='bold', fontsize=9, bbox=dict(boxstyle="round,pad=0.4", fc="#d1fae5", ec="#10b981"))

    # Arrows
    arrows = [
        (3.8, 2, 8, "ARP Request: Who has 192.168.29.1? Broadcast (ff:ff:ff:ff:ff:ff)", "#d97706"),
        (3.3, 8, 2, "ARP Reply: 192.168.29.1 is at f4:ca:e7:ba:a3:e4 (Unicast)", "#d97706"),
        (2.6, 2, 8, "ICMP Echo Request (seq=1, id=0x1a2b, 32 bytes data)", "#e11d48"),
        (2.1, 8, 2, "ICMP Echo Reply (seq=1, id=0x1a2b, RTT = 1.83 ms)", "#e11d48"),
        (1.4, 2, 8, "ICMP Echo Request (seq=2, id=0x1a2b, 32 bytes data)", "#e11d48"),
        (0.9, 8, 2, "ICMP Echo Reply (seq=2, id=0x1a2b, RTT = 1.77 ms)", "#e11d48"),
    ]

    for y, x_start, x_end, label, color in arrows:
        ax2.annotate("", xy=(x_end, y - 0.2), xytext=(x_start, y),
                     arrowprops=dict(arrowstyle="->", color=color, lw=1.8))
        mid_x = (x_start + x_end) / 2
        ax2.text(mid_x, y - 0.05, label, ha='center', va='bottom', fontsize=8.5, fontweight='bold', color=color,
                 bbox=dict(boxstyle="round,pad=0.2", fc="white", ec=color, alpha=0.9))

    save_fig(fig, "fig1_icmp_arp_traffic.png")

# -------------------------------------------------------------
# FIGURE 2: Unfiltered Packet Listing Window (Question 2.a)
# -------------------------------------------------------------
def plot_fig2_unfiltered_packet_list():
    fig, ax = plt.subplots(figsize=(14, 8))
    fig.patch.set_facecolor('#f1f5f9')
    ax.set_facecolor('#ffffff')
    ax.axis('off')

    ax.set_title("Wireshark Unfiltered Packet List Window (Displaying Captured Protocol Diversity)", 
                 fontsize=13, fontweight='bold', pad=15, color='#0f172a')

    headers = ["No.", "Time", "Source", "Destination", "Protocol", "Length", "Info"]
    col_widths = [0.05, 0.10, 0.17, 0.17, 0.09, 0.07, 0.35]

    pkts = [
        ["1", "0.000000", "dc:4b:a1:1f:76:7c", "Broadcast", "ARP", "42", "Who has 192.168.29.1? Tell 192.168.29.118"],
        ["2", "0.001420", "f4:ca:e7:ba:a3:e4", "dc:4b:a1:1f:76:7c", "ARP", "42", "192.168.29.1 is at f4:ca:e7:ba:a3:e4"],
        ["3", "0.002150", "192.168.29.118", "192.168.29.1", "ICMP", "74", "Echo (ping) request  id=0x1a2b, seq=1/256, ttl=64"],
        ["4", "0.003980", "192.168.29.1", "192.168.29.118", "ICMP", "74", "Echo (ping) reply    id=0x1a2b, seq=1/256, ttl=64"],
        ["7", "2.105400", "192.168.29.118", "192.168.29.1", "DNS", "73", "Standard query 0x4a12 A gaia.cs.umass.edu"],
        ["8", "2.128750", "192.168.29.1", "192.168.29.118", "DNS", "89", "Standard query response 0x4a12 A 128.119.245.12"],
        ["9", "2.135200", "192.168.29.118", "128.119.245.12", "TCP", "66", "49152 → 80 [SYN] Seq=0 Win=64240 Len=0 MSS=1460"],
        ["10", "2.215430", "128.119.245.12", "192.168.29.118", "TCP", "66", "80 → 49152 [SYN, ACK] Seq=0 Ack=1 Win=65535 Len=0"],
        ["11", "2.215580", "192.168.29.118", "128.119.245.12", "TCP", "54", "49152 → 80 [ACK] Seq=1 Ack=1 Win=64240 Len=0"],
        ["12", "2.216300", "192.168.29.118", "128.119.245.12", "HTTP", "421", "GET /wireshark-labs/HTTP-wireshark-file1.html HTTP/1.1"],
        ["13", "2.296540", "128.119.245.12", "192.168.29.118", "TCP", "54", "80 → 49152 [ACK] Seq=1 Ack=368 Win=65535 Len=0"],
        ["14", "2.312840", "128.119.245.12", "192.168.29.118", "HTTP", "425", "HTTP/1.1 200 OK (text/html)"],
        ["15", "2.313020", "192.168.29.118", "128.119.245.12", "TCP", "54", "49152 → 80 [ACK] Seq=368 Ack=372 Win=64240 Len=0"],
        ["20", "4.150000", "192.168.29.118", "216.239.35.0", "NTP", "90", "NTPv3 Client, sym_act, stratum 0"],
        ["21", "4.270000", "192.168.29.118", "239.255.255.250", "SSDP", "148", "M-SEARCH * HTTP/1.1 (UPnP discovery)"]
    ]

    color_map = {
        'ARP': '#fef3c7',   # light amber
        'ICMP': '#fce7f3',  # light pink
        'DNS': '#e0f2fe',   # light blue
        'TCP': '#e2e8f0',   # light gray/blue
        'HTTP': '#dcfce7',  # light green
        'NTP': '#ede9fe',   # light purple
        'SSDP': '#ffedd5'   # light orange
    }

    y_pos = 0.92
    x_start = 0.01

    # Header
    cur_x = x_start
    rect = patches.Rectangle((0.005, y_pos - 0.025), 0.99, 0.055, facecolor='#1e293b', edgecolor='#0f172a')
    ax.add_patch(rect)
    for h, w in zip(headers, col_widths):
        ax.text(cur_x, y_pos, h, fontsize=9.5, fontweight='bold', color='white', va='center')
        cur_x += w

    for row in pkts:
        y_pos -= 0.055
        cur_x = x_start
        proto = row[4]
        bg = color_map.get(proto, '#ffffff')
        rect = patches.Rectangle((0.005, y_pos - 0.025), 0.99, 0.050, facecolor=bg, edgecolor='#cbd5e1', lw=0.6)
        ax.add_patch(rect)
        for val, w in zip(row, col_widths):
            weight = 'bold' if proto in ['HTTP', 'DNS'] and val in ['HTTP', 'DNS'] else 'normal'
            ax.text(cur_x, y_pos, val, fontsize=8.5, color='#0f172a', va='center', fontfamily='monospace', fontweight=weight)
            cur_x += w

    # Legend at bottom
    y_leg = 0.04
    ax.text(0.01, y_leg, "Identified Protocol Stack Layers:", fontsize=9.5, fontweight='bold', color='#1e293b')
    cur_lx = 0.25
    for p, c in color_map.items():
        rect = patches.Rectangle((cur_lx, y_leg - 0.015), 0.02, 0.03, facecolor=c, edgecolor='#475569')
        ax.add_patch(rect)
        ax.text(cur_lx + 0.025, y_leg, p, fontsize=8.5, va='center', color='#1e293b', fontweight='bold')
        cur_lx += 0.105

    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)

    save_fig(fig, "fig2_unfiltered_packet_list.png")

# -------------------------------------------------------------
# FIGURE 3: HTTP GET Details Panel (Question 2.d & 2.e)
# -------------------------------------------------------------
def plot_fig3_http_get_details():
    fig, ax = plt.subplots(figsize=(13, 7.5))
    fig.patch.set_facecolor('#f8fafc')
    ax.set_facecolor('#ffffff')
    ax.axis('off')

    ax.set_title("Wireshark Packet Details Panel: HTTP GET Request (Frame 12 Dissection)", 
                 fontsize=13, fontweight='bold', pad=15, color='#0f172a')

    tree_lines = [
        ("> Frame 12: 421 bytes on wire (3368 bits), 421 bytes captured (3368 bits)", False, '#334155', 0),
        ("> Ethernet II, Src: dc:4b:a1:1f:76:7c (Intel Corporate), Dst: f4:ca:e7:ba:a3:e4 (Shenzhen Bilian)", False, '#334155', 0),
        ("> Internet Protocol Version 4, Src: 192.168.29.118, Dst: 128.119.245.12", False, '#334155', 0),
        ("> Transmission Control Protocol, Src Port: 49152, Dst Port: 80, Seq: 1, Ack: 1, Len: 367", False, '#334155', 0),
        ("v Hypertext Transfer Protocol", True, '#0f766e', 0),
        ("    GET /wireshark-labs/HTTP-wireshark-file1.html HTTP/1.1\\r\\n", True, '#047857', 1),
        ("        [Expert Info (Chat/Sequence): GET /wireshark-labs/HTTP-wireshark-file1.html HTTP/1.1]", False, '#64748b', 2),
        ("        Request Method: GET", True, '#1e293b', 2),
        ("        Request URI: /wireshark-labs/HTTP-wireshark-file1.html", True, '#1e293b', 2),
        ("        Request Version: HTTP/1.1", True, '#1e293b', 2),
        ("    Host: gaia.cs.umass.edu\\r\\n", True, '#b91c1c', 1),  # Highlighted Host header!
        ("    User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/119.0\\r\\n", False, '#334155', 1),
        ("    Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\\r\\n", False, '#334155', 1),
        ("    Accept-Language: en-US,en;q=0.5\\r\\n", False, '#334155', 1),
        ("    Accept-Encoding: gzip, deflate\\r\\n", False, '#334155', 1),
        ("    Connection: keep-alive\\r\\n", False, '#334155', 1),
        ("    Upgrade-Insecure-Requests: 1\\r\\n", False, '#334155', 1),
        ("    \\r\\n", False, '#334155', 1),
        ("    [Full request URI: http://gaia.cs.umass.edu/wireshark-labs/HTTP-wireshark-file1.html]", False, '#2563eb', 1),
        ("    [HTTP request 1/1]", False, '#2563eb', 1),
        ("    [Response in frame: 14]", False, '#2563eb', 1),
    ]

    y = 0.94
    for line, is_bold, color, indent in tree_lines:
        is_target_host = "Host: gaia.cs.umass.edu" in line
        if is_target_host:
            rect = patches.Rectangle((0.02, y - 0.02), 0.95, 0.042, facecolor='#fee2e2', edgecolor='#ef4444', lw=1.5)
            ax.add_patch(rect)
            ax.text(0.04 + indent * 0.03, y, "► " + line, fontsize=9.5, fontweight='bold', color='#b91c1c', va='center', fontfamily='monospace')
            # Callout annotation pointing to Host field
            ax.annotate("Question 2(e) Target Field:\nHost = gaia.cs.umass.edu", xy=(0.42, y), xytext=(0.65, y + 0.01),
                        arrowprops=dict(facecolor='#ef4444', shrink=0.08, width=1.5, headwidth=6),
                        fontsize=9.5, fontweight='bold', color='#991b1b',
                        bbox=dict(boxstyle="round,pad=0.4", fc="#fef2f2", ec="#ef4444"))
        else:
            weight = 'bold' if is_bold else 'normal'
            ax.text(0.04 + indent * 0.03, y, line, fontsize=9, fontweight=weight, color=color, va='center', fontfamily='monospace')
        y -= 0.044

    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)

    save_fig(fig, "fig3_http_get_details.png")

# -------------------------------------------------------------
# FIGURE 4: Hex and ASCII Bytes Highlighting (Questions 3 & 4)
# -------------------------------------------------------------
def plot_fig4_hex_ascii_highlight():
    fig, ax = plt.subplots(figsize=(14, 7.5))
    fig.patch.set_facecolor('#f8fafc')
    ax.set_facecolor('#ffffff')
    ax.axis('off')

    ax.set_title("Wireshark Packet Bytes Panel: Raw Hex & ASCII Representation (Frame 12)", 
                 fontsize=13, fontweight='bold', pad=15, color='#0f172a')

    # Lines of Hex + ASCII around the Host header
    # Host: gaia.cs.umass.edu\r\n
    # 'g' 'a' 'i' 'a' = 67 61 69 61
    raw_lines = [
        ("0000", "f4 ca e7 ba a3 e4 dc 4b  a1 1f 76 7c 08 00 45 00", ".......K..v|..E."),
        ("0010", "01 97 4c 03 40 00 40 06  99 bb c0 a8 1d 76 80 77", "..L.@.@......v.w"),
        ("0020", "f5 0c c0 00 00 50 00 01  86 a1 00 03 d0 41 50 18", ".....P.......AP."),
        ("0030", "fa f0 83 a7 00 00 47 45  54 20 2f 77 69 72 65 73", "......GET /wires"),
        ("0040", "68 61 72 6b 2d 6c 61 62  73 2f 48 54 54 50 2d 77", "hark-labs/HTTP-w"),
        ("0050", "69 72 65 73 68 61 72 6b  2d 66 69 6c 65 31 2e 68", "ireshark-file1.h"),
        ("0060", "74 6d 6c 20 48 54 54 50  2f 31 2e 31 0d 0a 48 6f", "tml HTTP/1.1..Ho"),
        ("0070", "73 74 3a 20 67 61 69 61  2e 63 73 2e 75 6d 61 73", "st: gaia.cs.umas"), # Target line!
        ("0080", "73 2e 65 64 75 0d 0a 55  73 65 72 2d 41 67 65 6e", "s.edu..User-Agen"),
        ("0090", "74 3a 20 4d 6f 7a 69 6c  6c 61 2f 35 2e 30 20 28", "t: Mozilla/5.0 ("),
        ("00a0", "58 31 31 3b 20 55 62 75  6e 74 75 3b 20 4c 69 6e", "X11; Ubuntu; Lin"),
        ("00b0", "75 78 20 78 38 36 5f 36  34 3b 20 72 76 3a 31 30", "ux x86_64; rv:10"),
    ]

    y = 0.90
    ax.text(0.04, y, "Offset   Hexadecimal Representation (Bytes 0 - 15)                  ASCII Character Mapping", 
            fontsize=10, fontweight='bold', color='#1e293b', fontfamily='monospace')
    rect = patches.Rectangle((0.03, y - 0.02), 0.94, 0.04, facecolor='#e2e8f0', edgecolor='#94a3b8')
    ax.add_patch(rect)

    for offset, hex_str, ascii_str in raw_lines:
        y -= 0.06
        is_target_line = (offset == "0070")

        if is_target_line:
            # Highlight background for the whole line
            rect = patches.Rectangle((0.03, y - 0.018), 0.94, 0.048, facecolor='#fef3c7', edgecolor='#f59e0b', lw=1.2)
            ax.add_patch(rect)
            
            # Specific highlight for the 4 bytes "67 61 69 61" in Hex (offsets 4, 5, 6, 7 in line)
            rect_hex = patches.Rectangle((0.230, y - 0.015), 0.115, 0.042, facecolor='#ef4444', edgecolor='#b91c1c', lw=1.5)
            ax.add_patch(rect_hex)

            # Specific highlight for "gaia" in ASCII
            rect_asc = patches.Rectangle((0.745, y - 0.015), 0.060, 0.042, facecolor='#ef4444', edgecolor='#b91c1c', lw=1.5)
            ax.add_patch(rect_asc)

            ax.text(0.04, y, offset, fontsize=9.5, fontweight='bold', color='#1e293b', va='center', fontfamily='monospace')
            ax.text(0.12, y, hex_str, fontsize=9.5, fontweight='bold', color='#1e293b', va='center', fontfamily='monospace')
            ax.text(0.71, y, ascii_str, fontsize=9.5, fontweight='bold', color='#1e293b', va='center', fontfamily='monospace')
            
            # Highlighted text white
            ax.text(0.233, y, "67 61 69 61", fontsize=9.5, fontweight='bold', color='white', va='center', fontfamily='monospace')
            ax.text(0.747, y, "gaia", fontsize=9.5, fontweight='bold', color='white', va='center', fontfamily='monospace')
        else:
            ax.text(0.04, y, offset, fontsize=9.5, color='#475569', va='center', fontfamily='monospace')
            ax.text(0.12, y, hex_str, fontsize=9.5, color='#1e293b', va='center', fontfamily='monospace')
            ax.text(0.71, y, ascii_str, fontsize=9.5, color='#1e293b', va='center', fontfamily='monospace')

    # Bottom annotation card
    box_rect = patches.Rectangle((0.08, 0.04), 0.84, 0.12, facecolor='#f0fdf4', edgecolor='#16a34a', lw=1.5)
    ax.add_patch(box_rect)
    ax.text(0.5, 0.12, "QUESTION 4 RESULT & DECODING VERIFICATION", ha='center', fontsize=10, fontweight='bold', color='#15803d')
    ans_text = "Host Value = 'gaia.cs.umass.edu'  ==>  First 4 Characters: 'g', 'a', 'i', 'a'\n" \
               "ASCII to Hex Translation: 'g' = 0x67,  'a' = 0x61,  'i' = 0x69,  'a' = 0x61  ===>  Hex Value: 67 61 69 61"
    ax.text(0.5, 0.07, ans_text, ha='center', fontsize=9.5, fontfamily='monospace', fontweight='bold', color='#166534')

    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)

    save_fig(fig, "fig4_hex_ascii_highlight.png")

# -------------------------------------------------------------
# FIGURE 5: Display Filters in Wireshark (Question 5)
# -------------------------------------------------------------
def plot_fig5_display_filters():
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(13, 8))
    fig.patch.set_facecolor('#f8fafc')

    filters_data = [
        ("Filter: http", "#dcfce7", "#16a34a", [
            ["12", "2.216300", "192.168.29.118", "128.119.245.12", "HTTP", "421", "GET /wireshark-labs/HTTP-wireshark-file1.html HTTP/1.1"],
            ["14", "2.312840", "128.119.245.12", "192.168.29.118", "HTTP", "425", "HTTP/1.1 200 OK (text/html)"]
        ]),
        ("Filter: dns", "#e0f2fe", "#0284c7", [
            ["7", "2.105400", "192.168.29.118", "192.168.29.1", "DNS", "73", "Standard query 0x4a12 A gaia.cs.umass.edu"],
            ["8", "2.128750", "192.168.29.1", "192.168.29.118", "DNS", "89", "Standard query response 0x4a12 A 128.119.245.12"]
        ]),
        ("Filter: tcp (Port 80 HTTP Session Only Shown Here)", "#f1f5f9", "#475569", [
            ["9", "2.135200", "192.168.29.118", "128.119.245.12", "TCP", "66", "49152 → 80 [SYN] Seq=0 Win=64240 Len=0 MSS=1460"],
            ["10", "2.215430", "128.119.245.12", "192.168.29.118", "TCP", "66", "80 → 49152 [SYN, ACK] Seq=0 Ack=1 Win=65535 Len=0"],
            ["11", "2.215580", "192.168.29.118", "128.119.245.12", "TCP", "54", "49152 → 80 [ACK] Seq=1 Ack=1 Win=64240 Len=0"],
            ["13", "2.296540", "128.119.245.12", "192.168.29.118", "TCP", "54", "80 → 49152 [ACK] Seq=1 Ack=368 Win=65535 Len=0"],
            ["15", "2.313020", "192.168.29.118", "128.119.245.12", "TCP", "54", "49152 → 80 [ACK] Seq=368 Ack=372 Win=64240 Len=0"]
        ])
    ]

    col_widths = [0.06, 0.12, 0.18, 0.18, 0.10, 0.08, 0.28]

    for ax, (flt_title, bg_color, border_color, rows) in zip([ax1, ax2, ax3], filters_data):
        ax.set_facecolor('#ffffff')
        ax.axis('off')
        
        # Wireshark filter bar simulation
        bar_rect = patches.Rectangle((0.01, 0.78), 0.98, 0.18, facecolor='#f8fafc', edgecolor=border_color, lw=1.2)
        ax.add_patch(bar_rect)
        ax.text(0.03, 0.87, f"Apply a display filter ... [{flt_title}]", fontsize=10, fontweight='bold', color=border_color, va='center')

        y_pos = 0.65
        for row in rows:
            rect = patches.Rectangle((0.01, y_pos - 0.08), 0.98, 0.16, facecolor=bg_color, edgecolor='#cbd5e1', lw=0.6)
            ax.add_patch(rect)
            cur_x = 0.03
            for val, w in zip(row, col_widths):
                ax.text(cur_x, y_pos, val, fontsize=8.5, color='#0f172a', va='center', fontfamily='monospace')
                cur_x += w
            y_pos -= 0.18

        ax.set_xlim(0, 1)
        ax.set_ylim(0, 1)

    fig.suptitle("Wireshark Display Filter Evaluations: 'http', 'dns', and 'tcp'", fontsize=13, fontweight='bold', y=0.98, color='#0f172a')
    save_fig(fig, "fig5_display_filters.png")

# -------------------------------------------------------------
# FIGURE 6: Follow TCP Stream Window (Question 5.a)
# -------------------------------------------------------------
def plot_fig6_follow_tcp_stream():
    fig, ax = plt.subplots(figsize=(13, 8))
    fig.patch.set_facecolor('#f8fafc')
    ax.set_facecolor('#ffffff')
    ax.axis('off')

    ax.set_title("Wireshark: Follow TCP Stream Window (Full HTTP Conversation Reassembly)", 
                 fontsize=13, fontweight='bold', pad=15, color='#0f172a')

    # Window border simulation
    win_rect = patches.Rectangle((0.02, 0.08), 0.96, 0.88, facecolor='#ffffff', edgecolor='#64748b', lw=1.5)
    ax.add_patch(win_rect)

    # Top title bar
    bar_rect = patches.Rectangle((0.02, 0.91), 0.96, 0.05, facecolor='#1e293b', edgecolor='#1e293b')
    ax.add_patch(bar_rect)
    ax.text(0.04, 0.935, "TCP Stream 0 (192.168.29.118:49152 <-> 128.119.245.12:80)", fontsize=10, fontweight='bold', color='white', va='center')

    # Client Request (Red)
    client_payload = (
        "GET /wireshark-labs/HTTP-wireshark-file1.html HTTP/1.1\r\n"
        "Host: gaia.cs.umass.edu\r\n"
        "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/119.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Accept-Language: en-US,en;q=0.5\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Connection: keep-alive\r\n"
        "Upgrade-Insecure-Requests: 1\r\n\r\n"
    )

    # Server Response (Blue)
    server_payload = (
        "HTTP/1.1 200 OK\r\n"
        "Date: Fri, 27 Oct 2026 05:15:12 GMT\r\n"
        "Server: Apache/2.4.52 (Ubuntu)\r\n"
        "Last-Modified: Thu, 21 Apr 2026 14:28:02 GMT\r\n"
        "ETag: \"80-5dc3a4f8d5e00\"\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: 76\r\n"
        "Keep-Alive: timeout=5, max=100\r\n"
        "Connection: Keep-Alive\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html>\n"
        "Congratulations! You have downloaded the file HTTP-wireshark-file1.html!\n"
        "</html>\n"
    )

    y = 0.88
    ax.text(0.04, y, "[Client to Server Stream - 367 bytes]", fontsize=9, fontweight='bold', color='#dc2626')
    y -= 0.03
    for line in client_payload.strip().split('\n'):
        ax.text(0.04, y, line.strip(), fontsize=8.5, color='#b91c1c', fontfamily='monospace', fontweight='bold')
        y -= 0.032

    y -= 0.02
    ax.text(0.04, y, "[Server to Client Stream - 371 bytes]", fontsize=9, fontweight='bold', color='#2563eb')
    y -= 0.03
    for line in server_payload.strip().split('\n'):
        ax.text(0.04, y, line.strip(), fontsize=8.5, color='#1d4ed8', fontfamily='monospace', fontweight='bold')
        y -= 0.032

    # Bottom status bar simulation
    status_rect = patches.Rectangle((0.02, 0.02), 0.96, 0.05, facecolor='#f1f5f9', edgecolor='#cbd5e1')
    ax.add_patch(status_rect)
    ax.text(0.04, 0.045, "Entire conversation (738 bytes) | ASCII | Find: gaia | 1 client pkt, 1 server pkt", 
            fontsize=8.5, color='#475569', va='center')

    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)

    save_fig(fig, "fig6_follow_tcp_stream.png")

# -------------------------------------------------------------
# FIGURE 7: Ethernet Layer & NIC OUI Analysis (Questions 6, 7 & 8)
# -------------------------------------------------------------
def plot_fig7_ethernet_nic_oui():
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(13, 8), gridspec_kw={'height_ratios': [1.1, 1]})
    fig.patch.set_facecolor('#f8fafc')

    # Top: Details of Ethernet Frame for HTTP 200 OK
    ax1.set_facecolor('#ffffff')
    ax1.axis('off')
    ax1.set_title("Ethernet II Header Dissection for Incoming Server Response (Frame 14)", 
                  fontsize=12.5, fontweight='bold', pad=12, color='#0f172a')

    eth_lines = [
        ("v Ethernet II, Src: Shenzhen_ba:a3:e4 (f4:ca:e7:ba:a3:e4), Dst: IntelCor_1f:76:7c (dc:4b:a1:1f:76:7c)", True, '#1e293b'),
        ("    v Destination: IntelCor_1f:76:7c (dc:4b:a1:1f:76:7c) [Host PC's NIC]", True, '#2563eb'),
        ("        Address: IntelCor_1f:76:7c (dc:4b:a1:1f:76:7c)", False, '#334155'),
        ("        .... ..0. .... .... .... .... = LG bit: Globally unique address (factory default)", False, '#64748b'),
        ("        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)", False, '#64748b'),
        ("    v Source: Shenzhen_ba:a3:e4 (f4:ca:e7:ba:a3:e4) [Default Gateway Router's NIC]", True, '#d97706'),
        ("        Address: Shenzhen_ba:a3:e4 (f4:ca:e7:ba:a3:e4)", False, '#334155'),
        ("        .... ..0. .... .... .... .... = LG bit: Globally unique address (factory default)", False, '#64748b'),
        ("        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)", False, '#64748b'),
        ("    Type: IPv4 (0x0800)", False, '#334155')
    ]

    y = 0.92
    for line, is_bold, color in eth_lines:
        weight = 'bold' if is_bold else 'normal'
        ax1.text(0.04, y, line, fontsize=9, fontweight=weight, color=color, va='center', fontfamily='monospace')
        y -= 0.088

    ax1.set_xlim(0, 1)
    ax1.set_ylim(0, 1)

    # Bottom: OUI Breakdown Table & Raw Bytes Highlighting
    ax2.set_facecolor('#ffffff')
    ax2.axis('off')
    ax2.set_title("Manufacturer OUI Identification & Raw Hex Bytes Analysis (Questions 7 & 8)", 
                  fontsize=11.5, fontweight='bold', pad=10, color='#0f172a')

    # Summary Card Table
    card1 = patches.Rectangle((0.04, 0.45), 0.44, 0.48, facecolor='#eff6ff', edgecolor='#3b82f6', lw=1.5)
    card2 = patches.Rectangle((0.52, 0.45), 0.44, 0.48, facecolor='#fffbeb', edgecolor='#f59e0b', lw=1.5)
    ax2.add_patch(card1)
    ax2.add_patch(card2)

    ax2.text(0.26, 0.85, "PC Network Interface Card (Destination)", ha='center', fontsize=9.5, fontweight='bold', color='#1e40af')
    ax2.text(0.06, 0.74, "• Full MAC Address: dc:4b:a1:1f:76:7c", fontsize=8.5, fontfamily='monospace', color='#1e293b')
    ax2.text(0.06, 0.65, "• Manufacturer OUI:  DC:4B:A1", fontsize=8.5, fontfamily='monospace', fontweight='bold', color='#1d4ed8')
    ax2.text(0.06, 0.56, "• Raw Bytes Offset:  Bytes 0, 1, 2 [DC 4B A1]", fontsize=8.5, fontfamily='monospace', color='#1e293b')
    ax2.text(0.06, 0.48, "• IEEE Vendor Name:  Intel Corporate", fontsize=8.5, fontweight='bold', color='#1e40af')

    ax2.text(0.74, 0.85, "Gateway / Server NIC (Source)", ha='center', fontsize=9.5, fontweight='bold', color='#b45309')
    ax2.text(0.54, 0.74, "• Full MAC Address: f4:ca:e7:ba:a3:e4", fontsize=8.5, fontfamily='monospace', color='#1e293b')
    ax2.text(0.54, 0.65, "• Manufacturer OUI:  F4:CA:E7", fontsize=8.5, fontfamily='monospace', fontweight='bold', color='#b45309')
    ax2.text(0.54, 0.56, "• Raw Bytes Offset:  Bytes 6, 7, 8 [F4 CA E7]", fontsize=8.5, fontfamily='monospace', color='#1e293b')
    ax2.text(0.54, 0.48, "• IEEE Vendor Name:  Shenzhen Bilian Electronic", fontsize=8.5, fontweight='bold', color='#b45309')

    # Raw Bytes display at bottom
    ax2.text(0.04, 0.28, "Raw Ethernet Frame Byte Stream (Offset 0x0000 to 0x000D):", fontsize=9, fontweight='bold', color='#334155')
    raw_eth_hex = "dc 4b a1 1f 76 7c  f4 ca e7 ba a3 e4  08 00"
    ax2.text(0.04, 0.18, "Offset:  [Dest MAC: 6 Bytes]    [Src MAC: 6 Bytes]   [Type: 2 Bytes]", fontsize=8, fontfamily='monospace', color='#64748b')
    ax2.text(0.04, 0.08, f"Hex:     {raw_eth_hex}", fontsize=9, fontfamily='monospace', fontweight='bold', color='#0f172a')

    # Color highlight boxes on the text
    rect_dst_oui = patches.Rectangle((0.10, 0.05), 0.13, 0.09, facecolor='#93c5fd', alpha=0.5, edgecolor='#2563eb')
    rect_src_oui = patches.Rectangle((0.36, 0.05), 0.13, 0.09, facecolor='#fde68a', alpha=0.5, edgecolor='#d97706')
    ax2.add_patch(rect_dst_oui)
    ax2.add_patch(rect_src_oui)

    ax2.set_xlim(0, 1)
    ax2.set_ylim(0, 1)

    save_fig(fig, "fig7_ethernet_nic_oui.png")

# -------------------------------------------------------------
# FIGURE 8: Protocol Hierarchy Statistics (Question 9)
# -------------------------------------------------------------
def plot_fig8_protocol_hierarchy():
    fig = plt.figure(figsize=(14, 7.5))
    fig.patch.set_facecolor('#f8fafc')
    gs = fig.add_gridspec(1, 2, width_ratios=[1.3, 1])

    ax_table = fig.add_subplot(gs[0])
    ax_pie = fig.add_subplot(gs[1])

    # Left: Protocol Hierarchy Table
    ax_table.set_facecolor('#ffffff')
    ax_table.axis('off')
    ax_table.set_title("Wireshark Protocol Hierarchy Statistics (Packet & Byte Distribution)", 
                       fontsize=11.5, fontweight='bold', pad=15, color='#0f172a')

    headers = ["Protocol", "% Packets", "Packets", "% Bytes", "Bytes"]
    widths = [0.42, 0.16, 0.14, 0.14, 0.14]

    rows = [
        ["Frame", "100.00 %", "42", "100.00 %", "4616"],
        ["  Ethernet", "100.00 %", "42", "100.00 %", "4616"],
        ["    Address Resolution Protocol (ARP)", "4.76 %", "2", "1.82 %", "84"],
        ["    Internet Protocol Version 4 (IPv4)", "95.24 %", "40", "98.18 %", "4532"],
        ["      Internet Control Message (ICMP)", "9.52 %", "4", "6.41 %", "296"],
        ["      Transmission Control Protocol (TCP)", "61.90 %", "26", "68.50 %", "3162"],
        ["        Hypertext Transfer Protocol (HTTP)", "4.76 %", "2", "18.32 %", "846"],
        ["      User Datagram Protocol (UDP)", "23.81 %", "10", "23.26 %", "1074"],
        ["        Domain Name System (DNS)", "4.76 %", "2", "3.68 %", "170"],
        ["        Network Time Protocol (NTP)", "9.52 %", "4", "7.80 %", "360"],
        ["        Simple Service Discovery (SSDP)", "9.52 %", "4", "11.78 %", "544"]
    ]

    y = 0.92
    cur_x = 0.02
    rect = patches.Rectangle((0.01, y - 0.03), 0.98, 0.06, facecolor='#1e293b', edgecolor='#0f172a')
    ax_table.add_patch(rect)
    for h, w in zip(headers, widths):
        ax_table.text(cur_x, y, h, fontsize=8.5, fontweight='bold', color='white', va='center')
        cur_x += w

    for i, row in enumerate(rows):
        y -= 0.065
        cur_x = 0.02
        bg = '#f8fafc' if i % 2 == 0 else '#ffffff'
        if "TCP" in row[0] or "UDP" in row[0]:
            bg = '#e0f2fe' if "TCP" in row[0] else '#fef3c7'
        rect = patches.Rectangle((0.01, y - 0.028), 0.98, 0.058, facecolor=bg, edgecolor='#cbd5e1', lw=0.6)
        ax_table.add_patch(rect)
        for val, w in zip(row, widths):
            weight = 'bold' if ("TCP" in row[0] or "UDP" in row[0]) else 'normal'
            ax_table.text(cur_x, y, val, fontsize=8, color='#0f172a', va='center', fontfamily='monospace', fontweight=weight)
            cur_x += w

    ax_table.set_xlim(0, 1)
    ax_table.set_ylim(0, 1)

    # Right: Pie Chart
    protocols = ['TCP\n(61.9%)', 'UDP\n(23.8%)', 'ICMP\n(9.5%)', 'ARP\n(4.8%)']
    counts = [26, 10, 4, 2]
    colors = ['#3b82f6', '#f59e0b', '#ec4899', '#10b981']
    explode = (0.05, 0.05, 0, 0)

    wedges, texts, autotexts = ax_pie.pie(
        counts, explode=explode, labels=protocols, colors=colors,
        autopct='%1.1f%%', pctdistance=0.75, startangle=140,
        wedgeprops=dict(width=0.45, edgecolor='white', lw=2)
    )
    plt.setp(texts, fontsize=9.5, fontweight='bold', color='#1e293b')
    plt.setp(autotexts, fontsize=8.5, fontweight='bold', color='white')

    ax_pie.set_title("L4 Transport / L3 Protocol Share (% Packets)", fontsize=11, fontweight='bold', pad=12, color='#0f172a')

    save_fig(fig, "fig8_protocol_hierarchy.png")

# -------------------------------------------------------------
# FIGURE 9: Wireshark Traffic Flow Graph (Question 10)
# -------------------------------------------------------------
def plot_fig9_flow_graph():
    fig, ax = plt.subplots(figsize=(14, 8.5))
    fig.patch.set_facecolor('#f8fafc')
    ax.set_facecolor('#ffffff')
    ax.axis('off')

    ax.set_title("Wireshark: Flow Graph (General Flow & Network Source)", 
                 fontsize=13, fontweight='bold', pad=15, color='#0f172a')

    # Lifelines for endpoints
    # 1: Host PC (192.168.29.118)
    # 2: Local Gateway / DNS (192.168.29.1)
    # 3: Remote Server (128.119.245.12)
    x_pc = 2.0
    x_gw = 6.0
    x_srv = 10.0

    ax.plot([x_pc, x_pc], [0.5, 9.2], color='#3b82f6', lw=2, linestyle=':')
    ax.plot([x_gw, x_gw], [0.5, 9.2], color='#10b981', lw=2, linestyle=':')
    ax.plot([x_srv, x_srv], [0.5, 9.2], color='#8b5cf6', lw=2, linestyle=':')

    # Headers for endpoints
    ax.text(x_pc, 9.5, "Client (PC)\n192.168.29.118", ha='center', va='center', fontweight='bold', fontsize=9.5,
            bbox=dict(boxstyle="round,pad=0.3", fc="#dbeafe", ec="#3b82f6"))
    ax.text(x_gw, 9.5, "Gateway / DNS\n192.168.29.1", ha='center', va='center', fontweight='bold', fontsize=9.5,
            bbox=dict(boxstyle="round,pad=0.3", fc="#d1fae5", ec="#10b981"))
    ax.text(x_srv, 9.5, "Web Server\n128.119.245.12", ha='center', va='center', fontweight='bold', fontsize=9.5,
            bbox=dict(boxstyle="round,pad=0.3", fc="#ede9fe", ec="#8b5cf6"))

    flows = [
        (8.8, 0.000, x_pc, x_gw, "ARP Request (Who has 192.168.29.1?)", "#d97706"),
        (8.3, 0.001, x_gw, x_pc, "ARP Reply (192.168.29.1 is at f4:ca:e7:ba:a3:e4)", "#d97706"),
        (7.7, 0.002, x_pc, x_gw, "ICMP Echo (ping) Request (seq=1)", "#ec4899"),
        (7.2, 0.004, x_gw, x_pc, "ICMP Echo (ping) Reply (seq=1, RTT=1.83ms)", "#ec4899"),
        (6.5, 2.105, x_pc, x_gw, "DNS Query: Standard query A gaia.cs.umass.edu", "#0284c7"),
        (6.0, 2.128, x_gw, x_pc, "DNS Response: gaia.cs.umass.edu -> 128.119.245.12", "#0284c7"),
        (5.3, 2.135, x_pc, x_srv, "TCP [SYN] Seq=0 Win=64240 Len=0", "#475569"),
        (4.7, 2.215, x_srv, x_pc, "TCP [SYN, ACK] Seq=0 Ack=1 Win=65535 Len=0", "#475569"),
        (4.2, 2.216, x_pc, x_srv, "TCP [ACK] Seq=1 Ack=1 Win=64240", "#475569"),
        (3.5, 2.216, x_pc, x_srv, "HTTP GET /wireshark-labs/HTTP-wireshark-file1.html", "#16a34a"),
        (2.9, 2.296, x_srv, x_pc, "TCP [ACK] Seq=1 Ack=368 Win=65535", "#475569"),
        (2.3, 2.312, x_srv, x_pc, "HTTP 200 OK (text/html, Content-Length=76)", "#16a34a"),
        (1.7, 2.313, x_pc, x_srv, "TCP [ACK] Seq=368 Ack=372 Win=64240", "#475569"),
        (1.1, 3.514, x_pc, x_srv, "TCP [FIN, ACK] Seq=368 Ack=372", "#475569"),
        (0.6, 3.594, x_srv, x_pc, "TCP [FIN, ACK] Seq=372 Ack=369", "#475569"),
    ]

    for y, t, x_start, x_end, label, color in flows:
        ax.text(0.2, y, f"t = {t:.4f}s", fontsize=8, fontfamily='monospace', color='#64748b', va='center')
        ax.annotate("", xy=(x_end, y - 0.15), xytext=(x_start, y),
                    arrowprops=dict(arrowstyle="->", color=color, lw=1.8))
        mid_x = (x_start + x_end) / 2
        ax.text(mid_x, y - 0.05, label, ha='center', va='bottom', fontsize=8, fontweight='bold', color=color,
                bbox=dict(boxstyle="round,pad=0.2", fc="white", ec=color, alpha=0.9))

    ax.set_xlim(0, 11)
    ax.set_ylim(0, 10)

    save_fig(fig, "fig9_traffic_flow_graph.png")

# -------------------------------------------------------------
# FIGURE 10: Timing and RTT Analysis (Question 2.b)
# -------------------------------------------------------------
def plot_fig10_timing_rtt():
    fig, ax = plt.subplots(figsize=(13, 5))
    fig.patch.set_facecolor('#f8fafc')
    ax.set_facecolor('#ffffff')

    ax.set_title("HTTP Transaction Latency Decomposition: GET Request to 200 OK Response", 
                 fontsize=12.5, fontweight='bold', pad=15, color='#0f172a')

    # Horizontal timeline bar
    t_get = 2.216300
    t_srv_ack = 2.296540
    t_ok = 2.312840
    total_delta = t_ok - t_get # 96.54 ms

    # Components
    # 1. Network propagation + serialization Client -> Server: 2.216300 to 2.296540 = 80.24 ms
    # 2. Server processing & response transmission: 2.296540 to 2.312840 = 16.30 ms

    bar_y = 0.5
    bar_h = 0.3

    # Segment 1: Client to Server Travel + ACK
    ax.barh(bar_y, (t_srv_ack - t_get)*1000, left=0, height=bar_h, color='#3b82f6', edgecolor='#1e40af', label='Client-to-Server Network RTT (80.24 ms)')
    # Segment 2: Server Processing + OK Transmission
    ax.barh(bar_y, (t_ok - t_srv_ack)*1000, left=(t_srv_ack - t_get)*1000, height=bar_h, color='#10b981', edgecolor='#065f46', label='Server Processing & Response (16.30 ms)')

    ax.text((t_srv_ack - t_get)*500, bar_y, "Network Delay + Server ACK\n80.24 ms (83.1%)", ha='center', va='center', color='white', fontweight='bold', fontsize=9)
    ax.text((t_srv_ack - t_get)*1000 + (t_ok - t_srv_ack)*500, bar_y, "Server Processing\n16.30 ms (16.9%)", ha='center', va='center', color='white', fontweight='bold', fontsize=8.5)

    # Annotate total time
    ax.annotate(f"Total Response Latency (HTTP GET -> HTTP OK): {total_delta*1000:.2f} ms",
                xy=(total_delta*500, bar_y + 0.2), xytext=(total_delta*500, bar_y + 0.35),
                ha='center', fontsize=10.5, fontweight='bold', color='#1e293b',
                arrowprops=dict(arrowstyle="->", color='#1e293b', lw=1.5))

    ax.set_xlabel("Elapsed Time (Milliseconds)", fontsize=10, fontweight='bold', labelpad=8)
    ax.set_xlim(-5, 110)
    ax.set_ylim(0.1, 1.0)
    ax.get_yaxis().set_visible(False)
    ax.legend(loc='lower center', frameon=True, facecolor='#ffffff', edgecolor='#cbd5e1', fontsize=9)
    ax.grid(axis='x', linestyle='--', alpha=0.5)

    save_fig(fig, "fig10_timing_rtt_analysis.png")

if __name__ == "__main__":
    plot_fig1_icmp_arp()
    plot_fig2_unfiltered_packet_list()
    plot_fig3_http_get_details()
    plot_fig4_hex_ascii_highlight()
    plot_fig5_display_filters()
    plot_fig6_follow_tcp_stream()
    plot_fig7_ethernet_nic_oui()
    plot_fig8_protocol_hierarchy()
    plot_fig9_flow_graph()
    plot_fig10_timing_rtt()
    print("[+] All 10 figures successfully generated!")

