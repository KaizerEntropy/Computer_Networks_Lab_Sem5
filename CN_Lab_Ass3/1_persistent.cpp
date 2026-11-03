#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <cstdlib>
#include "csma.hpp"
#include "utils.hpp"

using namespace std;

void run_1_persistent(int sock, struct sockaddr_in& server_addr, int station_id, int frames_to_send, long long &total_delay, double prob_error, double prob_loss) {
    cout << "\n[Station " << station_id << "] === STARTING 1-PERSISTENT CSMA ===" << endl;
    for (int i = 0; i < frames_to_send; ++i) {
        bool success = false;
        auto start_time = chrono::steady_clock::now();
        cout << "[Station " << station_id << "] Preparing to send Frame " << i << "..." << endl;
        while (!success) {
            Message sense_msg = {MsgType::SENSE, station_id, 0, 0, 0};
            sendto(sock, &sense_msg, sizeof(sense_msg), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
            
            Message reply;
            int n = recvfrom(sock, &reply, sizeof(reply), 0, NULL, NULL);
            if (n > 0 && reply.type == MsgType::SENSE_REPLY_IDLE) {
                cout << "[Station " << station_id << "] Sensed Channel: IDLE. Transmitting Frame " << i << " immediately!" << endl;
                Message trans_msg = {MsgType::TRANSMIT, station_id, i, prob_error, prob_loss};
                sendto(sock, &trans_msg, sizeof(trans_msg), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                
                while (true) {
                    recvfrom(sock, &reply, sizeof(reply), 0, NULL, NULL);
                    if (reply.type == MsgType::SUCCESS) {
                        cout << "[Station " << station_id << "] <SUCCESS> Frame " << i << " transmitted perfectly!" << endl;
                        auto end_time = chrono::steady_clock::now();
                        total_delay += chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
                        success = true;
                        break;
                    } else if (reply.type == MsgType::ERROR_LOST) {
                        cout << "[Station " << station_id << "] <!> Frame " << i << " was LOST in transit! (Packet dropped by network)" << endl;
                        cout << "[Station " << station_id << "] Retransmitting Frame " << i << " due to loss..." << endl;
                        break;
                    } else if (reply.type == MsgType::ERROR_CORRUPT) {
                        cout << "[Station " << station_id << "] <!> Frame " << i << " was CORRUPTED by noise! (FCS Hash Failed)" << endl;
                        cout << "[Station " << station_id << "] Retransmitting Frame " << i << " due to error..." << endl;
                        break;
                    } else if (reply.type == MsgType::COLLISION) {
                        int backoff = (rand() % 100) + 50; 
                        cout << "[Station " << station_id << "] <!> COLLISION DETECTED for Frame " << i << ". Backing off for " << backoff << "ms..." << endl;
                        this_thread::sleep_for(chrono::milliseconds(backoff));
                        break;
                    }
                }
            } else {
                // Busy, wait slightly and try again (1-persistent senses continuously)
                this_thread::sleep_for(chrono::milliseconds(5));
            }
        }
    }
    cout << "[Station " << station_id << "] All frames sent!" << endl;
}
