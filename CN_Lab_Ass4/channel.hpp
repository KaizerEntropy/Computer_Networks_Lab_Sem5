#pragma once

#include <vector>
#include <map>
#include <set>
#include <netinet/in.h>
#include "walsh.hpp"
#include "protocol.hpp"
#include "utils.hpp"

class CDMAChannel {
private:
    int port;
    int expected_stations;
    double noise_prob;
    WalshCode walsh;
    int sock_fd;

    // Track active transmitters in current transmission round
    std::map<int, std::vector<std::vector<int>>> station_buffers;
    std::set<struct sockaddr_in, AddrComparator> receivers;

    void processSuperposition();

public:
    CDMAChannel(int port = 8080, int num_stations = 4, double noise = 0.0);
    ~CDMAChannel();

    bool init();
    void run();
};
