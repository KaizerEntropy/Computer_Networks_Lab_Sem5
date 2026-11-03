#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <set>
#include <chrono>
#include <vector>
#include "utils.hpp"

using namespace std;

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    // Allow address reuse to avoid bind errors if restarted quickly
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Bind failed\n";
        return 1;
    }

    cout << "========================================\n";
    cout << "   CSMA CHANNEL COORDINATOR (UDP 8080)  \n";
    cout << "========================================\n";
    cout << "[Channel] Simulating shared medium...\n\n";

    bool is_busy = false;
    bool cd_active = false;
    bool collision_occurred = false;
    double current_prob_error = 0.0;
    double current_prob_loss = 0.0;
    set<struct sockaddr_in, AddrComparator> transmitters; 
    auto trans_start_time = chrono::steady_clock::now();
    const int TRANS_TIME_MS = 2000; // 2 seconds so humans can trigger collisions

    struct timeval tv;
    tv.tv_sec = 0; // 0 seconds timeout
    tv.tv_usec = 5000; // 5ms timeout for fast polling
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    long long total_success = 0;
    long long total_collisions = 0;
    long long total_errors = 0;
    auto sim_start_time = chrono::steady_clock::now();
    auto last_event_time = sim_start_time;
    bool started = false;

    while (true) {
        if (is_busy) {
            auto now = chrono::steady_clock::now();
            if (chrono::duration_cast<chrono::milliseconds>(now - trans_start_time).count() >= TRANS_TIME_MS) {
                Message reply;
                if (collision_occurred) {
                    reply.type = MsgType::COLLISION;
                    total_collisions++;
                    cout << "[Channel] Window Over -> RESULT: COLLISION (" << transmitters.size() << " stations collided)" << endl;
                } else {
                    double rand_val = (double)rand() / RAND_MAX;
                    if (rand_val < current_prob_loss) {
                        reply.type = MsgType::ERROR_LOST;
                        total_errors++;
                        cout << "[Channel] Window Over -> RESULT: FRAME LOST DUE TO NOISE!" << endl;
                    } else if (rand_val < (current_prob_loss + current_prob_error)) {
                        reply.type = MsgType::ERROR_CORRUPT;
                        total_errors++;
                        cout << "[Channel] Window Over -> RESULT: FRAME CORRUPTED BY NOISE!" << endl;
                    } else {
                        reply.type = MsgType::SUCCESS;
                        total_success++;
                        cout << "[Channel] Window Over -> RESULT: SUCCESSFUL TRANSMISSION!" << endl;
                    }
                }
                
                for (auto& addr : transmitters) {
                    sendto(sock, &reply, sizeof(reply), 0, (struct sockaddr*)&addr, sizeof(addr));
                }
                
                is_busy = false;
                cd_active = false;
                collision_occurred = false;
                current_prob_error = 0.0;
                current_prob_loss = 0.0;
                transmitters.clear();
                last_event_time = chrono::steady_clock::now();
            }
        }

        Message msg;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int n = recvfrom(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&client_addr, &client_len);
        
        if (n > 0) {
            if (!started) {
                started = true;
                sim_start_time = chrono::steady_clock::now();
            }
            last_event_time = chrono::steady_clock::now();

            if (msg.type == MsgType::SENSE) {
                Message reply;
                reply.type = is_busy ? MsgType::SENSE_REPLY_BUSY : MsgType::SENSE_REPLY_IDLE;
                sendto(sock, &reply, sizeof(reply), 0, (struct sockaddr*)&client_addr, client_len);
            } else if (msg.type == MsgType::TRANSMIT || msg.type == MsgType::TRANSMIT_CD) {
                if (!is_busy) {
                    cout << "[Channel] Station " << msg.station_id << " began transmitting Frame " << msg.data << "! Channel is now BUSY." << endl;
                    is_busy = true;
                    collision_occurred = false;
                    current_prob_error = msg.prob_error;
                    current_prob_loss = msg.prob_loss;
                    if (msg.type == MsgType::TRANSMIT_CD) cd_active = true;
                    transmitters.insert(client_addr);
                    trans_start_time = chrono::steady_clock::now();
                } else {
                    cout << "[Channel] OVERLAP DETECTED! Station " << msg.station_id << " transmitted while channel was busy!" << endl;
                    collision_occurred = true;
                    transmitters.insert(client_addr);
                    
                    if (cd_active || msg.type == MsgType::TRANSMIT_CD) {
                        cout << "[Channel] CSMA/CD active! Aborting transmission window immediately!" << endl;
                        Message reply;
                        reply.type = MsgType::COLLISION;
                        total_collisions++;
                        cout << "[Channel] RESULT: COLLISION (" << transmitters.size() << " stations collided)" << endl;
                        for (auto& addr : transmitters) {
                            sendto(sock, &reply, sizeof(reply), 0, (struct sockaddr*)&addr, sizeof(addr));
                        }
                        is_busy = false;
                        cd_active = false;
                        collision_occurred = false;
                        transmitters.clear();
                        last_event_time = chrono::steady_clock::now();
                    }
                }
            } else if (msg.type == MsgType::EXIT) {
                cout << "[Channel] Received EXIT signal from Station " << msg.station_id << ". Shutting down channel..." << endl;
                break;
            }
        } else {
            // Keep running infinitely. The channel represents the physical wire.
            // It will not exit until Ctrl+C is pressed.
            if (started && !is_busy) {
                // We just keep spinning
            }
        }
    }

    long long total_sim_time = chrono::duration_cast<chrono::milliseconds>(last_event_time - sim_start_time).count();
    cout << "STATS: Success=" << total_success 
         << " Collisions=" << total_collisions 
         << " Errors/Lost=" << total_errors
         << " SimTime=" << total_sim_time << "\n";
         
    return 0;
}
