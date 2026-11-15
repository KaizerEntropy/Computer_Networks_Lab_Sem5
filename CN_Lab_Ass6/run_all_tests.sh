#!/bin/bash
# ==============================================================================
# JADAVPUR UNIVERSITY - DEPARTMENT OF COMPUTER SCIENCE & ENGINEERING
# Computer Networks Laboratory - Assignment 6 Automated Test & Build Suite
# Author: Diptarko Bhattacharjee | Roll: 002410501013
# ==============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

mkdir -p logs plots

echo "================================================================================"
echo " 1. COMPILING C++ NETWORK PROTOCOL & TOPOLOGY SIMULATOR"
echo "================================================================================"
make clean
make all
echo "Compilation successful!"

echo "================================================================================"
echo " 2. RUNNING INDIVIDUAL EXPERIMENT SUITES"
echo "================================================================================"
for p in 1 2 3 4 5 6 7; do
    echo " -> Running Problem $p Simulation..."
    ./sim_network "$p" > "logs/problem_${p}.log"
done

echo " -> Running Full Integrated Suite..."
./sim_network all > "logs/full_simulation.log"
echo "Simulations completed! Logs written to logs/"

echo "================================================================================"
echo " 3. GENERATING TOPOLOGY & PROTOCOL PERFORMANCE DIAGRAMS"
echo "================================================================================"
python3 generate_plots.py
echo "Plots generated successfully!"

echo "================================================================================"
echo " 4. COMPILING ACADEMIC LAB REPORT (LaTeX)"
echo "================================================================================"
if command -v pdflatex &> /dev/null; then
    pdflatex -interaction=nonstopmode report.tex > /dev/null 2>&1 || true
    pdflatex -interaction=nonstopmode report.tex > /dev/null 2>&1 || true
    if [ -f report.pdf ]; then
        cp report.pdf CN_Ass6_Report.pdf
        echo "Report compiled successfully -> CN_Ass6_Report.pdf & report.pdf"
    else
        echo "pdflatex compilation finished (inspect report.log if warnings)."
    fi
else
    echo "pdflatex not found on PATH; skipping PDF compilation."
fi

echo "================================================================================"
echo " ASSIGNMENT 6 PIPELINE COMPLETED SUCCESSFULLY"
echo "================================================================================"

