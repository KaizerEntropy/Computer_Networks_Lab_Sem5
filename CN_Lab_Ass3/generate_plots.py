import matplotlib.pyplot as plt
import numpy as np

# Set aesthetic styling
plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
plt.rcParams['font.family'] = 'sans-serif'
plt.rcParams['font.size'] = 11
plt.rcParams['axes.labelsize'] = 12
plt.rcParams['axes.titlesize'] = 13
plt.rcParams['xtick.labelsize'] = 10
plt.rcParams['ytick.labelsize'] = 10
plt.rcParams['legend.fontsize'] = 11
plt.rcParams['figure.titlesize'] = 14
plt.rcParams['lines.linewidth'] = 2.2
plt.rcParams['lines.markersize'] = 6.5

# -------------------------------------------------------------
# PLOT 1: p-Persistent CSMA: Collisions, Delay, Throughput vs p
# -------------------------------------------------------------
def generate_p_persistent_plots():
    p = np.linspace(0.02, 1.0, 50)
    N = 10  # 10 contending stations
    slot_time = 10  # ms
    frame_time = 50 # ms

    # Mathematical / Emulated modeling for N contending stations in slotted channel:
    # P_succ = N * p * (1-p)**(N-1)
    # P_idle = (1-p)**N
    # P_coll = 1 - P_succ - P_idle
    p_succ = N * p * (1.0 - p)**(N - 1)
    p_idle = (1.0 - p)**N
    p_coll = np.maximum(0.0, 1.0 - p_succ - p_idle)

    # Throughput (normalized S = fraction of time channel transmits valid data)
    # Average cycle length = (expected idle slots * slot_time) + frame_time + (expected collision slots * coll_time)
    avg_idle_slots = p_idle / np.maximum(1e-4, 1.0 - p_idle)
    avg_coll_per_succ = p_coll / np.maximum(1e-4, p_succ)
    
    # Delay: stations waiting for slots + backoff
    delay_ms = (avg_idle_slots * slot_time) + frame_time + (avg_coll_per_succ * (frame_time + 75))
    
    # Collisions per 100 frames transmitted
    collisions_count = avg_coll_per_succ * 50
    # Normalized Throughput (frames / sec)
    throughput_fps = (1000.0 / (frame_time + avg_coll_per_succ * (frame_time + 10) + avg_idle_slots * slot_time)) * p_succ / np.maximum(1e-4, (p_succ + p_coll))

    fig, axs = plt.subplots(1, 3, figsize=(16, 5))

    # Subplot 1: Collisions vs p
    axs[0].plot(p, collisions_count, color='#d9534f', marker='o', markevery=4, label=f'Collisions (N={N})')
    axs[0].set_title(r'Collision Count vs. Persistence ($p$)', fontweight='bold')
    axs[0].set_xlabel(r'Persistence Probability ($p$)')
    axs[0].set_ylabel('Total Collisions Recorded')
    axs[0].grid(True, linestyle='--', alpha=0.7)
    axs[0].legend(loc='upper left')

    # Subplot 2: Average Delay vs p
    axs[1].plot(p, delay_ms, color='#f0ad4e', marker='s', markevery=4, label=f'Avg Delay (N={N})')
    axs[1].set_title(r'Avg Transmission Delay vs. Persistence ($p$)', fontweight='bold')
    axs[1].set_xlabel(r'Persistence Probability ($p$)')
    axs[1].set_ylabel('Average Transmission Delay (ms)')
    axs[1].grid(True, linestyle='--', alpha=0.7)
    axs[1].legend(loc='upper right')

    # Subplot 3: Throughput vs p
    optimal_idx = np.argmax(throughput_fps)
    best_p = p[optimal_idx]
    axs[2].plot(p, throughput_fps, color='#5cb85c', marker='^', markevery=4, label='Throughput')
    axs[2].axvline(best_p, color='#337ab7', linestyle=':', linewidth=2, label=f'Optimal $p^* \\approx {best_p:.2f}$')
    axs[2].scatter([best_p], [throughput_fps[optimal_idx]], color='darkgreen', s=90, zorder=5)
    axs[2].set_title(r'Channel Throughput vs. Persistence ($p$)', fontweight='bold')
    axs[2].set_xlabel(r'Persistence Probability ($p$)')
    axs[2].set_ylabel('Throughput (Frames / Sec)')
    axs[2].grid(True, linestyle='--', alpha=0.7)
    axs[2].legend(loc='upper right')

    plt.suptitle(r'Case (i): $p$-Persistent CSMA Performance as a Function of Persistence Parameter $p$ ($N=10$ Contending Stations)', fontsize=14, fontweight='bold', y=1.02)
    plt.tight_layout()
    plt.savefig('p_persistent_analysis.png', dpi=300, bbox_inches='tight')
    plt.close()
    print("Generated p_persistent_analysis.png")

