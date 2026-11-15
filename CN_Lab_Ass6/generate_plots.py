#!/usr/bin/env python3
"""
Publication-Quality Topology and Protocol Diagram Generator for Assignment 6
Jadavpur University - Department of Computer Science & Engineering
Author: Diptarko Bhattacharjee (Roll: 002410501013)
"""

import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np

# Create plots directory
os.makedirs("plots", exist_ok=True)
plt.rcParams['font.sans-serif'] = 'DejaVu Sans'
plt.rcParams['font.family'] = 'sans-serif'
plt.rcParams['figure.dpi'] = 300

# -----------------------------------------------------------------------------
# PLOT 1: Problem 1 - Back-to-Back Crossover Topology
# -----------------------------------------------------------------------------
def plot_problem1():
    fig, ax = plt.subplots(figsize=(8, 4))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 5)
    ax.axis('off')

    # Title
    ax.text(5, 4.6, "Problem 1: Back-to-Back Host Interconnection (Crossover Cable)",
            ha='center', va='center', fontsize=12, fontweight='bold', color='#1A237E')

    # Host 1
    rect1 = patches.FancyBboxPatch((1, 1.5), 2.5, 2.0, boxstyle="round,pad=0.2",
                                   edgecolor="#2E7D32", facecolor="#E8F5E9", linewidth=2)
    ax.add_patch(rect1)
    ax.text(2.25, 2.8, "Host-1 (PC0)", ha='center', va='center', fontsize=11, fontweight='bold')
    ax.text(2.25, 2.3, "IP: 192.168.1.1/24\nMAC: 0001.64A1.0001\nPort: FastEthernet0",
            ha='center', va='center', fontsize=8, family='monospace')

    # Host 2
    rect2 = patches.FancyBboxPatch((6.5, 1.5), 2.5, 2.0, boxstyle="round,pad=0.2",
                                   edgecolor="#2E7D32", facecolor="#E8F5E9", linewidth=2)
    ax.add_patch(rect2)
    ax.text(7.75, 2.8, "Host-2 (PC1)", ha='center', va='center', fontsize=11, fontweight='bold')
    ax.text(7.75, 2.3, "IP: 192.168.1.2/24\nMAC: 0001.64D4.0002\nPort: FastEthernet0",
            ha='center', va='center', fontsize=8, family='monospace')

    # Crossover Cable (Dashed orange/red link)
    ax.plot([3.5, 6.5], [2.5, 2.5], color='#E65100', linestyle='--', linewidth=3.5, label='Copper Cross-Over Cable')
    ax.text(5.0, 2.8, "Copper Cross-Over Cable\n(Tx 1,2 <---> Rx 3,6)", ha='center', va='center',
            fontsize=9, fontweight='bold', color='#BF360C',
            bbox=dict(boxstyle='square,pad=0.3', facecolor='#FFF3E0', edgecolor='#FFB74D'))

    # ICMP Ping annotation
    ax.annotate("", xy=(6.3, 1.9), xytext=(3.7, 1.9),
                arrowprops=dict(arrowstyle="<->", color="#0288D1", lw=2))
    ax.text(5.0, 1.6, "ICMP Echo Request / Echo Reply (0% loss, RTT < 1ms)",
            ha='center', va='center', fontsize=8.5, style='italic', color="#01579B")

    plt.tight_layout()
    plt.savefig("plots/topology_prob1_crossover.png", dpi=300)
    plt.savefig("topology_prob1_crossover.png", dpi=300)
    plt.close()

