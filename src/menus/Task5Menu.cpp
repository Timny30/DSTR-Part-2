#include "menus/Menus.hpp"
#include "core/Input.hpp"
#include "core/Validation.hpp"
#include <iostream>

namespace {

std::string readParcelId() { return Validation::toUpper(Input::readLine("Parcel ID: ")); }

void insertParcel(ParcelSystem& system) {
    Parcel parcel;
    parcel.id = readParcelId();
    if (!promptNewParcelDetails(parcel)) {
        printInfo("Insert cancelled.");
        return;
    }
    Result result = system.parcels().registerWithId(parcel, system.staffId());
    printResult(result);
    if (result.success) printParcelRow(*system.parcels().find(parcel.id));
}

void searchById(ParcelSystem& system) {
    std::string parcelId = readParcelId();
    if (!Validation::isParcelId(parcelId)) {
        printResult(Result::fail("Invalid Parcel ID. Use P0001 to P9999."));
        return;
    }
    Parcel* parcel = system.organisation().find(parcelId);
    if (parcel == nullptr) {
        printResult(Result::fail(system.organisation().isEmpty() ? "There are no parcel records."
                                                                 : "Parcel " + parcelId + " was not found."));
        return;
    }
    printHeading("Search result");
    printParcelRow(*parcel);
}

void searchByCategory(ParcelSystem& system) {
    ParcelType type;
    if (!promptParcelType(type)) return;
    printHeading("Category: " + toString(type));
    int count = 0;
    system.organisation().forEachByType(type, [&](const Parcel& parcel) {
        printParcelRow(parcel);
        count++;
    });
    if (count == 0) printInfo("No parcels are in this category.");
}

void searchByRecipient(ParcelSystem& system) {
    std::string recipientId = Validation::toUpper(Input::readLine("Recipient ID: "));
    if (!Validation::isRecipientId(recipientId)) {
        printResult(Result::fail("Recipient ID must be 3 to 12 letters or digits."));
        return;
    }
    printHeading("Parcels for " + recipientId);
    int count = 0;
    system.organisation().forEachByRecipient(recipientId, [&](const Parcel& parcel) {
        printParcelRow(parcel);
        count++;
    });
    if (count == 0) printInfo("This recipient has no parcels.");
}

void displaySorted(ParcelSystem& system) {
    printHeading("All parcels by ID (in-order traversal)");
    if (system.organisation().isEmpty()) {
        printInfo("There are no parcel records.");
        return;
    }
    system.organisation().forEachInOrder([](const Parcel& parcel) { printParcelRow(parcel); });
}

void displayStructure(ParcelSystem& system) {
    printHeading("Tree structure (pre-order traversal)");
    if (system.organisation().isEmpty()) {
        printInfo("There are no parcel records.");
        return;
    }
    system.organisation().forEachPreOrder([](const Parcel& parcel, int depth) {
        std::cout << std::string(static_cast<size_t>(depth) * 2, ' ') << "|- " << parcel.id
                  << " (" << toString(parcel.status) << ")\n";
    });
    printInfo("Parcels: " + std::to_string(system.organisation().size()) +
              ", tree height: " + std::to_string(system.organisation().height()) + ".");
}

} 

void runTask5Menu(ParcelSystem& system) {
    while (!Input::atEnd()) {
        printHeading("Task 5: Parcel Organisation");
        std::cout << "1. Insert a parcel record\n2. Search by Parcel ID\n3. Search by category\n"
                  << "4. Search by recipient\n5. Display all, sorted by ID\n6. Display tree structure\n"
                  << "7. Update a parcel record\n8. Remove a parcel record\n0. Back\n";
        switch (Input::readChoice("Choice: ", 0, 8)) {
            case 1: insertParcel(system); break;
            case 2: searchById(system); break;
            case 3: searchByCategory(system); break;
            case 4: searchByRecipient(system); break;
            case 5: displaySorted(system); break;
            case 6: displayStructure(system); break;
            case 7: printResult(promptAndUpdateParcel(system)); break;
            case 8: printResult(system.deleteParcel(readParcelId())); break;
            default: return;
        }
    }
}
