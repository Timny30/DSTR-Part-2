#include "menus/Menus.hpp"
#include "core/Input.hpp"
#include "core/Validation.hpp"
#include <iostream>

namespace {

std::string readParcelId() { return Validation::toUpper(Input::readLine("Parcel ID: ")); }

void registerParcel(ParcelSystem& system) {
    Parcel parcel;
    if (!promptNewParcelDetails(parcel)) {
        printInfo("Registration cancelled.");
        return;
    }
    std::string newId;
    Result result = system.parcels().registerNew(parcel, system.staffId(), newId);
    printResult(result);
    if (result.success) printParcelRow(*system.parcels().find(newId));
}

void listPending(ParcelSystem& system) {
    printHeading("Parcels waiting for a route, next first");
    int count = 0;
    system.parcels().forEachPending([&](const Parcel& parcel) {
        printParcelRow(parcel);
        count++;
    });
    if (count == 0) printInfo("There are no parcels waiting for a route.");
}

// Task 1 only saves the route. The parcel moves on when Task 2 creates its request.
void promptAndRoute(ParcelSystem& system, const std::string& parcelId) {
    std::cout << "Route:\n  1. Recipient collection\n  2. Campus delivery\n  3. Special handling\n"
              << "  4. Return to sender\n  5. Hold for investigation\n";
    int choice = Input::readChoice("Choice (0 to decide later): ", 0, 5);
    if (choice == 0) {
        printInfo("Route for parcel " + parcelId + " wasn't changed.");
        return;
    }
    printResult(system.parcels().routeParcel(parcelId, static_cast<ParcelRoute>(choice), system.staffId()));
}

void routeNext(ParcelSystem& system) {
    std::string parcelId;
    int skipped = 0;
    Result result = system.parcels().peekNext(parcelId, skipped);
    if (skipped > 0) printInfo("Skipped " + std::to_string(skipped) + " queue entries that are no longer pending.");
    printResult(result);
    if (result.success) {
        printParcelRow(*system.parcels().find(parcelId));
        promptAndRoute(system, parcelId);
    }
}

void changeRoute(ParcelSystem& system) {
    std::string parcelId = readParcelId();
    Parcel* parcel = system.parcels().find(parcelId);
    if (parcel == nullptr) {
        printResult(Result::fail("Parcel " + parcelId + " was not found."));
    } else if (parcel->status != ParcelStatus::Pending || parcel->route == ParcelRoute::None) {
        printResult(Result::fail("Only a routed parcel that's still Pending can change route."));
    } else {
        promptAndRoute(system, parcelId);
    }
}

void listByStatus(ParcelSystem& system) {
    std::cout << "Status:\n";
    for (int i = 0; i < PARCEL_STATUS_COUNT; i++) {
        std::cout << "  " << (i + 1) << ". " << toString(static_cast<ParcelStatus>(i)) << "\n";
    }
    int choice = Input::readChoice("Choice (0 to cancel): ", 0, PARCEL_STATUS_COUNT);
    if (choice == 0) return;
    ParcelStatus status = static_cast<ParcelStatus>(choice - 1);
    printHeading("Parcels: " + toString(status));
    int count = 0;
    system.organisation().forEachInOrder([&](const Parcel& parcel) {
        if (parcel.status == status) {
            printParcelRow(parcel);
            count++;
        }
    });
    if (count == 0) printInfo("No parcels have this status.");
}

void viewParcel(ParcelSystem& system) {
    std::string parcelId = readParcelId();
    Parcel* parcel = system.parcels().find(parcelId);
    if (parcel == nullptr) {
        printResult(Result::fail("Parcel " + parcelId + " was not found."));
        return;
    }
    printHeading("Parcel details");
    printParcelRow(*parcel);
}

}  // namespace

void runTask1Menu(ParcelSystem& system) {
    while (!Input::atEnd()) {
        printHeading("Task 1: Parcel Intake and Routing");
        std::cout << "1. Register a new parcel\n2. List parcels waiting for a route\n"
                  << "3. Choose route for next parcel\n4. Change a parcel's route\n"
                  << "5. View a parcel\n6. List parcels by status\n"
                  << "7. Update parcel details\n8. Cancel a parcel\n0. Back\n";
        switch (Input::readChoice("Choice: ", 0, 8)) {
            case 1: registerParcel(system); break;
            case 2: listPending(system); break;
            case 3: routeNext(system); break;
            case 4: changeRoute(system); break;
            case 5: viewParcel(system); break;
            case 6: listByStatus(system); break;
            case 7: printResult(promptAndUpdateParcel(system)); break;
            case 8: printResult(system.cancelParcel(readParcelId())); break;
            default: return;
        }
    }
}