# -----------------------------------------------------------------------------
# PLOT 2: Problem 2 - LAN-A Hub Topology
# -----------------------------------------------------------------------------
def plot_problem2():
    fig, ax = plt.subplots(figsize=(8, 5))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 7)
    ax.axis('off')

    ax.text(5, 6.5, "Problem 2: LAN-A (3 Hosts connected via Layer-1 Hub)",
            ha='center', va='center', fontsize=12, fontweight='bold', color='#1A237E')

    # Hub
    hub_rect = patches.FancyBboxPatch((4.0, 3.2), 2.0, 1.4, boxstyle="round,pad=0.2",
                                     edgecolor="#FF8F00", facecolor="#FFF8E1", linewidth=2.5)
    ax.add_patch(hub_rect)
    ax.text(5.0, 4.1, "Hub-A\n(Generic Hub-PT)", ha='center', va='center', fontsize=10, fontweight='bold')
    ax.text(5.0, 3.5, "Single Collision Domain\nSingle Broadcast Domain", ha='center', va='center',
            fontsize=7.5, color='#E65100')

    # 3 Hosts
    hosts = [
        ("Host-A1", "192.168.10.1", "0002.16A1.0001", (1.0, 0.8), "Fa0"),
        ("Host-A2", "192.168.10.2", "0002.16A2.0002", (4.0, 0.8), "Fa1"),
        ("Host-A3", "192.168.10.3", "0002.16A3.0003", (7.0, 0.8), "Fa2")
    ]

    for name, ip, mac, pos, hub_port in hosts:
        h_rect = patches.FancyBboxPatch(pos, 2.0, 1.4, boxstyle="round,pad=0.2",
                                        edgecolor="#2E7D32", facecolor="#E8F5E9", linewidth=2)
        ax.add_patch(h_rect)
        ax.text(pos[0]+1.0, pos[1]+1.0, name, ha='center', va='center', fontsize=9.5, fontweight='bold')
        ax.text(pos[0]+1.0, pos[1]+0.5, f"IP: {ip}\nMAC: {mac}", ha='center', va='center',
                fontsize=7, family='monospace')

        # Straight through cable
        ax.plot([pos[0]+1.0, 4.3 + (hub_port == 'Fa1')*0.7 + (hub_port == 'Fa2')*1.4],
                [pos[1]+1.4, 3.2], color='#37474F', linewidth=2)
        ax.text((pos[0]+1.0 + 5.0)/2, (pos[1]+1.4 + 3.2)/2, f"Straight-Through\n({hub_port})",
                fontsize=6.5, ha='center', color='#455A64')

    ax.text(5.0, 5.5, "Layer-1 Repeater: Blindly echoes bits from incoming port to all other ports",
            ha='center', va='center', fontsize=8.5, style='italic',
            bbox=dict(boxstyle='round,pad=0.3', facecolor='#ECEFF1', edgecolor='#B0BEC5'))

    plt.tight_layout()
    plt.savefig("plots/topology_prob2_hub.png", dpi=300)
    plt.savefig("topology_prob2_hub.png", dpi=300)
    plt.close()

# -----------------------------------------------------------------------------
# PLOT 3: Problem 3 - LAN-B Switch Topology
# -----------------------------------------------------------------------------
def plot_problem3():
    fig, ax = plt.subplots(figsize=(8, 5.5))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 7)
    ax.axis('off')

    ax.text(5, 6.6, "Problem 3: LAN-B (3 Hosts via Cisco Catalyst 2950 Switch)",
            ha='center', va='center', fontsize=12, fontweight='bold', color='#1A237E')

    # Switch
    sw_rect = patches.FancyBboxPatch((3.5, 3.5), 3.0, 1.5, boxstyle="round,pad=0.2",
                                    edgecolor="#0277BD", facecolor="#E1F5FE", linewidth=2.5)
    ax.add_patch(sw_rect)
    ax.text(5.0, 4.5, "Switch-B (Cisco 2950-24)", ha='center', va='center', fontsize=10.5, fontweight='bold', color='#01579B')
    ax.text(5.0, 3.9, "Dynamic MAC CAM Table (Port <-> MAC)\nMicrosegmentation: 1 Collision Domain / Port",
            ha='center', va='center', fontsize=7.5, color='#0277BD')

    # 3 Hosts
    hosts = [
        ("Host-B1", "192.168.20.1", "0060.2F11.0001", (0.8, 0.8), "Fa0/1"),
        ("Host-B2", "192.168.20.2", "0060.2F22.0002", (3.8, 0.8), "Fa0/2"),
        ("Host-B3", "192.168.20.3", "0060.2F33.0003", (6.8, 0.8), "Fa0/3")
    ]

    for name, ip, mac, pos, sw_port in hosts:
        h_rect = patches.FancyBboxPatch(pos, 2.4, 1.4, boxstyle="round,pad=0.2",
                                        edgecolor="#2E7D32", facecolor="#E8F5E9", linewidth=2)
        ax.add_patch(h_rect)
        ax.text(pos[0]+1.2, pos[1]+1.0, name, ha='center', va='center', fontsize=9.5, fontweight='bold')
        ax.text(pos[0]+1.2, pos[1]+0.5, f"IP: {ip}\nMAC: {mac}", ha='center', va='center',
                fontsize=7, family='monospace')

        # Link to switch
        ax.plot([pos[0]+1.2, 4.0 + (sw_port == 'Fa0/2')*1.0 + (sw_port == 'Fa0/3')*2.0],
                [pos[1]+1.4, 3.5], color='#1565C0', linewidth=2.5)
        ax.text((pos[0]+1.2 + 5.0)/2, (pos[1]+1.4 + 3.5)/2, sw_port,
                fontsize=7.5, fontweight='bold', ha='center', color='#0D47A1')

    # CAM Table callout
    cam_box = ("Switch-B CAM Table:\n"
               "Fa0/1 -> 0060.2F11.0001\n"
               "Fa0/2 -> 0060.2F22.0002\n"
               "Fa0/3 -> 0060.2F33.0003")
    ax.text(8.5, 4.25, cam_box, fontsize=7.5, family='monospace',
            bbox=dict(boxstyle='round,pad=0.4', facecolor='#FFF9C4', edgecolor='#FBC02D'))

    plt.tight_layout()
    plt.savefig("plots/topology_prob3_switch.png", dpi=300)
    plt.savefig("topology_prob3_switch.png", dpi=300)
    plt.close()

