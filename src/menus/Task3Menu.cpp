#include "menus/Menus.hpp"
#include "core/Input.hpp"
#include "core/Validation.hpp"
#include <iostream>

namespace {

std::string readResourceId() { return Validation::toUpper(Input::readLine("Resource ID: ")); }

void addResource(ParcelSystem& system) {
    std::cout << "Resource type:\n";
    for (int i = 0; i < RESOURCE_TYPE_COUNT; i++) {
        std::cout << "  " << (i + 1) << ". " << toString(static_cast<ResourceType>(i)) << "\n";
    }
    int choice = Input::readChoice("Choice (0 to cancel): ", 0, RESOURCE_TYPE_COUNT);
    if (choice == 0) return;
    std::string location = Input::readLine("Location (optional): ");
    std::string newId;
    printResult(system.resources().add(static_cast<ResourceType>(choice - 1), location, newId));
}

void listResources(ParcelSystem& system) {
    printHeading("Resources");
    if (system.resources().isEmpty()) {
        printInfo("No resources have been registered.");
        return;
    }
    system.resources().forEach([](const Resource& resource) { printResourceRow(resource); });
}

void assignNext(ParcelSystem& system) {
    std::string requestId;
    std::string resourceId;
    printResult(system.handleNextRequest(requestId, resourceId));
}

void assignNextToChosen(ParcelSystem& system) {
    std::string requestId;
    printResult(system.handleNextRequestWith(readResourceId(), requestId));
}

void updateStatus(ParcelSystem& system) {
    std::string resourceId = readResourceId();
    std::cout << "New status:\n";
    for (int i = 0; i < RESOURCE_STATUS_COUNT; i++) {
        std::cout << "  " << (i + 1) << ". " << toString(static_cast<ResourceStatus>(i)) << "\n";
    }
    int choice = Input::readChoice("Choice (0 to cancel): ", 0, RESOURCE_STATUS_COUNT);
    if (choice == 0) return;
    printResult(system.setResourceStatus(resourceId, static_cast<ResourceStatus>(choice - 1)));
}

void showUtilisation(ParcelSystem& system) {
    printHeading("Resource utilisation");
    for (int i = 0; i < RESOURCE_TYPE_COUNT; i++) {
        ResourceType type = static_cast<ResourceType>(i);
        int total = 0;
        int occupied = 0;
        int handled = 0;
        system.resources().forEach([&](const Resource& resource) {
            if (resource.type != type) return;
            total++;
            handled += resource.transactions;
            if (resource.status == ResourceStatus::Occupied) occupied++;
        });
        std::cout << toString(type) << ": " << total << " registered, " << occupied
                  << " occupied, " << handled << " jobs completed\n";
    }
}

}  

void runTask3Menu(ParcelSystem& system) {
    while (!Input::atEnd()) {
        printHeading("Task 3: Counters and Delivery Resources");
        std::cout << "1. Register a resource\n2. List resources and availability\n"
                  << "3. Assign next request automatically\n4. Assign next request to a chosen resource\n"
                  << "5. Update resource status\n6. Show utilisation\n0. Back\n";
        switch (Input::readChoice("Choice: ", 0, 6)) {
            case 1: addResource(system); break;
            case 2: listResources(system); break;
            case 3: assignNext(system); break;
            case 4: assignNextToChosen(system); break;
            case 5: updateStatus(system); break;
            case 6: showUtilisation(system); break;
            default: return;
        }
    }
}
