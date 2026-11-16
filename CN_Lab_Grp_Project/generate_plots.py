#!/usr/bin/env python3
import json
import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

os.makedirs('plots', exist_ok=True)

# -----------------------------------------------------------------------------
# Figure 1: Sequence Diagram of Multi-Protocol Workflow (Clean, No Overlap)
# -----------------------------------------------------------------------------
fig, ax = plt.subplots(figsize=(13, 14), dpi=300)
ax.axis('off')
ax.set_xlim(0, 1)
ax.set_ylim(0, 1)

# Timelines (Lifelines)
x_cli = 0.20
x_srv = 0.80

# Lifeline vertical bars
ax.plot([x_cli, x_cli], [0.93, 0.03], color='#34495e', linewidth=2.5, linestyle='--')
ax.plot([x_srv, x_srv], [0.93, 0.03], color='#34495e', linewidth=2.5, linestyle='--')

# Header Boxes
ax.text(x_cli, 0.955, "CLIENT NODE\nMAC: 52:54:00:12:34:56\n(Port Dynamic / UDP & TCP)",
        ha='center', va='center', fontsize=10, fontweight='bold', color='#1a5276',
        bbox=dict(boxstyle='round,pad=0.5', facecolor='#d4e6f1', edgecolor='#2980b9', lw=2))

ax.text(x_srv, 0.955, "SERVER DAEMON\nIP: 192.168.10.1\n(UDP: 6767, 6769 | TCP: 2323)",
        ha='center', va='center', fontsize=10, fontweight='bold', color='#196f3d',
        bbox=dict(boxstyle='round,pad=0.5', facecolor='#d5f5e3', edgecolor='#27ae60', lw=2))

# Protocol Phase Regions (Background shading)
phases = [
    (0.915, 0.745, "#fef9e7", "#f39c12", "PHASE 1: BOOTP NETWORK BOOTSTRAP (RFC 951 / RFC 1048)"),
    (0.725, 0.435, "#f4ecf7", "#8e44ad", "PHASE 2: DHCP 4-WAY DORA LEASE ACQUISITION (RFC 2131 / RFC 2132)"),
    (0.415, 0.145, "#ebf5fb", "#2980b9", "PHASE 3: TELNET APPLICATION SESSION & ARQ DATA (RFC 854 / RFC 855)"),
    (0.125, 0.020, "#fdedec", "#e74c3c", "PHASE 4: DHCP LEASE TEARDOWN & RECLAMATION (RFC 2131 Sec 3.1.6)")
]

for y_top, y_bot, bg_col, edge_col, label in phases:
    rect = plt.Rectangle((0.04, y_bot), 0.92, y_top - y_bot,
                         facecolor=bg_col, edgecolor=edge_col, linestyle=':', alpha=0.55, lw=1.5, zorder=0)
    ax.add_patch(rect)
    ax.text(0.06, y_top - 0.02, label, fontsize=9.5, fontweight='bold', color=edge_col, va='top')

# Helper for drawing clean sequence arrows with bounded two-line text pills
def draw_seq(y_pos, line1, line2, color_code, l_to_r=True, tag=""):
    y_start = y_pos
    y_end = y_pos - 0.025
    x_s = x_cli if l_to_r else x_srv
    x_e = x_srv if l_to_r else x_cli

    ax.annotate("", xy=(x_e, y_end), xytext=(x_s, y_start),
                arrowprops=dict(arrowstyle="-|>", color=color_code, lw=2.2, mutation_scale=15),
                zorder=2)
    
    x_mid = (x_s + x_e) / 2.0
    y_mid = (y_start + y_end) / 2.0
    
    full_text = f"[{tag}]  {line1}\n{line2}"
    ax.text(x_mid, y_mid, full_text, ha='center', va='center',
            fontsize=8.5, fontweight='bold', color=color_code, zorder=3,
            bbox=dict(boxstyle='round,pad=0.35', facecolor='white', edgecolor=color_code, alpha=0.98, lw=1.3))

