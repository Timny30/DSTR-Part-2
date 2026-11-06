#!/usr/bin/env bash
# Builds the program as assignment.exe. Run from the project folder.
set -e
g++ -std=c++17 -g -Wall -Wextra -Isrc src/*.cpp src/core/*.cpp src/menus/*.cpp -o assignment.exe
echo "Built assignment.exe"