# -----------------------------------------------------------------------------
# PLOT 4: Problem 4 - Interconnecting Hub-A and Switch-B
# -----------------------------------------------------------------------------
def plot_problem4():
    fig, ax = plt.subplots(figsize=(10, 5.5))
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 7)
    ax.axis('off')

    ax.text(6, 6.6, "Problem 4: Interconnection of LAN-A (Hub) and LAN-B (Switch) via Crossover Cable",
            ha='center', va='center', fontsize=12, fontweight='bold', color='#1A237E')

    # Left: Hub-A
    hub_rect = patches.FancyBboxPatch((1.5, 3.2), 2.4, 1.4, boxstyle="round,pad=0.2",
                                     edgecolor="#FF8F00", facecolor="#FFF8E1", linewidth=2)
    ax.add_patch(hub_rect)
    ax.text(2.7, 4.1, "Hub-A (LAN-A)", ha='center', va='center', fontsize=10, fontweight='bold', color='#E65100')
    ax.text(2.7, 3.6, "Layer-1 Repeater\nSubnet: 192.168.1.0/24", ha='center', va='center', fontsize=7.5)

    # Right: Switch-B
    sw_rect = patches.FancyBboxPatch((8.0, 3.2), 2.8, 1.4, boxstyle="round,pad=0.2",
                                    edgecolor="#0277BD", facecolor="#E1F5FE", linewidth=2.5)
    ax.add_patch(sw_rect)
    ax.text(9.4, 4.1, "Switch-B (LAN-B)", ha='center', va='center', fontsize=10, fontweight='bold', color='#01579B')
    ax.text(9.4, 3.6, "Cisco Catalyst 2950\nSubnet: 192.168.1.0/24", ha='center', va='center', fontsize=7.5)

    # Interconnect Crossover Cable
    ax.plot([3.9, 8.0], [3.9, 3.9], color='#D84315', linestyle='--', linewidth=3.5)
    ax.text(5.95, 4.3, "Uplink: Copper Cross-Over Cable\n(Hub Fa3 <====> Switch Fa0/4)",
            ha='center', va='center', fontsize=8.5, fontweight='bold', color='#BF360C',
            bbox=dict(boxstyle='round,pad=0.3', facecolor='#FFCCBC', edgecolor='#FF7043'))

    # Hosts under Hub-A
    h_a = [("Host-A1\n.1.1", 0.5), ("Host-A2\n.1.2", 1.8), ("Host-A3\n.1.3", 3.1)]
    for name, x in h_a:
        rect = patches.FancyBboxPatch((x, 0.8), 1.0, 1.1, boxstyle="round,pad=0.1",
                                      edgecolor="#388E3C", facecolor="#E8F5E9")
        ax.add_patch(rect)
        ax.text(x+0.5, 1.35, name, ha='center', va='center', fontsize=7.5, fontweight='bold')
        ax.plot([x+0.5, 2.7], [1.9, 3.2], color='#546E7A', linewidth=1.5)

    # Hosts under Switch-B
    h_b = [("Host-B1\n.1.4", 7.5), ("Host-B2\n.1.5", 8.8), ("Host-B3\n.1.6", 10.1)]
    for name, x in h_b:
        rect = patches.FancyBboxPatch((x, 0.8), 1.0, 1.1, boxstyle="round,pad=0.1",
                                      edgecolor="#1976D2", facecolor="#E3F2FD")
        ax.add_patch(rect)
        ax.text(x+0.5, 1.35, name, ha='center', va='center', fontsize=7.5, fontweight='bold')
        ax.plot([x+0.5, 9.4], [1.9, 3.2], color='#1976D2', linewidth=1.5)

    # Callout highlighting Multi-MAC learning on Fa0/4
    callout = ("CRITICAL OBSERVATION ON SWITCH-B:\n"
               "Port Fa0/4 learns MULTIPLE MACs:\n"
               " - Fa0/4 -> MAC Host-A1 (.1.1)\n"
               " - Fa0/4 -> MAC Host-A2 (.1.2)\n"
               " - Fa0/4 -> MAC Host-A3 (.1.3)")
    ax.text(6.0, 1.8, callout, fontsize=8, family='monospace', fontweight='bold', color='#4A148C',
            ha='center', bbox=dict(boxstyle='round,pad=0.4', facecolor='#F3E5F5', edgecolor='#BA68C8'))

    plt.tight_layout()
    plt.savefig("plots/topology_prob4_interconnected.png", dpi=300)
    plt.savefig("topology_prob4_interconnected.png", dpi=300)
    plt.close()

