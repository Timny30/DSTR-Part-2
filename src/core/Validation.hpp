#pragma once
#include <string>
#include "core/Models.hpp"

namespace Validation {
    std::string trim(const std::string& text);
    std::string toUpper(const std::string& text);

    bool isParcelId(const std::string& text);       // P0001 to P9999
    bool isRecipientId(const std::string& text);    // 3 to 12 letters or digits
    bool isStaffId(const std::string& text);        // 2 to 12 letters or digits
    bool isPersonName(const std::string& text);     // letters, spaces, - ' .
    bool isDateTime(const std::string& text);       // YYYY-MM-DD HH:MM

    // Returns an empty string if valid, otherwise the reason it isn't.
    std::string parcelFieldError(const Parcel& parcel);

    // formatId("REQ", 7, 4) returns "REQ0007".
    std::string formatId(const std::string& prefix, int number, int width);
}
