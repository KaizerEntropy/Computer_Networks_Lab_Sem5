CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude -pthread

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

SRCS = $(SRC_DIR)/error_detection.cpp \
       $(SRC_DIR)/error_injection.cpp \
       $(SRC_DIR)/channel.cpp \
       $(SRC_DIR)/bootp.cpp \
       $(SRC_DIR)/dhcp.cpp \
       $(SRC_DIR)/telnet.cpp

OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

TARGETS = server client simulator

.PHONY: all clean run test plots help

all: $(TARGETS)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(INC_DIR)/%.hpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

server: server.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

client: client.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

simulator: simulator.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGETS) server_received_payload.txt matrix_benchmark.json efficiency_data.json plots/*.png

plots:
	python3 generate_plots.py

test: all
	bash run_all_tests.sh

help:
	@echo "Available make targets:"
	@echo "  all       : Build server, client, and simulator"
	@echo "  server    : Build multi-protocol server daemon"
	@echo "  client    : Build simulation client"
	@echo "  simulator : Build automated benchmark harness"
	@echo "  clean     : Clean build objects and generated outputs"
	@echo "  plots     : Generate high-resolution comparison plots"
	@echo "  test      : Execute full automated test suite"

