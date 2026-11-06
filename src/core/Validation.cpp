#include "core/Validation.hpp"
#include <cctype>

namespace {
    bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }
    bool isAlpha(char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0; }

    bool isAlphanumericOfLength(const std::string& text, size_t minLength, size_t maxLength) {
        if (text.size() < minLength || text.size() > maxLength) return false;
        for (char c : text) {
            if (!isDigit(c) && !isAlpha(c)) return false;
        }
        return true;
    }

    int digitsToInt(const std::string& text, size_t start, size_t count) {
        int value = 0;
        for (size_t i = start; i < start + count; i++) value = value * 10 + (text[i] - '0');
        return value;
    }
}

namespace Validation {

std::string trim(const std::string& text) {
    size_t start = 0;
    size_t end = text.size();
    while (start < end && std::isspace(static_cast<unsigned char>(text[start]))) start++;
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) end--;
    return text.substr(start, end - start);
}

std::string toUpper(const std::string& text) {
    std::string result = text;
    for (char& c : result) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return result;
}

bool isParcelId(const std::string& text) {
    if (text.size() != 5 || text[0] != 'P') return false;
    for (size_t i = 1; i < 5; i++) {
        if (!isDigit(text[i])) return false;
    }
    return text != "P0000";
}

bool isRecipientId(const std::string& text) { return isAlphanumericOfLength(text, 3, 12); }

bool isStaffId(const std::string& text) { return isAlphanumericOfLength(text, 2, 12); }

bool isPersonName(const std::string& text) {
    bool hasLetter = false;
    for (char c : text) {
        if (isAlpha(c)) hasLetter = true;
        else if (c != ' ' && c != '-' && c != '\'' && c != '.') return false;
    }
    return hasLetter;
}

bool isDateTime(const std::string& text) {
    if (text.size() != 16) return false;
    for (size_t i = 0; i < 16; i++) {
        if (i == 4 || i == 7) {
            if (text[i] != '-') return false;
        } else if (i == 10) {
            if (text[i] != ' ') return false;
        } else if (i == 13) {
            if (text[i] != ':') return false;
        } else if (!isDigit(text[i])) {
            return false;
        }
    }
    int month = digitsToInt(text, 5, 2);
    int day = digitsToInt(text, 8, 2);
    int hour = digitsToInt(text, 11, 2);
    int minute = digitsToInt(text, 14, 2);
    return month >= 1 && month <= 12 && day >= 1 && day <= 31 && hour <= 23 && minute <= 59;
}

std::string parcelFieldError(const Parcel& parcel) {
    if (parcel.recipientId.empty()) return "Recipient ID is required.";
    if (!isRecipientId(parcel.recipientId)) return "Recipient ID must be 3 to 12 letters or digits.";
    if (parcel.recipientName.empty()) return "Recipient name is required.";
    if (!isPersonName(parcel.recipientName)) return "Recipient name may only contain letters, spaces, - ' and .";
    if (parcel.deliveryCompany.empty()) return "Delivery company is required.";
    return "";
}

std::string formatId(const std::string& prefix, int number, int width) {
    std::string digits = std::to_string(number);
    while (static_cast<int>(digits.size()) < width) digits = "0" + digits;
    return prefix + digits;
}

}
