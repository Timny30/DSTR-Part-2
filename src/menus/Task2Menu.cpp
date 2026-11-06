#include "menus/Menus.hpp"
#include "core/Input.hpp"
#include "core/Validation.hpp"
#include <iostream>

namespace {

std::string readRequestId() { return Validation::toUpper(Input::readLine("Request ID: ")); }

// 1. Create a Service Request
void createRequest(ParcelSystem& system) {
    printHeading("1. Create a Service Request");
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

// 2. Maintain Active Service Requests
void maintainActiveRequests(ParcelSystem& system) {
    while (!Input::atEnd()) {
        printHeading("2. Maintain Active Service Requests");
        std::cout << "1. List active requests\n"
                  << "2. View a request by ID\n"
                  << "3. List closed requests (completed / cancelled / returned)\n"
                  << "0. Back\n";
        switch (Input::readChoice("Choice: ", 0, 3)) {
            case 1: {
                printHeading("Active service requests");
                int count = 0;
                system.services().forEachActive([&](const ServiceRequest& request) {
                    printRequestRow(request);
                    count++;
                });
                if (count == 0) printInfo("There are no active service requests.");
                break;
            }
            case 2: {
                std::string requestId = readRequestId();
                ServiceRequest* request = system.services().find(requestId);
                if (request == nullptr) {
                    printResult(Result::fail("Request " + requestId + " was not found."));
                } else {
                    printHeading("Request details");
                    printRequestRow(*request);
                }
                break;
            }
            case 3: {
                printHeading("Completed, cancelled and returned requests");
                int count = 0;
                system.services().forEachClosed([&](const ServiceRequest& request) {
                    printRequestRow(request);
                    count++;
                });
                if (count == 0) printInfo("There are no closed service requests.");
                break;
            }
            default:
                return;
        }
    }
}

// 3. Determine the Next Request to be Handled
void determineNextRequest(ParcelSystem& system) {
    printHeading("3. Determine the Next Request to be Handled");
    printInfo("Rule: SpecialHandling(40) > ReDelivery(30) > Collection/Delivery(20) > Return(10). "
              "Urgent adds +50. Same score keeps arrival order.");
    ServiceRequest* request = nullptr;
    Result result = system.services().peekNext(request);
    printResult(result);
    if (result.success) {
        printRequestRow(*request);
        std::cout << "  Priority score: " << request->score
                  << " | Sequence: " << request->sequence << "\n";
    }
}

// 4. Process and Update Service Requests
void processAndUpdateRequests(ParcelSystem& system) {
    while (!Input::atEnd()) {
        printHeading("4. Process and Update Service Requests");
        std::cout << "1. Process next request (assign resource)\n"
                  << "2. Complete a request\n"
                  << "3. Cancel a request\n"
                  << "4. Record a failed delivery (for re-delivery)\n"
                  << "0. Back\n";
        switch (Input::readChoice("Choice: ", 0, 4)) {
            case 1: {
                std::string requestId;
                std::string resourceId;
                printResult(system.handleNextRequest(requestId, resourceId));
                break;
            }
            case 2: {
                std::string requestId = readRequestId();
                ServiceRequest* request = system.services().find(requestId);
                std::string recipientId;
                if (request != nullptr && request->type == ServiceType::RecipientCollection &&
                    request->status == RequestStatus::Processing) {
                    recipientId = Input::readLine("Recipient ID of the person collecting: ");
                }
                printResult(system.completeRequest(requestId, recipientId));
                break;
            }
            case 3:
                printResult(system.cancelRequest(readRequestId()));
                break;
            case 4:
                printResult(system.failDelivery(readRequestId()));
                break;
            default:
                return;
        }
    }
}

// 5. Handle Exceptional Service Conditions (info + live demos)
void handleExceptionalConditions(ParcelSystem& system) {
    printHeading("5. Handle Exceptional Service Conditions");
    std::cout << "The system already rejects or reports these cases:\n"
              << "  - Empty set of active requests (no next request)\n"
              << "  - Invalid / non-existent Request ID or Parcel ID\n"
              << "  - Creating a request on a parcel that already has an active one\n"
              << "  - Creating a request when the parcel is in a final status\n"
              << "  - Processing / completing / cancelling an already closed request\n"
              << "  - Incorrect recipient ID at collection time\n"
              << "  - Processing a request that has no resource assigned yet\n"
              << "  - Equivalent priority: same score keeps earlier arrival order\n\n";

    std::cout << "Quick checks you can try now:\n"
              << "1. Try to get next request when the queue is empty\n"
              << "2. Look up a non-existent request ID\n"
              << "3. Cancel / complete an already closed request\n"
              << "0. Back\n";

    switch (Input::readChoice("Choice: ", 0, 3)) {
        case 1: {
            ServiceRequest* request = nullptr;
            printResult(system.services().peekNext(request));
            break;
        }
        case 2: {
            std::string id = readRequestId();
            ServiceRequest* request = system.services().find(id);
            if (request == nullptr)
                printResult(Result::fail("Request " + id + " was not found."));
            else
                printRequestRow(*request);
            break;
        }
        case 3: {
            std::string id = readRequestId();
            printResult(system.cancelRequest(id));  // will fail if already closed
            break;
        }
        default:
            break;
    }
}

}  // namespace

void runTask2Menu(ParcelSystem& system) {
    while (!Input::atEnd()) {
        printHeading("Task 2: Parcel Service and Collection");
        std::cout << "1. Create a Service Request\n"
                  << "2. Maintain Active Service Requests\n"
                  << "3. Determine the Next Request to be Handled\n"
                  << "4. Process and Update Service Requests\n"
                  << "5. Handle Exceptional Service Conditions\n"
                  << "0. Back\n";
        switch (Input::readChoice("Choice: ", 0, 5)) {
            case 1: createRequest(system); break;
            case 2: maintainActiveRequests(system); break;
            case 3: determineNextRequest(system); break;
            case 4: processAndUpdateRequests(system); break;
            case 5: handleExceptionalConditions(system); break;
            default: return;
        }
    }
}