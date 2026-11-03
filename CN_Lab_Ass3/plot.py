import subprocess
import time
import re

def run_experiment(protocol, n_stations, p=0.5, frames=10):
    channel_proc = subprocess.Popen(['./channel'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    time.sleep(0.1) 
    station_procs = []
    for i in range(1, n_stations + 1):
        proc = subprocess.Popen(['./station', str(i), str(protocol), str(p), str(frames)], stdout=subprocess.PIPE, text=True)
        station_procs.append(proc)

    channel_out, _ = channel_proc.communicate()
    total_delay = 0
    for proc in station_procs:
        out, _ = proc.communicate()
        match = re.search(r'STATS: Delay=(\d+)', out)
        if match: total_delay += int(match.group(1))

    success, collisions, sim_time = 0, 0, 0
    match = re.search(r'STATS: Success=(\d+) Collisions=(\d+) SimTime=(\d+)', channel_out)
    if match:
        success, collisions, sim_time = int(match.group(1)), int(match.group(2)), int(match.group(3))
        
    avg_delay = (total_delay / (n_stations * frames)) if (n_stations * frames) > 0 else 0
    throughput = (success / sim_time) * 1000 if sim_time > 0 else 0 
    return collisions, avg_delay, throughput

def exp1_p_persistent():
    print("=== Exp 1: p-persistent varying p ===")
    p_values = [0.1, 0.3, 0.5, 0.7, 0.9]
    thrus = []
    for p in p_values:
        c, d, t = run_experiment(3, 10, p)
        thrus.append(t)
        print(f"p={p}: Collisions={c}, Delay={d:.2f}ms, Throughput={t:.2f}frames/s")
    best_p = p_values[thrus.index(max(thrus))]
    print(f"Best p = {best_p}\n")
    return best_p

def exp2_all_schemes(best_p):
    print(f"=== Exp 2: All schemes varying N (using best p={best_p}) ===")
    N_values = [2, 5, 8, 12, 16]
    protocols = {1: '1-Pers', 2: 'Non-Pers', 3: f'p-Pers(p={best_p})', 4: 'CSMA/CD'}
    
    for proto in protocols:
        print(f"\nProtocol: {protocols[proto]}")
        for N in N_values:
            c, d, t = run_experiment(proto, N, p=best_p if proto==3 else 0.5)
            print(f"N={N}: Col={c}, Del={d:.2f}ms, Thru={t:.2f}frames/s")

if __name__ == "__main__":
    best_p = exp1_p_persistent()
    exp2_all_schemes(best_p)

