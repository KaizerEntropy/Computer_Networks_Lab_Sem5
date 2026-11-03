# Assignment 4 — Code Division Multiple Access (CDMA) with Walsh Codes

**CSE/PC/B/S/314 Computer Networks Laboratory**  
Department of Computer Science & Engineering, Jadavpur University  

---

## 1. Project Overview

This project implements **Code Division Multiple Access (CDMA)** with **orthogonal Walsh-Hadamard codes** over network sockets and standalone simulation. It provides a modular C++17 implementation following the same sender-receiver and channel architecture as Assignments 1, 2, and 3.

Multiple independent stations simultaneously transmit data across a shared physical channel using direct sequence chip sequences. The channel performs **linear superposition** ($\mathbf{C} = \sum \mathbf{s}_i$). At the receivers, computing the inner product with each station's orthogonal Walsh code word isolates that station's data and cancels all interference from all other stations, enabling **100% bit-exact reconstruction**.

---

## 2. Mathematical Foundation & CDMA Theory

### 2.1 Walsh-Hadamard Matrix Construction
Given $n$ stations, the chip dimension $M$ is the smallest power of 2 such that $M \ge n$ ($M \in \{2, 4, 8, 16, \dots\}$).
The Walsh matrix is constructed recursively:
$$H_1 = [1], \quad H_{2k} = \begin{bmatrix} H_k & H_k \\ H_k & -H_k \end{bmatrix}$$

For $M = 4$:
$$H_4 = \begin{bmatrix}
+1 & +1 & +1 & +1 \\
+1 & -1 & +1 & -1 \\
+1 & +1 & -1 & -1 \\
+1 & -1 & -1 & +1
\end{bmatrix}$$
Each row $i$ serves as the unique spreading code $W_i$ for Station $i$.

### 2.2 Orthogonality Property
For any two code words $W_i$ and $W_j$:
$$\frac{1}{M} (W_i \cdot W_j) = \frac{1}{M} \sum_{k=0}^{M-1} W_i[k] W_j[k] = \begin{cases} 1, & \text{if } i = j \\ 0, & \text{if } i \neq j \end{cases}$$

### 2.3 Modulation (Sender Encoding)
Each data bit $d_i$ from station $i$ is mapped bipolarly:
- Bit $1 \implies +1$
- Bit $0 \implies -1$
- Silent/Idle $\implies 0$

The transmitted chip vector for bit $d_i$ is:
$$\mathbf{s}_i = d_i \times W_i = [d_i W_i[0], d_i W_i[1], \dots, d_i W_i[M-1]]$$

### 2.4 Linear Superposition (Common Channel)
When $n$ stations transmit simultaneously, the composite channel signal $\mathbf{C}$ is:
$$\mathbf{C} = \sum_{j=0}^{n-1} \mathbf{s}_j = \sum_{j=0}^{n-1} d_j W_j$$

### 2.5 Demodulation & Perfect Reconstruction (Receiver)
The receiver recovers Station $i$'s bit by computing the inner product of the composite signal with $W_i$:
$$\mathbf{C} \cdot W_i = \left(\sum_{j=0}^{n-1} d_j W_j \right) \cdot W_i = d_i (W_i \cdot W_i) + \sum_{j \neq i} d_j (W_j \cdot W_i) = d_i \cdot M + 0 = M \cdot d_i$$

Normalized decision metric:
$$\hat{d}_i = \frac{\mathbf{C} \cdot W_i}{M}$$
- If $\hat{d}_i > 0.5 \implies$ Bit **1**
- If $\hat{d}_i < -0.5 \implies$ Bit **0**
- If $|\hat{d}_i| \le 0.5 \implies$ Station was **Silent / Idle**

---

## 3. Modular Architecture

