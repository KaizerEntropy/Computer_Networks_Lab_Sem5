#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include "utils.hpp"
#include "csma.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    srand(time(0) ^ getpid()); 
    
    int station_id = 1;
    int protocol = 1;
    int frames_to_send = 5;
    double p = 0.5;

    double prob_error = 0.0;
    double prob_loss = 0.0;
    
    while (true) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(8080);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

        if (argc >= 4) {
            station_id = stoi(argv[1]);
            protocol = stoi(argv[2]);
            p = stod(argv[3]);
            if (argc >= 5) frames_to_send = stoi(argv[4]);
        } else {
            cout << "\n=======================================\n";
            cout << "         CSMA STATION MENU\n";
            cout << "=======================================\n";
            cout << "Enter Station ID (e.g. 1, 2, 3...) [or -1 to exit]: ";
            if (!(cin >> station_id) || station_id == -1) {
                Message exit_msg = {MsgType::EXIT, -1, 0};
                sendto(sock, &exit_msg, sizeof(exit_msg), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                close(sock);
                break;
            }
            
            cout << "\nSelect CSMA Protocol:\n";
            cout << "1. 1-Persistent\n";
            cout << "2. Non-Persistent\n";
            cout << "3. p-Persistent\n";
            cout << "4. CSMA/CD\n";
            cout << "Choice: ";
            if (!(cin >> protocol)) { close(sock); break; }
            
            if (protocol == 3) {
                cout << "Enter Probability 'p' (0.0 - 1.0): ";
                if (!(cin >> p)) { close(sock); break; }
            }

            cout << "Enter number of frames to send: ";
            if (!(cin >> frames_to_send)) { close(sock); break; }
            
            cout << "Enter Probability of Bit Error (e.g. 0.1 for 10%): ";
            if (!(cin >> prob_error)) { close(sock); break; }
            
            cout << "Enter Probability of Packet Loss (e.g. 0.05 for 5%): ";
            if (!(cin >> prob_loss)) { close(sock); break; }
            cout << "---------------------------------------\n";
        }

        long long total_delay = 0;

        if (protocol == 1) {
            run_1_persistent(sock, server_addr, station_id, frames_to_send, total_delay, prob_error, prob_loss);
        } else if (protocol == 2) {
            run_non_persistent(sock, server_addr, station_id, frames_to_send, total_delay, prob_error, prob_loss);
        } else if (protocol == 3) {
            run_p_persistent(sock, server_addr, station_id, frames_to_send, p, total_delay, prob_error, prob_loss);
        } else if (protocol == 4) {
            run_csma_cd(sock, server_addr, station_id, frames_to_send, total_delay, prob_error, prob_loss);
        }
        
        cout << "STATS: Delay=" << total_delay << " \n";
        close(sock);
        
        if (argc >= 4) {
            break; // Exit if running from automated script
        }
    }
    return 0;
}