# 1. BOOTP
draw_seq(0.850, "BOOTREQUEST (UDP Port 6767)", "XID: 0xABCD1234 | Client MAC: 52:54:00:12:34:56", "#d35400", True, "BOOTP")
draw_seq(0.775, "BOOTREPLY (UDP Unicast)", "Assigns IP: 192.168.10.50 | Bootfile: pxelinux.0", "#27ae60", False, "BOOTP")

# 2. DHCP DORA
draw_seq(0.665, "DHCPDISCOVER (UDP Port 6769)", "Broadcast Datagram | Transaction ID: 0x5678ABCD", "#8e44ad", True, "DHCP")
draw_seq(0.595, "DHCPOFFER (UDP Unicast)", "Offered IP: 192.168.10.100 | Lease: 7200s (T1=3600s, T2=6300s)", "#2980b9", False, "DHCP")
draw_seq(0.525, "DHCPREQUEST (UDP Broadcast)", "Accepting Lease 192.168.10.100 | Server ID: 192.168.10.1", "#8e44ad", True, "DHCP")
draw_seq(0.455, "DHCPACK (UDP Unicast)", "Lease Committed | Netmask: 255.255.255.0, DefGW: 192.168.10.1", "#27ae60", False, "DHCP")

# 3. TELNET
draw_seq(0.360, "TCP 3-Way Handshake + IAC NVT Option Probes (Port 2323)", "IAC DO TTYPE | IAC WILL ECHO | IAC DO SGA", "#c0392b", True, "TELNET")
draw_seq(0.295, "IAC Option Acknowledgments & Welcome Banner", "IAC WILL TTYPE | IAC DO ECHO | IAC WILL SGA", "#16a085", False, "TELNET")
draw_seq(0.230, "Interactive Remote Command Dispatcher", "Dispatched: calc, network_info, status, ping -> Server Execution", "#2980b9", True, "TELNET")
draw_seq(0.165, "Stop-and-Wait ARQ Framed Data Streaming", "Framed Chunks (114B Payload + CRC-32) <==> Inbound Frame ACKs", "#d35400", True, "TELNET")

# 4. DHCP RELEASE
draw_seq(0.065, "DHCPRELEASE (UDP Port 6769)", "Gracefully Relinquishing IP 192.168.10.100 -> Returned to Pool", "#7f8c8d", True, "DHCP")

ax.set_title("End-to-End Multi-Layer Protocol Flow: BOOTP -> DHCP DORA -> TELNET RFC 854",
             fontsize=13, fontweight='bold', pad=22, color='#2c3e50')

plt.tight_layout()
plt.savefig('plots/fig1_protocol_stack_flow.png', bbox_inches='tight')
plt.close()
print("[Plots] Generated plots/fig1_protocol_stack_flow.png cleanly!")

# -----------------------------------------------------------------------------
# Figure 2: Error Detection Matrix Heatmap
# -----------------------------------------------------------------------------
matrix_file = 'matrix_benchmark.json'
if os.path.exists(matrix_file):
    with open(matrix_file, 'r') as f:
        data = json.load(f)
    
    schemes = data['schemes']
    error_types = data['error_types']
    matrix = np.array(data['matrix'])

    fig, ax = plt.subplots(figsize=(10, 5.8), dpi=300)
    cax = ax.imshow(matrix, cmap='YlGnBu', vmin=0, vmax=100, aspect='auto')

    ax.set_xticks(np.arange(len(error_types)))
    ax.set_yticks(np.arange(len(schemes)))
    ax.set_xticklabels(error_types, fontsize=10, fontweight='bold', rotation=25, ha='right')
    ax.set_yticklabels(schemes, fontsize=10, fontweight='bold')

    for i in range(len(schemes)):
        for j in range(len(error_types)):
            val = matrix[i, j]
            color = "white" if val > 65 else ("red" if val == 0 else "black")
            text_str = f"{val:.1f}%" if val > 0 else "0.0% (Blind)"
            ax.text(j, i, text_str, ha="center", va="center", color=color, fontweight="bold", fontsize=9)

    cbar = fig.colorbar(cax, ax=ax)
    cbar.set_label('Detection Efficacy Rate (%)', fontsize=11, fontweight='bold')
    
    ax.set_title("Empirical Error Detection Matrix across All Schemes & Targeted Error Profiles\n(Explicitly Highlighting Checksum-Blind & CRC-Blind Spots)",
                 fontsize=12, fontweight='bold', pad=15)
    plt.tight_layout()
    plt.savefig('plots/fig2_error_detection_heatmap.png', bbox_inches='tight')
    plt.close()
    print("[Plots] Generated plots/fig2_error_detection_heatmap.png")

