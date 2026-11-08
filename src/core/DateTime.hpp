#pragma once
#include <ctime>
#include <string>

namespace DateTime {

inline std::string fixedTime;

inline std::string now() {
    if (!fixedTime.empty()) return fixedTime;
    std::time_t current = std::time(nullptr);
    char buffer[17];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", std::localtime(&current));
    return std::string(buffer);
}

inline void setFixedTime(const std::string& value) { fixedTime = value; }

} 
