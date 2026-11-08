#pragma once
#include <string>

struct Result {
    bool success;
    std::string message;

    static Result ok(const std::string& text) { return Result{true, text}; }
    static Result fail(const std::string& text) { return Result{false, text}; }
};
