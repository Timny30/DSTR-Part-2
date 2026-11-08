#include <iostream>
#include <string>
#include "ParcelSystem.hpp"
#include "core/DateTime.hpp"
#include "core/Input.hpp"
#include "core/Validation.hpp"
#include "menus/Menus.hpp"

int main(int argc, char* argv[]) {
    bool loadDemo = true;
    for (int i = 1; i < argc; i++) {
        std::string flag = argv[i];
        if (flag == "--no-demo") {
            loadDemo = false;
        } else if (flag == "--fixed-time" && i + 1 < argc) {
            DateTime::setFixedTime(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << flag << "\n";
            return 1;
        }
    }

    std::cout << "=== Smart Campus Parcel Collection and Delivery Management System ===\n";

    ParcelSystem system;
    if (loadDemo) {
        Result loaded = system.loadDemoData("data");
        system.forEachLoadWarning([](const std::string& warning) { std::cout << "[WARNING] " << warning << "\n"; });
        std::cout << "[INFO] " << loaded.message << "\n";
    }

    std::string staffId;
    while (!Input::atEnd() && !Validation::isStaffId(staffId)) {
        staffId = Validation::toUpper(Input::readLine("Staff ID: "));
        if (!Input::atEnd() && !Validation::isStaffId(staffId)) {
            std::cout << "\n[ERROR] Staff ID must be 2 to 12 letters or digits.\n";
        }
    }
    system.setStaffId(staffId);

    while (!Input::atEnd()) {
        printHeading("Main Menu");
        std::cout << "1. Task 1: Parcel intake and processing\n"
                  << "2. Task 2: Parcel service and collection\n"
                  << "3. Task 3: Counters and delivery resources\n"
                  << "4. Task 4: Parcel activity and records\n"
                  << "5. Task 5: Parcel organisation\n"
                  << "0. Exit\n";
        switch (Input::readChoice("Choice: ", 0, 5)) {
            case 1: runTask1Menu(system); break;
            case 2: runTask2Menu(system); break;
            case 3: runTask3Menu(system); break;
            case 4: runTask4Menu(system); break;
            case 5: runTask5Menu(system); break;
            default:
                std::cout << "\nGoodbye.\n";
                return 0;
        }
    }
    return 0;
}
