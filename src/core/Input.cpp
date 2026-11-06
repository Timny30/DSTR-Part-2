#include "core/Input.hpp"
#include "core/Validation.hpp"
#include <cctype>
#include <iostream>

namespace {
    bool inputEnded = false;
}

namespace Input {

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        inputEnded = true;
        return "";
    }
    return Validation::trim(line);
}

int readChoice(const std::string& prompt, int minimum, int maximum) {
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

bool readYesNo(const std::string& prompt) {
    while (true) {
        std::string line = Validation::toUpper(readLine(prompt + " (y/n): "));
        if (inputEnded) return false;
        if (line == "Y" || line == "YES") return true;
        if (line == "N" || line == "NO") return false;
        std::cout << "\n[ERROR] Enter y or n.\n";
    }
}

bool atEnd() { return inputEnded; }

}