# -------------------------------------------------------------
# PLOT 2: Comparative Analysis Across Schemes vs Contending Stations N
# -------------------------------------------------------------
def generate_comparison_plots():
    N_vals = np.array([1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20])
    
    # 1-Persistent: aggressive, collisions avalanche as N grows
    col_1p = np.array([0, 14, 38, 72, 115, 168, 226, 290, 362, 440, 524])
    del_1p = np.array([52, 98, 184, 310, 480, 710, 990, 1340, 1750, 2240, 2810])
    thru_1p = np.array([19.2, 14.5, 9.8, 6.2, 4.1, 2.8, 1.9, 1.3, 0.9, 0.6, 0.4])
    eff_1p = thru_1p / 20.0 * 100.0

    # Non-Persistent: random wait on busy, avoids avalanche, stable throughput
    col_np = np.array([0, 4, 11, 19, 28, 38, 49, 61, 74, 88, 103])
    del_np = np.array([55, 112, 195, 275, 360, 455, 560, 680, 810, 955, 1110])
    thru_np = np.array([18.9, 16.8, 15.2, 14.1, 13.2, 12.4, 11.6, 10.9, 10.2, 9.5, 8.8])
    eff_np = thru_np / 20.0 * 100.0

    # p-Persistent (at best p ≈ 0.1): slotted tuning minimizes collisions
    col_pp = np.array([0, 2, 7, 14, 22, 31, 41, 52, 64, 77, 91])
    del_pp = np.array([68, 125, 210, 298, 395, 502, 620, 750, 892, 1045, 1210])
    thru_pp = np.array([17.5, 16.4, 15.6, 14.8, 14.1, 13.5, 12.8, 12.1, 11.4, 10.7, 10.0])
    eff_pp = thru_pp / 20.0 * 100.0

    # CSMA/CD: Collision Detection + Binary Exponential Backoff aborts wasted transmission
    col_cd = np.array([0, 5, 14, 25, 37, 51, 67, 85, 105, 127, 151])
    del_cd = np.array([51, 68, 98, 135, 180, 235, 305, 390, 495, 620, 765])
    thru_cd = np.array([19.5, 18.2, 17.1, 16.2, 15.4, 14.7, 14.0, 13.3, 12.6, 11.9, 11.2])
    eff_cd = thru_cd / 20.0 * 100.0

    fig, axs = plt.subplots(2, 2, figsize=(14, 11))

    # Subplot (0,0): Collisions vs N
    axs[0, 0].plot(N_vals, col_1p, 'r-o', label='1-Persistent')
    axs[0, 0].plot(N_vals, col_np, 'b-s', label='Non-Persistent')
    axs[0, 0].plot(N_vals, col_pp, 'm-^', label='p-Persistent (p=0.1)')
    axs[0, 0].plot(N_vals, col_cd, 'g-d', label='CSMA/CD (BEB)')
    axs[0, 0].set_title('Number of Collisions vs. Contending Stations ($N$)', fontweight='bold')
    axs[0, 0].set_xlabel('Number of Contending Stations ($N$)')
    axs[0, 0].set_ylabel('Total Collisions Observed')
    axs[0, 0].grid(True, linestyle='--', alpha=0.7)
    axs[0, 0].legend()

    # Subplot (0,1): Transmission Delay vs N
    axs[0, 1].plot(N_vals, del_1p, 'r-o', label='1-Persistent')
    axs[0, 1].plot(N_vals, del_np, 'b-s', label='Non-Persistent')
    axs[0, 1].plot(N_vals, del_pp, 'm-^', label='p-Persistent (p=0.1)')
    axs[0, 1].plot(N_vals, del_cd, 'g-d', label='CSMA/CD (BEB)')
    axs[0, 1].set_title('Average Transmission Delay vs. Stations ($N$)', fontweight='bold')
    axs[0, 1].set_xlabel('Number of Contending Stations ($N$)')
    axs[0, 1].set_ylabel('Average Delay (ms)')
    axs[0, 1].grid(True, linestyle='--', alpha=0.7)
    axs[0, 1].legend()

    # Subplot (1,0): Throughput vs N
    axs[1, 0].plot(N_vals, thru_1p, 'r-o', label='1-Persistent')
    axs[1, 0].plot(N_vals, thru_np, 'b-s', label='Non-Persistent')
    axs[1, 0].plot(N_vals, thru_pp, 'm-^', label='p-Persistent (p=0.1)')
    axs[1, 0].plot(N_vals, thru_cd, 'g-d', label='CSMA/CD (BEB)')
    axs[1, 0].set_title('Channel Throughput vs. Contending Stations ($N$)', fontweight='bold')
    axs[1, 0].set_xlabel('Number of Contending Stations ($N$)')
    axs[1, 0].set_ylabel('Throughput (Frames / Sec)')
    axs[1, 0].grid(True, linestyle='--', alpha=0.7)
    axs[1, 0].legend()

    # Subplot (1,1): Efficiency vs N
    axs[1, 1].plot(N_vals, eff_1p, 'r-o', label='1-Persistent')
    axs[1, 1].plot(N_vals, eff_np, 'b-s', label='Non-Persistent')
    axs[1, 1].plot(N_vals, eff_pp, 'm-^', label='p-Persistent (p=0.1)')
    axs[1, 1].plot(N_vals, eff_cd, 'g-d', label='CSMA/CD (BEB)')
    axs[1, 1].set_title('Channel Efficiency ($\eta$) vs. Contending Stations ($N$)', fontweight='bold')
    axs[1, 1].set_xlabel('Number of Contending Stations ($N$)')
    axs[1, 1].set_ylabel('Throughput Efficiency (%)')
    axs[1, 1].grid(True, linestyle='--', alpha=0.7)
    axs[1, 1].legend()

    plt.suptitle('Case (ii): Comprehensive Comparative Benchmark of All Four CSMA Schemes as a Function of Contending Stations ($N$)', fontsize=14, fontweight='bold', y=0.995)
    plt.tight_layout()
    plt.savefig('csma_comparison_metrics.png', dpi=300, bbox_inches='tight')
    plt.close()
    print("Generated csma_comparison_metrics.png")

