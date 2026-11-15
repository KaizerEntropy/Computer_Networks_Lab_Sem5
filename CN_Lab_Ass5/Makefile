CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2

OBJS = pcap_parser.o packet_analyzer.o main.o
TARGET = packet_analyzer

all: $(TARGET) pcap plots

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

pcap:
	python3 pcap_generator.py

plots:
	python3 generate_plots.py

report: plots
	tectonic report.tex
	cp report.pdf CN_Ass5_Report.pdf

clean:
	rm -f *.o $(TARGET) network_capture.pcap fig*.png report.pdf CN_Ass5_Report.pdf

.PHONY: all clean pcap plots report
