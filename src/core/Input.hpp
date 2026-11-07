#pragma once
#include <cctype>
#include <iostream>
#include <string>
#include "core/Validation.hpp"

// All console input goes through here. Whole lines are read, so a multi-word
// answer can't spill into the next prompt.
//
// This file is header-only. Everything here is marked inline, so all .cpp files
// share one definition. That matters for inputEnded: every menu must see the same
// flag, or a menu could keep prompting after the input has run out.
namespace Input {

inline bool inputEnded = false;

// Returns the line with surrounding spaces removed.
inline std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        inputEnded = true;
        return "";
    }
    return Validation::trim(line);
}

// Repeats until the answer is a number within the range.
inline int readChoice(const std::string& prompt, int minimum, int maximum) {
    while (true) {
        std::string line = readLine(prompt);
        if (inputEnded) return minimum;  // every menu uses its minimum as "back"

        bool isNumber = !line.empty() && line.size() <= 6;
        for (char c : line) {
            if (!std::isdigit(static_cast<unsigned char>(c))) isNumber = false;
        }
        if (isNumber) {
            int value = std::stoi(line);
            if (value >= minimum && value <= maximum) return value;
        }
        std::cout << "\n[ERROR] Enter a number from " << minimum << " to " << maximum << ".\n";
    }
}

inline bool readYesNo(const std::string& prompt) {
    while (true) {
        std::string line = Validation::toUpper(readLine(prompt + " (y/n): "));
        if (inputEnded) return false;
        if (line == "Y" || line == "YES") return true;
        if (line == "N" || line == "NO") return false;
        std::cout << "\n[ERROR] Enter y or n.\n";
    }
}

// True once input has run out.
inline bool atEnd() { return inputEnded; }

}  // namespace Input