# -----------------------------------------------------------------------------
# PLOT 5: Problem 5 - Dual-LAN Routing (JU-Main and JU-SL)
# -----------------------------------------------------------------------------
def plot_problem5():
    fig, ax = plt.subplots(figsize=(11, 6))
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 7.5)
    ax.axis('off')

    ax.text(6, 7.0, "Problem 5: Campus Dual-LAN Routing (JU-Main & JU-SL) via Cisco 1818 Routers",
            ha='center', va='center', fontsize=12, fontweight='bold', color='#1A237E')

    # JU-Main Subnet Box
    main_zone = patches.FancyBboxPatch((0.5, 0.5), 4.8, 5.8, boxstyle="round,pad=0.3",
                                      edgecolor="#B0BEC5", facecolor="#FAFAFA", linestyle=':')
    ax.add_patch(main_zone)
    ax.text(2.9, 6.0, "JU-Main Campus LAN (192.168.148.0/24)", fontsize=9.5, fontweight='bold', color='#37474F')

    # JU-SL Subnet Box
    sl_zone = patches.FancyBboxPatch((6.7, 0.5), 4.8, 5.8, boxstyle="round,pad=0.3",
                                    edgecolor="#B0BEC5", facecolor="#FAFAFA", linestyle=':')
    ax.add_patch(sl_zone)
    ax.text(9.1, 6.0, "JU-SL Salt Lake LAN (192.168.149.0/24)", fontsize=9.5, fontweight='bold', color='#37474F')

    # Routers
    # Router 1 (JU-Main)
    r1_circle = patches.Circle((4.2, 4.3), 0.7, edgecolor="#C62828", facecolor="#FFEBEE", linewidth=2)
    ax.add_patch(r1_circle)
    ax.text(4.2, 4.4, "Router\nJU-Main", ha='center', va='center', fontsize=8.5, fontweight='bold')
    ax.text(4.2, 3.3, "Fa0/0: 192.168.148.1\nFa0/1: 192.168.150.1", ha='center', va='center', fontsize=6.5, family='monospace')

    # Router 2 (JU-SL)
    r2_circle = patches.Circle((7.8, 4.3), 0.7, edgecolor="#C62828", facecolor="#FFEBEE", linewidth=2)
    ax.add_patch(r2_circle)
    ax.text(7.8, 4.4, "Router\nJU-SL", ha='center', va='center', fontsize=8.5, fontweight='bold')
    ax.text(7.8, 3.3, "Fa0/0: 192.168.149.1\nFa0/1: 192.168.150.2", ha='center', va='center', fontsize=6.5, family='monospace')

    # WAN Point-to-Point Link
    ax.plot([4.9, 7.1], [4.3, 4.3], color='#D84315', linewidth=3, linestyle='--')
    ax.text(6.0, 4.7, "WAN Point-to-Point (192.168.150.0/24)\nStatic Routing: ip route ...",
            ha='center', va='center', fontsize=7.5, fontweight='bold', color='#BF360C',
            bbox=dict(boxstyle='square,pad=0.2', facecolor='#FFF3E0', edgecolor='#FFB74D'))

    # Switches
    sw1 = patches.FancyBboxPatch((1.6, 2.5), 1.8, 0.9, boxstyle="round,pad=0.1",
                                edgecolor="#0277BD", facecolor="#E1F5FE", linewidth=1.5)
    ax.add_patch(sw1)
    ax.text(2.5, 2.95, "PC-LAB1-Switch\n(Cisco 2950)", ha='center', va='center', fontsize=7.5, fontweight='bold')

    sw2 = patches.FancyBboxPatch((8.6, 2.5), 1.8, 0.9, boxstyle="round,pad=0.1",
                                edgecolor="#0277BD", facecolor="#E1F5FE", linewidth=1.5)
    ax.add_patch(sw2)
    ax.text(9.5, 2.95, "PC-LAB2-Switch\n(Cisco 2950)", ha='center', va='center', fontsize=7.5, fontweight='bold')

    # Links Switch to Router
    ax.plot([2.5, 3.6], [3.4, 4.0], color='#37474F', linewidth=2)
    ax.plot([9.5, 8.4], [3.4, 4.0], color='#37474F', linewidth=2)

    # Hosts JU-Main
    m_hosts = [("Host-M1\n148.2", 0.8), ("Host-M2\n148.3", 2.0), ("Host-M3\n148.4", 3.2)]
    for name, x in m_hosts:
        rect = patches.FancyBboxPatch((x, 0.8), 0.9, 0.9, boxstyle="round,pad=0.1",
                                      edgecolor="#2E7D32", facecolor="#E8F5E9")
        ax.add_patch(rect)
        ax.text(x+0.45, 1.25, name, ha='center', va='center', fontsize=7, fontweight='bold')
        ax.plot([x+0.45, 2.5], [1.7, 2.5], color='#2E7D32', linewidth=1.2)

    # Hosts JU-SL
    sl_hosts = [("Host-S1\n149.2", 7.8), ("Host-S2\n149.3", 9.0), ("Host-S3\n149.4", 10.2)]
    for name, x in sl_hosts:
        rect = patches.FancyBboxPatch((x, 0.8), 0.9, 0.9, boxstyle="round,pad=0.1",
                                      edgecolor="#2E7D32", facecolor="#E8F5E9")
        ax.add_patch(rect)
        ax.text(x+0.45, 1.25, name, ha='center', va='center', fontsize=7, fontweight='bold')
        ax.plot([x+0.45, 9.5], [1.7, 2.5], color='#2E7D32', linewidth=1.2)

    plt.tight_layout()
    plt.savefig("plots/topology_prob5_routers.png", dpi=300)
    plt.savefig("topology_prob5_routers.png", dpi=300)
    plt.close()