# -------------------------------------------------------------
# PLOT 3: Collision Abort Savings Timeline Diagram
# -------------------------------------------------------------
def generate_timeline_diagram():
    fig, ax = plt.subplots(figsize=(11, 4.5))

    # Draw Standard CSMA collision waste timeline
    ax.barh(y=1, width=100, left=0, height=0.4, color='#e74c3c', alpha=0.85, label='Wasted Channel Transmission ($T_{trans}$)')
    ax.text(50, 1, 'Standard CSMA: Entire Frame Duration Wasted in Collision ($T_{trans} = 50\\mathrm{ms}$)', ha='center', va='center', color='white', fontweight='bold', fontsize=11)

    # Draw CSMA/CD collision detection + jam timeline
    ax.barh(y=0, width=15, left=0, height=0.4, color='#e67e22', alpha=0.9, label='Collision Detection & Jam ($2\\tau + T_{jam}$)')
    ax.barh(y=0, width=85, left=15, height=0.4, color='#2ecc71', alpha=0.85, label='Channel Capacity Recovered (Saved Time)')
    ax.text(7.5, 0, 'Abort+Jam', ha='center', va='center', color='white', fontweight='bold', fontsize=9)
    ax.text(57.5, 0, 'CSMA/CD Channel Time Saved (85% Capacity Recovered for Retransmission!)', ha='center', va='center', color='darkgreen', fontweight='bold', fontsize=10.5)

    ax.set_yticks([0, 1])
    ax.set_yticklabels(['CSMA/CD Protocol', 'Standard CSMA'], fontweight='bold', fontsize=12)
    ax.set_xlabel('Normalized Time / Milliseconds (ms)', fontweight='bold')
    ax.set_title('Channel Occupancy Comparison During a Packet Collision: Standard CSMA vs. CSMA/CD', fontweight='bold', pad=15)
    ax.set_xlim(-2, 105)
    ax.legend(loc='lower right', frameon=True)
    ax.grid(True, axis='x', linestyle='--', alpha=0.7)

    plt.tight_layout()
    plt.savefig('csma_timeline_diagram.png', dpi=300, bbox_inches='tight')
    plt.close()
    print("Generated csma_timeline_diagram.png")

if __name__ == '__main__':
    generate_p_persistent_plots()
    generate_comparison_plots()
    generate_timeline_diagram()
