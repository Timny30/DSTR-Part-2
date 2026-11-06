#pragma once
#include <string>

namespace DateTime {
    // Current time as YYYY-MM-DD HH:MM, or the fixed time if one has been set.
    std::string now();

    // Fixes the clock so that scripted test output is repeatable.
    void setFixedTime(const std::string& value);
}