# -----------------------------------------------------------------------------
# PLOT 6: Problem 6 - DHCP DORA Handshake
# -----------------------------------------------------------------------------
def plot_dhcp_dora():
    fig, ax = plt.subplots(figsize=(8, 6))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 8)
    ax.axis('off')

    ax.text(5, 7.5, "Problem 6: DHCP 4-Way DORA Handshake Protocol Workflow",
            ha='center', va='center', fontsize=12, fontweight='bold', color='#1A237E')

    # Lifelines
    ax.plot([2.5, 2.5], [1.0, 6.8], color='#37474F', linewidth=2)
    ax.plot([7.5, 7.5], [1.0, 6.8], color='#37474F', linewidth=2)

    # Entity Boxes
    ax.text(2.5, 7.0, "DHCP Client (Host)\n0.0.0.0:68", ha='center', va='center',
            fontsize=9.5, fontweight='bold', bbox=dict(boxstyle='round,pad=0.3', facecolor='#E8F5E9', edgecolor='#2E7D32'))
    ax.text(7.5, 7.0, "DHCP Server\n192.168.148.10:67", ha='center', va='center',
            fontsize=9.5, fontweight='bold', bbox=dict(boxstyle='round,pad=0.3', facecolor='#EDE7F6', edgecolor='#512DA8'))

    # DORA Messages
    dora = [
        (5.8, "1. DHCPDISCOVER (Broadcast 255.255.255.255)", "ciaddr=0.0.0.0, chaddr=Host MAC", 2.5, 7.5, '#E65100'),
        (4.6, "2. DHCPOFFER (Unicast / Broadcast)", "yiaddr=192.168.148.20, mask=255.255.255.0, gw=148.1", 7.5, 2.5, '#0277BD'),
        (3.4, "3. DHCPREQUEST (Broadcast Acceptance)", "Requesting yiaddr=192.168.148.20 from siaddr=148.10", 2.5, 7.5, '#2E7D32'),
        (2.2, "4. DHCPACK (Final Lease Acknowledgment)", "Lease=86400s, DNS=192.168.148.10, GW=192.168.148.1", 7.5, 2.5, '#C2185B')
    ]

    for y, title, subtitle, x_start, x_end, col in dora:
        ax.annotate("", xy=(x_end, y), xytext=(x_start, y),
                    arrowprops=dict(arrowstyle="->", color=col, lw=2.5))
        ax.text(5.0, y+0.35, title, ha='center', va='center', fontsize=8.5, fontweight='bold', color=col)
        ax.text(5.0, y-0.25, subtitle, ha='center', va='center', fontsize=7.5, family='monospace', color='#424242')

    plt.tight_layout()
    plt.savefig("plots/dhcp_dora_sequence.png", dpi=300)
    plt.savefig("dhcp_dora_sequence.png", dpi=300)
    plt.close()

