CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -pthread -O2

COMMON_OBJS = walsh.o frame.o utils.o

TARGETS = channel station sender receiver cdma_sim

all: $(TARGETS)

channel: channel.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ channel.o $(COMMON_OBJS)

station: station.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ station.o $(COMMON_OBJS)

sender: sender.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ sender.o $(COMMON_OBJS)

receiver: receiver.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ receiver.o $(COMMON_OBJS)

cdma_sim: cdma_sim.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ cdma_sim.o $(COMMON_OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGETS) received_station_*.txt received_file.txt
