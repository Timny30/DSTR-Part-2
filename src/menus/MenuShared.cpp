#include "menus/Menus.hpp"
#include "core/Input.hpp"
#include "core/Validation.hpp"
#include <iostream>

void printResult(const Result& result) {
    std::cout << "\n" << (result.success ? "[OK] " : "[ERROR] ") << result.message << "\n";
}

void printInfo(const std::string& message) { std::cout << "\n[INFO] " << message << "\n"; }

void printHeading(const std::string& title) { std::cout << "\n--- " << title << " ---\n"; }

void printParcelRow(const Parcel& parcel) {
    std::cout << parcel.id << " | " << parcel.recipientId << " | " << parcel.recipientName << " | "
              << parcel.deliveryCompany << " | " << toString(parcel.type) << " | " << parcel.arrival << " | "
              << (parcel.hasService ? toString(parcel.serviceType) : "No service") << " | "
              << toString(parcel.status);
    if (parcel.status == ParcelStatus::Pending && parcel.route != ParcelRoute::None) {
        std::cout << " | Route: " << toString(parcel.route);
    }
    std::cout << "\n";
}

void printRequestRow(const ServiceRequest& request) {
    std::cout << request.id << " | " << request.parcelId << " | " << request.recipientId << " | "
              << request.recipientName << " | " << toString(request.type) << " | " << request.requestTime
              << " | Score " << request.score << (request.urgent ? " (urgent)" : "") << " | "
              << toString(request.status);
    if (!request.resourceId.empty()) std::cout << " | Resource " << request.resourceId;
    if (!request.note.empty()) std::cout << " | " << request.note;
    std::cout << "\n";
}

void printResourceRow(const Resource& resource) {
    std::cout << resource.id << " | " << toString(resource.type) << " | " << resource.location << " | "
              << toString(resource.status) << " | Parcel "
              << (resource.currentParcelId.empty() ? "-" : resource.currentParcelId) << " | Request "
              << (resource.currentRequestId.empty() ? "-" : resource.currentRequestId) << " | Handled "
              << resource.transactions << "\n";
}

void printActivityRow(const Activity& activity) {
    std::cout << activity.id << " | " << activity.time << " | " << activity.parcelId << " | "
              << toString(activity.type) << " | " << toString(activity.previousStatus) << " -> "
              << toString(activity.newStatus) << " | Staff " << activity.staffId;
    if (!activity.requestId.empty()) std::cout << " | " << activity.requestId;
    if (!activity.resourceId.empty()) std::cout << " | " << activity.resourceId;
    if (!activity.note.empty()) std::cout << " | " << activity.note;
    if (activity.reversed) std::cout << " | REVERSED";
    std::cout << "\n";
}

bool promptParcelType(ParcelType& out) {
    std::cout << "Parcel type:\n";
    for (int i = 0; i < PARCEL_TYPE_COUNT; i++) {
        std::cout << "  " << (i + 1) << ". " << toString(static_cast<ParcelType>(i)) << "\n";
    }
    int choice = Input::readChoice("Choice (0 to cancel): ", 0, PARCEL_TYPE_COUNT);
    if (choice == 0) return false;
    out = static_cast<ParcelType>(choice - 1);
    return true;
}

bool promptServiceType(ServiceType& out) {
    std::cout << "Service type:\n";
    for (int i = 0; i < SERVICE_TYPE_COUNT; i++) {
        std::cout << "  " << (i + 1) << ". " << toString(static_cast<ServiceType>(i)) << "\n";
    }
    int choice = Input::readChoice("Choice (0 to cancel): ", 0, SERVICE_TYPE_COUNT);
    if (choice == 0) return false;
    out = static_cast<ServiceType>(choice - 1);
    return true;
}

bool promptNewParcelDetails(Parcel& parcel) {
    parcel.recipientId = Input::readLine("Recipient ID: ");
    parcel.recipientName = Input::readLine("Recipient name: ");
    parcel.deliveryCompany = Input::readLine("Delivery company: ");
    return promptParcelType(parcel.type);
}

Result promptAndUpdateParcel(ParcelSystem& system) {
    std::string parcelId = Validation::toUpper(Input::readLine("Parcel ID: "));
    Parcel* current = system.parcels().find(parcelId);
    if (current == nullptr) return system.organisation().update(parcelId, Parcel());

    Parcel changed = *current;
    std::cout << "Leave a field blank to keep its current value.\n";
    std::string text = Input::readLine("Recipient ID [" + current->recipientId + "]: ");
    if (!text.empty()) changed.recipientId = text;
    text = Input::readLine("Recipient name [" + current->recipientName + "]: ");
    if (!text.empty()) changed.recipientName = text;
    text = Input::readLine("Delivery company [" + current->deliveryCompany + "]: ");
    if (!text.empty()) changed.deliveryCompany = text;
    if (Input::readYesNo("Change parcel type?")) {
        if (!promptParcelType(changed.type)) return Result::fail("Update cancelled.");
    }
    return system.updateParcel(parcelId, changed);
}
