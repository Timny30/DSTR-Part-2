@echo off
rem Builds the program as assignment.exe. Run from the project folder.
g++ -std=c++17 -g -Wall -Wextra -Iinclude src/*.cpp src/core/*.cpp src/modules/*.cpp src/menus/*.cpp -o assignment.exe
if errorlevel 1 exit /b 1
echo Built assignment.exe
