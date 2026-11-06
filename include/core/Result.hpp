#pragma once
#include <string>

// Outcome of a module operation. Modules return this instead of printing,
// so the rules can be tested and all console output stays in the menus.
struct Result {
    bool success;
    std::string message;

    static Result ok(const std::string& text) { return Result{true, text}; }
    static Result fail(const std::string& text) { return Result{false, text}; }
};
