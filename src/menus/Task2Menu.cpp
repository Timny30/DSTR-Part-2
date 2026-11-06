#include "menus/Menus.hpp"
#include "core/Input.hpp"
#include "core/Validation.hpp"
#include <iostream>

namespace {

std::string readRequestId() { return Validation::toUpper(Input::readLine("Request ID: ")); }

void createRequest(ParcelSystem& system) {
    std::string parcelId = Validation::toUpper(Input::readLine("Parcel ID: "));
    ServiceType type;
    if (!promptServiceType(type)) {
        printInfo("Request cancelled.");
        return;
    }
    bool urgent = Input::readYesNo("Mark as urgent?");
    std::string requestId;
    printResult(system.createRequest(parcelId, type, urgent, requestId));
}

void listActive(ParcelSystem& system) {
    printHeading("Active service requests");
    int count = 0;
    system.services().forEachActive([&](const ServiceRequest& request) {
        printRequestRow(request);
        count++;
    });
    if (count == 0) printInfo("There are no active service requests.");
}

void listClosed(ParcelSystem& system) {
    printHeading("Completed, cancelled and returned requests");
    int count = 0;
    system.services().forEachClosed([&](const ServiceRequest& request) {
        printRequestRow(request);
        count++;
    });
    if (count == 0) printInfo("There are no closed service requests.");
}

void viewRequest(ParcelSystem& system) {
    std::string requestId = readRequestId();
    ServiceRequest* request = system.services().find(requestId);
    if (request == nullptr) {
        printResult(Result::fail("Request " + requestId + " was not found."));
        return;
    }
    printHeading("Request details");
    printRequestRow(*request);
}

void previewNext(ParcelSystem& system) {
    ServiceRequest* request = nullptr;
    Result result = system.services().peekNext(request);
    printResult(result);
    if (result.success) printRequestRow(*request);
}

void processNext(ParcelSystem& system) {
    std::string requestId;
    std::string resourceId;
    printResult(system.handleNextRequest(requestId, resourceId));
}

void completeRequest(ParcelSystem& system) {
    std::string requestId = readRequestId();
    ServiceRequest* request = system.services().find(requestId);
    std::string recipientId;
    // Only ask when the request is really ready, so the prompt matches what happens next.
    if (request != nullptr && request->type == ServiceType::RecipientCollection &&
        request->status == RequestStatus::Processing) {
        recipientId = Input::readLine("Recipient ID of the person collecting: ");
    }
    printResult(system.completeRequest(requestId, recipientId));
}

}  // namespace

void runTask2Menu(ParcelSystem& system) {
    while (!Input::atEnd()) {
        printHeading("Task 2: Parcel Service and Collection");
        std::cout << "1. Create a service request\n2. List active requests\n3. View a request\n"
                  << "4. Preview next request\n5. Process next request\n6. Complete a request\n"
                  << "7. Cancel a request\n8. Record a failed delivery\n9. List closed requests\n0. Back\n";
        switch (Input::readChoice("Choice: ", 0, 9)) {
            case 1: createRequest(system); break;
            case 2: listActive(system); break;
            case 3: viewRequest(system); break;
            case 4: previewNext(system); break;
            case 5: processNext(system); break;
            case 6: completeRequest(system); break;
            case 7: printResult(system.cancelRequest(readRequestId())); break;
            case 8: printResult(system.failDelivery(readRequestId())); break;
            case 9: listClosed(system); break;
            default: return;
        }
    }
}