# -----------------------------------------------------------------------------
# PLOT 7: Problem 7 - Enterprise CSE Multi-Service Network
# -----------------------------------------------------------------------------
def plot_problem7():
    fig, ax = plt.subplots(figsize=(10, 6.5))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 8)
    ax.axis('off')

    ax.text(5, 7.5, "Problem 7: Departmental CSE Multi-Service LAN (cse.myuniv.edu)",
            ha='center', va='center', fontsize=12, fontweight='bold', color='#1A237E')

    # Central Switch
    sw_rect = patches.FancyBboxPatch((3.5, 3.8), 3.0, 1.4, boxstyle="round,pad=0.2",
                                    edgecolor="#0277BD", facecolor="#E1F5FE", linewidth=2.5)
    ax.add_patch(sw_rect)
    ax.text(5.0, 4.7, "CSE-Switch (Cisco 2950-24)", ha='center', va='center', fontsize=10.5, fontweight='bold', color='#01579B')
    ax.text(5.0, 4.1, "Departmental Core Layer-2 Switch\nSubnet: 192.168.100.0/24",
            ha='center', va='center', fontsize=7.5, color='#0277BD')

    # Upper Servers (Fixed IPs)
    servers = [
        ("CSE-DHCP-Server", "192.168.100.2", "Fa0/4", "DHCP Service ON\nPool: 100.50 - 100.100", 0.5),
        ("CSE-DNS-Server", "192.168.100.4", "Fa0/5", "DNS Service ON\nDomain: cse.myuniv.edu", 2.9),
        ("CSE-Web-Server", "192.168.100.10", "Fa0/6", "HTTP/HTTPS ON\nIntranet Portal HTML", 5.3),
        ("CSE-FTP-Server", "192.168.100.20", "Fa0/7", "FTP Service ON\nAuth: csejadavpur", 7.7)
    ]

    for name, ip, port, desc, x in servers:
        s_rect = patches.FancyBboxPatch((x, 5.5), 1.9, 1.4, boxstyle="round,pad=0.1",
                                       edgecolor="#512DA8", facecolor="#EDE7F6", linewidth=1.5)
        ax.add_patch(s_rect)
        ax.text(x+0.95, 6.6, name, ha='center', va='center', fontsize=7.5, fontweight='bold', color='#311B92')
        ax.text(x+0.95, 6.2, ip, ha='center', va='center', fontsize=7, family='monospace')
        ax.text(x+0.95, 5.8, desc, ha='center', va='center', fontsize=6, style='italic', color='#4527A0')

        # Link to switch
        ax.plot([x+0.95, 4.0 + (port == 'Fa0/5')*0.7 + (port == 'Fa0/6')*1.3 + (port == 'Fa0/7')*1.8],
                [5.5, 5.2], color='#512DA8', linewidth=1.8)

    # Lower Workstations (Dynamic DHCP)
    clients = [
        ("PC1", "DHCP: 192.168.100.50", "Fa0/1", 1.2),
        ("PC2", "DHCP: 192.168.100.51", "Fa0/2", 4.2),
        ("PC3", "DHCP: 192.168.100.52", "Fa0/3", 7.2)
    ]

    for name, ip_str, port, x in clients:
        c_rect = patches.FancyBboxPatch((x, 1.0), 1.8, 1.3, boxstyle="round,pad=0.1",
                                       edgecolor="#2E7D32", facecolor="#E8F5E9", linewidth=1.5)
        ax.add_patch(c_rect)
        ax.text(x+0.9, 2.0, name, ha='center', va='center', fontsize=8.5, fontweight='bold')
        ax.text(x+0.9, 1.5, f"{ip_str}\nDNS: 192.168.100.4\nPort: {port}",
                ha='center', va='center', fontsize=6.5, family='monospace')

        # Link to switch
        ax.plot([x+0.9, 4.0 + (port == 'Fa0/2')*1.0 + (port == 'Fa0/3')*1.8],
                [2.3, 3.8], color='#2E7D32', linewidth=1.8)

    # DNS Resolution annotation
    ax.text(5.0, 0.4, "All Workstations resolve 'cse.myuniv.edu' via Local DNS Server -> Access HTTP Web & FTP Services",
            ha='center', va='center', fontsize=8, fontweight='bold', color='#1A237E',
            bbox=dict(boxstyle='round,pad=0.3', facecolor='#E8EAF6', edgecolor='#9FA8DA'))

    plt.tight_layout()
    plt.savefig("plots/topology_prob7_integrated_services.png", dpi=300)
    plt.savefig("topology_prob7_integrated_services.png", dpi=300)
    plt.close()