| Component | Source Files | Description |
|---|---|---|
| **Walsh Engine** | [`walsh.hpp`](file:///home/diptarko-bhattacharjee/Desktop/Diptarko/Semester_Material/Sem_5/CN/CN_Lab/CN_Lab_Ass4/walsh.hpp), [`walsh.cpp`](file:///home/diptarko-bhattacharjee/Desktop/Diptarko/Semester_Material/Sem_5/CN/CN_Lab/CN_Lab_Ass4/walsh.cpp) | Walsh matrix generation, bit/byte modulation, demodulation, inner products, superposition, noise injection. |
| **Frame Protocol** | [`frame.hpp`](file:///home/diptarko-bhattacharjee/Desktop/Diptarko/Semester_Material/Sem_5/CN/CN_Lab/CN_Lab_Ass4/frame.hpp), [`frame.cpp`](file:///home/diptarko-bhattacharjee/Desktop/Diptarko/Semester_Material/Sem_5/CN/CN_Lab/CN_Lab_Ass4/frame.cpp) | Structured CDMA frame (`CDMAFrame`) with source ID, sequence number, payload length, payload, and CRC-32/Checksum FCS. |
| **Network Protocol**| [`protocol.hpp`](file:///home/diptarko-bhattacharjee/Desktop/Diptarko/Semester_Material/Sem_5/CN/CN_Lab/CN_Lab_Ass4/protocol.hpp) | Low-level UDP datagram header and chip serialization. |
| **Utilities** | [`utils.hpp`](file:///home/diptarko-bhattacharjee/Desktop/Diptarko/Semester_Material/Sem_5/CN/CN_Lab/CN_Lab_Ass4/utils.hpp), [`utils.cpp`](file:///home/diptarko-bhattacharjee/Desktop/Diptarko/Semester_Material/Sem_5/CN/CN_Lab/CN_Lab_Ass4/utils.cpp) | ANSI color output, chip formatters, file diff verification, 16-bit Checksum & IEEE 802.3 CRC-32 algorithms. |
| **Channel Coordinator** | [`channel.hpp`](file:///home/diptarko-bhattacharjee/Desktop/Diptarko/Semester_Material/Sem_5/CN/CN_Lab/CN_Lab_Ass4/channel.hpp), [`channel.cpp`](file:///home/diptarko-bhattacharjee/Desktop/Diptarko/Semester_Material/Sem_5/CN/CN_Lab/CN_Lab_Ass4/channel.cpp) | UDP server representing the shared physical medium; computes linear superposition and broadcasts composite chips. |
| **Sender** | [`sender.cpp`](file:///home/diptarko-bhattacharjee/Desktop/Diptarko/Semester_Material/Sem_5/CN/CN_Lab/CN_Lab_Ass4/sender.cpp) | Client transmitter that modulates input files or text strings using Station $i$'s Walsh code word and transmits to channel. |
| **Receiver** | [`receiver.cpp`](file:///home/diptarko-bhattacharjee/Desktop/Diptarko/Semester_Material/Sem_5/CN/CN_Lab/CN_Lab_Ass4/receiver.cpp) | Client receiver that demultiplexes the composite stream for a specific station or all $N$ stations simultaneously, validating CRC-32. |
| **Autonomous Station** | [`station.cpp`](file:///home/diptarko-bhattacharjee/Desktop/Diptarko/Semester_Material/Sem_5/CN/CN_Lab/CN_Lab_Ass4/station.cpp) | Unified peer station that can both transmit encoded data and receive/decode from the channel. |
| **Simulator & Bench** | [`cdma_sim.cpp`](file:///home/diptarko-bhattacharjee/Desktop/Diptarko/Semester_Material/Sem_5/CN/CN_Lab/CN_Lab_Ass4/cdma_sim.cpp) | Interactive and automated test suite covering mathematical proofs, noise benchmarks, and silent stations. |

---

## 4. How to Build

Compile all targets with `make`:

```bash
cd /home/diptarko-bhattacharjee/Desktop/Diptarko/Semester_Material/Sem_5/CN/CN_Lab/CN_Lab_Ass4
make clean && make
```

This compiles:
- `channel`
- `receiver`
- `sender`
- `station`
- `cdma_sim`

---

## 5. Execution Modes

### Mode 1: Automated All-in-One Test Harness (Recommended)

Run the full automated test suite that executes all mathematical proofs, multi-station socket transmissions, and bit-exact diff verification:

```bash
./run_all_tests.sh
```

---

### Mode 2: Interactive CDMA Simulator

Run the standalone interactive simulator:

```bash
./cdma_sim
```

**Menu Options:**
1. **Bit-by-Bit Textbook CDMA Trace**: Step-by-step vector addition and inner-product arithmetic.
2. **Walsh-Hadamard Orthogonality Verification**: Proof for $N = 2, 4, 8, 16$.
3. **Multi-Station Concurrent Text Transfer**: Transmit 4 concurrent streams and verify CRC-32 integrity.
4. **Channel Noise Robustness & Processing Gain**: Tests bit error rate against $0\% - 30\%$ chip perturbation.
5. **Asymmetric Transmissions & Silent Stations**: Proves that inactive stations cause zero interference.
6. **Run ALL Tests Sequentially**.

*(To run the simulator non-interactively in batch mode: `./cdma_sim --auto`)*

---

### Mode 3: Distributed Multi-Process Socket Execution (Separate Terminals)

To run CDMA across separate terminals (or separate machines over the local network):

#### Terminal 1 — Channel Coordinator
```bash
./channel 8080 4 0.0
```
*Arguments: `./channel [port=8080] [num_stations=4] [noise_probability=0.0]`*

#### Terminal 2 — Receiver
```bash
./receiver 8080 -1 127.0.0.1 4
```
*Arguments: `./receiver [channel_port=8080] [station_id=-1 for ALL] [channel_ip=127.0.0.1] [num_stations=4]`*  
*(The receiver registers with the channel and waits for the broadcast).*

#### Terminal 3 — Sender 0
```bash
./sender 127.0.0.1 8080 0 sample_st0.txt 4
```

#### Terminal 4 — Sender 1
```bash
./sender 127.0.0.1 8080 1 sample_st1.txt 4
```

#### Terminal 5 — Sender 2
```bash
./sender 127.0.0.1 8080 2 sample_st2.txt 4
```

#### Terminal 6 — Sender 3
```bash
./sender 127.0.0.1 8080 3 sample_st3.txt 4
```

Once the senders transmit, the Channel superimposes all chip streams, prints the real-time chip addition breakdown, and broadcasts the composite signal. The Receiver decodes each stream, validates the CRC-32 checksum, saves `received_station_<id>.txt`, and performs a 100% diff comparison!

---

### Mode 4: Peer Station Execution
Alternatively, run peer stations directly:
```bash
./station 127.0.0.1 8080 <station_id> <input_file_or_string> 4
```
Each station transmits its own data and decodes transmissions from all other stations on the channel.
