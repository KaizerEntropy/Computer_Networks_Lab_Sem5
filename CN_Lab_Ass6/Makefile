CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -pedantic -I./simulator

SRCS = simulator/arp_table.cpp \
       simulator/mac_table.cpp \
       simulator/routing_table.cpp \
       simulator/network.cpp \
       simulator/hub.cpp \
       simulator/switch.cpp \
       simulator/router.cpp \
       simulator/services.cpp \
       simulator/host.cpp \
       simulator/problems.cpp \
       simulator/main.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = sim_network

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf logs/*.log

run: all
	./$(TARGET) all

report:
	python3 generate_plots.py
	pdflatex -interaction=nonstopmode report.tex
	pdflatex -interaction=nonstopmode report.tex
	cp report.pdf CN_Ass6_Report.pdf

.PHONY: all clean run report