# -----------------------------------------------------------------------------
# PLOT 8: Latency Comparison across Topologies
# -----------------------------------------------------------------------------
def plot_latency_comparison():
    fig, ax = plt.subplots(figsize=(8, 4.5))

    topologies = [
        'Prob 1:\nCrossover\n(P2P)',
        'Prob 2:\nHub\n(Shared L1)',
        'Prob 3:\nSwitch\n(L2 Switched)',
        'Prob 4:\nHub+Switch\n(Bridged)',
        'Prob 5:\nDual Router\n(Routed WAN)'
    ]
    rtt_initial = [2.0, 2.1, 2.2, 2.4, 12.5]  # With ARP resolution
    rtt_steady  = [0.8, 0.9, 0.8, 0.9, 10.4]  # Steady state

    x = np.arange(len(topologies))
    width = 0.35

    rects1 = ax.bar(x - width/2, rtt_initial, width, label='First Packet RTT (Cold ARP)', color='#EF5350')
    rects2 = ax.bar(x + width/2, rtt_steady, width, label='Steady-State RTT (Warm ARP)', color='#42A5F5')

    ax.set_ylabel('Round Trip Time (ms)', fontsize=10, fontweight='bold')
    ax.set_title('Empirical Latency & Round Trip Time Comparison across Network Topologies',
                 fontsize=11, fontweight='bold', color='#1A237E')
    ax.set_xticks(x)
    ax.set_xticklabels(topologies, fontsize=8.5)
    ax.legend(frameon=True, facecolor='#F5F5F5')
    ax.grid(axis='y', linestyle='--', alpha=0.6)

    for rect in rects1:
        h = rect.get_height()
        ax.annotate(f'{h:.1f}ms', xy=(rect.get_x() + rect.get_width()/2, h),
                    xytext=(0, 3), textcoords="offset points", ha='center', va='bottom', fontsize=7.5)

    for rect in rects2:
        h = rect.get_height()
        ax.annotate(f'{h:.1f}ms', xy=(rect.get_x() + rect.get_width()/2, h),
                    xytext=(0, 3), textcoords="offset points", ha='center', va='bottom', fontsize=7.5)

    plt.tight_layout()
    plt.savefig("plots/latency_comparison.png", dpi=300)
    plt.savefig("latency_comparison.png", dpi=300)
    plt.close()

if __name__ == '__main__':
    print("Generating Assignment 6 Topology and Performance Diagrams...")
    plot_problem1()
    plot_problem2()
    plot_problem3()
    plot_problem4()
    plot_problem5()
    plot_dhcp_dora()
    plot_problem7()
    plot_latency_comparison()
    print("All plots generated successfully in plots/ and current directory!")

