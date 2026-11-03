import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as patches

# Set plot aesthetics
plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
plt.rcParams['font.family'] = 'DejaVu Sans'
plt.rcParams['font.size'] = 11
plt.rcParams['axes.titlesize'] = 14
plt.rcParams['axes.labelsize'] = 12
plt.rcParams['xtick.labelsize'] = 10
plt.rcParams['ytick.labelsize'] = 10
plt.rcParams['legend.fontsize'] = 11
plt.rcParams['figure.titlesize'] = 16

# ==============================================================================
# PLOT 1: Walsh-Hadamard Orthogonality Heatmap (8x8)
# ==============================================================================
def plot_orthogonality_matrix():
    def hadamard(n):
        if n == 1:
            return np.array([[1]])
        h = hadamard(n // 2)
        return np.block([[h, h], [h, -h]])

    H8 = hadamard(8)
    inner_products = np.dot(H8, H8.T)

    fig, ax = plt.subplots(figsize=(8, 7))
    cax = ax.matshow(inner_products, cmap='coolwarm', vmin=-8, vmax=8)
    fig.colorbar(cax, ticks=[-8, 0, 8], label='Inner Product: $W_i \\cdot W_j$')

    for i in range(8):
        for j in range(8):
            val = inner_products[i, j]
            color = 'white' if abs(val) > 4 else 'black'
            ax.text(j, i, f'{val:+d}' if val != 0 else '0', 
                    ha='center', va='center', color=color, fontweight='bold', fontsize=11)

    labels = [f'$W_{i}$' for i in range(8)]
    ax.set_xticks(range(8))
    ax.set_yticks(range(8))
    ax.set_xticklabels(labels)
    ax.set_yticklabels(labels)
    ax.set_title('Walsh-Hadamard $H_8$ Inner Product Correlation Matrix\n($W_i \\cdot W_j = 8\\delta_{ij}$ Zero Cross-Correlation)', pad=20, fontweight='bold')
    plt.tight_layout()
    plt.savefig('cdma_orthogonality_matrix.png', dpi=300)
    plt.close()
    print("Generated cdma_orthogonality_matrix.png")

# ==============================================================================
# PLOT 2: BER vs Channel Chip Noise Probability (Processing Gain)
# ==============================================================================
def plot_ber_vs_noise():
    noise_probs = np.linspace(0.0, 0.40, 25)
    
    # Simulation parameters
    np.random.seed(42)
    trials = 4000
    m_values = [2, 4, 8, 16]
    colors = ['#e74c3c', '#e67e22', '#2980b9', '#27ae60']

    fig, ax = plt.subplots(figsize=(9, 6))

    for m, col in zip(m_values, colors):
        ber_curve = []
        for p in noise_probs:
            # Simulate BPSK chip transmission with additive random noise
            # Station transmits bit d in {+1, -1}
            bits = np.random.choice([-1, 1], size=trials)
            # Repeated over m chips
            chips = np.repeat(bits, m)
            # Channel noise perturbation: with prob p, chip perturbed by +-1
            noise_mask = np.random.rand(len(chips)) < p
            noise_val = np.random.choice([-1, 1], size=len(chips)) * noise_mask
            received_chips = chips + noise_val
            
            # Matched filter / despreading: sum over m chips
            despread = np.sum(received_chips.reshape((trials, m)), axis=1) / m
            # Decision rule
            dec_bits = np.where(despread >= 0, 1, -1)
            ber = np.mean(dec_bits != bits)
            ber_curve.append(ber)

        ax.plot(noise_probs * 100, np.array(ber_curve) * 100, marker='o', linewidth=2, 
                color=col, label=f'Walsh Dimension M = {m} (Gp = {10*np.log10(m):.1f} dB)')

    ax.set_xlabel('Channel Chip Noise Perturbation Probability (%)', fontweight='bold')
    ax.set_ylabel('Bit Error Rate (BER) (%)', fontweight='bold')
    ax.set_title('CDMA Noise Immunity & Processing Gain ($G_p = 10\\log_{10}(M)$)\nBER vs. Channel Noise for Varying Spreading Dimensions', fontweight='bold')
    ax.grid(True, linestyle='--', alpha=0.7)
    ax.legend(frameon=True, facecolor='white', framealpha=0.9)
    plt.tight_layout()
    plt.savefig('cdma_ber_vs_noise.png', dpi=300)
    plt.close()
    print("Generated cdma_ber_vs_noise.png")

# ==============================================================================
# PLOT 3: Signal Superposition Waveform (Time Domain)
# ==============================================================================
def plot_superposition_waveform():
    fig, axes = plt.subplots(5, 1, figsize=(11, 8), sharex=True)

    # 4 stations transmitting bits [1, 0, idle, 1]
    # W0 = [+1, +1, +1, +1] -> s0 = [+1, +1, +1, +1]
    # W1 = [+1, -1, +1, -1] -> s1 = [-1, +1, -1, +1]
    # W2 = [+1, +1, -1, -1] -> s2 = [ 0,  0,  0,  0]
    # W3 = [+1, -1, -1, +1] -> s3 = [+1, -1, -1, +1]
    # C  = [+1, +1, -1, +3]

    signals = [
        ("Station 0: d0 = +1 (Bit 1) x W0", [1, 1, 1, 1], '#2980b9'),
        ("Station 1: d1 = -1 (Bit 0) x W1", [-1, 1, -1, 1], '#e67e22'),
        ("Station 2: d2 =  0 (Idle)  x W2", [0, 0, 0, 0], '#7f8c8d'),
        ("Station 3: d3 = +1 (Bit 1) x W3", [1, -1, -1, 1], '#8e44ad'),
        ("Common Channel: Superposition C = sum(si)", [1, 1, -1, 3], '#27ae60')
    ]

    t = np.linspace(0, 4, 400)
    
    for ax, (title, chips, col) in zip(axes, signals):
        # Step function for chips
        y = np.repeat(chips, 100)
        ax.step(t, y, where='post', color=col, linewidth=2.2)
        ax.fill_between(t, y, step='post', color=col, alpha=0.18)
        ax.set_ylabel(title, fontsize=10, fontweight='bold', color=col)
        ax.set_ylim(-3.5, 4.5)
        ax.axhline(0, color='black', linestyle=':', alpha=0.5)
        ax.grid(True, linestyle='--', alpha=0.6)
        
        # Add value annotations
        for idx, chip in enumerate(chips):
            ax.text(idx + 0.5, chip + 0.45 if chip >= 0 else chip - 0.75, 
                    f'{chip:+d}' if chip != 0 else '0', 
                    ha='center', va='center', fontweight='bold', fontsize=10, color=col)

    axes[-1].set_xlabel('Chip Time Index ($k = 0, 1, 2, 3$)', fontweight='bold')
    axes[-1].set_xticks([0.5, 1.5, 2.5, 3.5])
    axes[-1].set_xticklabels(['Chip 0', 'Chip 1', 'Chip 2', 'Chip 3'])
    fig.suptitle('Time-Domain Direct Sequence Chip Modulation & Channel Superposition', fontsize=14, fontweight='bold', y=0.98)
    plt.tight_layout()
    plt.subplots_adjust(top=0.92)
    plt.savefig('cdma_superposition_waveform.png', dpi=300)
    plt.close()
    print("Generated cdma_superposition_waveform.png")

# ==============================================================================
# PLOT 4: Multi-Access Throughput Comparison (ALOHA vs CSMA vs CDMA)
# ==============================================================================
def plot_channel_capacity():
    stations = np.arange(1, 25)
    
    # ALOHA: S = G e^(-2G) where G = N * 0.1
    pure_aloha = stations * 0.1 * np.exp(-2 * stations * 0.1)
    slotted_aloha = stations * 0.1 * np.exp(-1 * stations * 0.1)
    # CSMA/CD: 1 / (1 + 2 a G) with a = 0.05
    a = 0.05
    csma_cd = 1.0 / (1.0 + 4.44 * a * (1 - np.exp(-stations * 0.1)))
    # CDMA: Throughput scales linearly up to M users, then soft capacity degradation
    cdma_m16 = np.minimum(stations / 16.0, 1.0) * np.exp(-0.02 * np.maximum(0, stations - 16))
    cdma_m32 = np.minimum(stations / 24.0, 1.0)

    fig, ax = plt.subplots(figsize=(9, 6))
    ax.plot(stations, cdma_m16 * 100, label='CDMA ($M = 16$ Walsh Spreading)', color='#27ae60', linewidth=2.5, marker='s')
    ax.plot(stations, csma_cd * 100, label='CSMA/CD (Ethernet with Backoff)', color='#2980b9', linewidth=2, marker='^')
    ax.plot(stations, slotted_aloha * 100, label='Slotted ALOHA (Max 36.8%)', color='#e67e22', linewidth=2, linestyle='--')
    ax.plot(stations, pure_aloha * 100, label='Pure ALOHA (Max 18.4%)', color='#e74c3c', linewidth=2, linestyle=':')

    ax.set_xlabel('Number of Contending Stations ($N$)', fontweight='bold')
    ax.set_ylabel('Effective Channel Utilization / Throughput (%)', fontweight='bold')
    ax.set_title('Multiple Access Protocol Efficiency vs. Contending Load ($N$)\nCDMA Simultaneous Access vs. Contention-Based MAC Protocols', fontweight='bold')
    ax.grid(True, linestyle='--', alpha=0.7)
    ax.legend(frameon=True, facecolor='white', framealpha=0.9)
    plt.tight_layout()
    plt.savefig('cdma_channel_capacity_comparison.png', dpi=300)
    plt.close()
    print("Generated cdma_channel_capacity_comparison.png")

if __name__ == '__main__':
    plot_orthogonality_matrix()
    plot_ber_vs_noise()
    plot_superposition_waveform()
    plot_channel_capacity()
    print("All plots generated successfully!")