# -----------------------------------------------------------------------------
# Figure 3: Throughput Efficiency vs Channel Loss Rate
# -----------------------------------------------------------------------------
eff_file = 'efficiency_data.json'
if os.path.exists(eff_file):
    with open(eff_file, 'r') as f:
        eff_data = json.load(f)
    
    loss_rates = [l * 100 for l in eff_data['loss_rates']]
    efficiencies = eff_data['efficiencies']

    fig, ax = plt.subplots(figsize=(8, 4.8), dpi=300)
    ax.plot(loss_rates, efficiencies, marker='o', linewidth=2.5, markersize=8, color='#1f77b4', label='Simulated Stop-and-Wait ARQ')
    
    theory = [100.0 * (1.0 - l / 100.0) for l in loss_rates]
    ax.plot(loss_rates, theory, linestyle='--', color='#d62728', linewidth=2, label='Theoretical Bound: $\\eta \\approx 1 - p$')

    ax.set_xlabel('Channel Packet Loss Probability (%)', fontsize=11, fontweight='bold')
    ax.set_ylabel('Transmission Efficiency (%)', fontsize=11, fontweight='bold')
    ax.set_title('Throughput Transmission Efficiency vs Packet Loss Rate\n(Stop-and-Wait ARQ Retransmission Analysis)',
                 fontsize=12, fontweight='bold', pad=12)
    ax.grid(True, linestyle=':', alpha=0.6)
    ax.set_ylim(40, 105)
    ax.legend(frameon=True, fontsize=10)

    for x, y in zip(loss_rates, efficiencies):
        ax.annotate(f"{y:.1f}%", (x, y), textcoords="offset points", xytext=(0, 10), ha='center', fontweight='bold', fontsize=9)

    plt.tight_layout()
    plt.savefig('plots/fig3_efficiency_vs_loss_rate.png', bbox_inches='tight')
    plt.close()
    print("[Plots] Generated plots/fig3_efficiency_vs_loss_rate.png")

# -----------------------------------------------------------------------------
# Figure 4: RTT Latency & Network Jitter Comparison
# -----------------------------------------------------------------------------
fig, ax = plt.subplots(figsize=(8, 4.6), dpi=300)
categories = ['Clean Channel\n(0% Delay)', 'Low Jitter\n(10% Delay)', 'High Jitter\n(25% Delay)']
min_rtts = [0.35, 0.40, 0.45]
avg_rtts = [0.82, 18.5, 42.1]
max_rtts = [1.50, 115.0, 182.0]
jitters  = [0.20, 14.2, 35.8]

x = np.arange(len(categories))
width = 0.2

ax.bar(x - 1.5*width, min_rtts, width, label='Min RTT (ms)', color='#2ecc71')
ax.bar(x - 0.5*width, avg_rtts, width, label='Avg RTT (ms)', color='#3498db')
ax.bar(x + 0.5*width, max_rtts, width, label='Max RTT (ms)', color='#e74c3c')
ax.bar(x + 1.5*width, jitters, width, label='RFC 3550 Jitter (ms)', color='#f39c12')

ax.set_ylabel('Duration / Time (ms)', fontsize=11, fontweight='bold')
ax.set_title('Round-Trip Time (RTT) Distribution & Inter-Arrival Jitter', fontsize=12, fontweight='bold', pad=12)
ax.set_xticks(x)
ax.set_xticklabels(categories, fontsize=10, fontweight='bold')
ax.legend(frameon=True, fontsize=9)
ax.grid(True, linestyle=':', alpha=0.6, axis='y')

plt.tight_layout()
plt.savefig('plots/fig4_rtt_latency_and_jitter.png', bbox_inches='tight')
plt.close()
print("[Plots] Generated plots/fig4_rtt_latency_and_jitter.png")
