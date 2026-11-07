#pragma once
#include <ctime>
#include <string>

// This file is header-only. Everything here is marked inline, so all .cpp files
// share one definition. That matters for fixedTime: without inline, each .cpp
// file could end up with its own separate copy of the clock setting.
namespace DateTime {

inline std::string fixedTime;

// Current time as YYYY-MM-DD HH:MM, or the fixed time if one has been set.
inline std::string now() {
    if (!fixedTime.empty()) return fixedTime;
    std::time_t current = std::time(nullptr);
    char buffer[17];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", std::localtime(&current));
    return std::string(buffer);
}

// Fixes the clock, so that the program's output is repeatable.
inline void setFixedTime(const std::string& value) { fixedTime = value; }

}  // namespace DateTime
