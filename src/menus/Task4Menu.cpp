#include "menus/Menus.hpp"
#include "core/Input.hpp"
#include "core/Validation.hpp"
#include <iostream>

namespace {

void showRecent(ParcelSystem& system) {
    printHeading("Recent activity, oldest first");
    if (system.activities().isEmpty()) {
        printInfo("There is no activity history yet.");
        return;
    }
    system.activities().forEachRecent(15, [](const Activity& activity) { printActivityRow(activity); });
}

void showParcelTrace(ParcelSystem& system) {
    std::string parcelId = Validation::toUpper(Input::readLine("Parcel ID: "));
    if (!Validation::isParcelId(parcelId)) {
        printResult(Result::fail("Invalid Parcel ID. Use P0001 to P9999."));
        return;
    }
    printHeading("Activity trace for " + parcelId);
    int count = 0;
    std::string latest;
    system.activities().forEachForParcel(parcelId, [&](const Activity& activity) {
        printActivityRow(activity);
        latest = activity.id;
        count++;
    });
    if (count == 0) {
        printInfo("Parcel " + parcelId + " has no activity history.");
        return;
    }
    printInfo("Most recent activity: " + latest + ".");
    Parcel* parcel = system.parcels().find(parcelId);
    printInfo(parcel == nullptr ? "This parcel's record no longer exists."
                                : "Current status: " + toString(parcel->status) + ".");
}

void undoActivity(ParcelSystem& system) {
    Activity* top = system.activities().peekUndoable();
    if (top == nullptr) {
        printResult(Result::fail("There is nothing to undo."));
        return;
    }
    printInfo("Most recent reversible activity:");
    printActivityRow(*top);
    std::string activityId = Validation::toUpper(Input::readLine("Activity ID to undo: "));
    if (Input::readYesNo("Reverse " + activityId + "?")) {
        printResult(system.undo(activityId));
    } else {
        printInfo("Undo cancelled.");
    }
}

}  

void runTask4Menu(ParcelSystem& system) {
    while (!Input::atEnd()) {
        printHeading("Task 4: Parcel Activity and Records");
        std::cout << "1. Show recent activity\n2. Show a parcel's activity trace\n3. Undo recent activity\n0. Back\n";
        switch (Input::readChoice("Choice: ", 0, 3)) {
            case 1: showRecent(system); break;
            case 2: showParcelTrace(system); break;
            case 3: undoActivity(system); break;
            default: return;
        }
    }
}
