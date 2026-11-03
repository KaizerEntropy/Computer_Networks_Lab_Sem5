#!/bin/bash
# visual_demo.sh - Demonstrates CSMA/CD visually in the terminal

echo "========================================="
echo "  Starting CSMA Visual Demonstration"
echo "========================================="

# Start the channel server in the background
./channel &
CHANNEL_PID=$!

sleep 0.5

# Start 3 stations concurrently using CSMA/CD (Protocol 4) 
# each sending 3 frames
./station 1 4 0.5 3 &
./station 2 4 0.5 3 &
./station 3 4 0.5 3 &

# Wait for all background jobs to finish
wait

echo "========================================="
echo "  Demonstration Finished!"
echo "========================================="
