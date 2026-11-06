#pragma once
#include <string>

// All console input goes through here. Whole lines are read, so a multi-word
// answer can't spill into the next prompt.
namespace Input {
    std::string readLine(const std::string& prompt);    // trimmed
    int readChoice(const std::string& prompt, int minimum, int maximum);  // repeats until valid
    bool readYesNo(const std::string& prompt);
    bool atEnd();                                       // true once input has run out
}
