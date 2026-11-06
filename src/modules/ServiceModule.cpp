#include "modules/ServiceModule.hpp"
#include "core/DateTime.hpp"
#include "core/Validation.hpp"

ServiceModule::ServiceModule(ParcelModule& parcelModule) : parcels(parcelModule) {}

int ServiceModule::scoreFor(ServiceType type, bool urgent) {
    int score = 0;
    switch (type) {
        case ServiceType::SpecialHandling: score = 40; break;      // highest risk of damage
        case ServiceType::ReDelivery: score = 30; break;           // already had a failed delivery
        case ServiceType::RecipientCollection: score = 20; break;  // the recipient is waiting
        case ServiceType::CampusDelivery: score = 20; break;       // standard service
        case ServiceType::ReturnToSender: score = 10; break;       // no recipient is waiting
    }
    return urgent ? score + 50 : score;
}

bool ServiceModule::isClosed(const ServiceRequest& request) {
    return request.status == RequestStatus::Completed || request.status == RequestStatus::Cancelled ||
           request.status == RequestStatus::Returned;
}

Result ServiceModule::validateCreate(const std::string& parcelId) {
    Parcel* parcel = parcels.find(parcelId);
    if (parcel == nullptr) return Result::fail("Parcel " + parcelId + " was not found.");
    if (parcel->status != ParcelStatus::Processing && parcel->status != ParcelStatus::ReadyForCollection &&
        parcel->status != ParcelStatus::OnHold) {
        return Result::fail("A request can't be created while parcel " + parcelId + " is " +
                            toString(parcel->status) + ".");
    }
    ServiceRequest* active = activeForParcel(parcelId);
    if (active != nullptr) {
        return Result::fail("Parcel " + parcelId + " already has active request " + active->id + ".");
    }
    return Result::ok("Request can be created.");
}

Result ServiceModule::create(const std::string& parcelId, ServiceType type, bool urgent,
                             const std::string& staffId, std::string& outId) {
    Result valid = validateCreate(parcelId);
    if (!valid.success) return valid;
    Parcel* parcel = parcels.find(parcelId);

    ServiceRequest request;
    request.id = Validation::formatId("REQ", nextNumber, 4);
    request.parcelId = parcelId;
    request.recipientId = parcel->recipientId;
    request.recipientName = parcel->recipientName;
    request.type = type;
    request.requestTime = DateTime::now();
    request.urgent = urgent;
    request.score = scoreFor(type, urgent);
    request.sequence = nextNumber;
    nextNumber++;
    requests.append(request);
    pushBack(*find(request.id));

    StatusChangeOptions options;
    options.type = ActivityType::ServiceRequestCreation;
    options.requestId = request.id;
    options.note = toString(type);
    Result logged = parcels.changeStatus(parcelId, parcel->status, staffId, options);
    if (!logged.success) return logged;

    parcel->hasService = true;
    parcel->serviceType = type;
    outId = request.id;
    return Result::ok("Request " + request.id + " created for parcel " + parcelId + " with priority score " +
                      std::to_string(request.score) + ".");
}

ServiceRequest* ServiceModule::find(const std::string& requestId) {
    return requests.find([&](const ServiceRequest& request) { return request.id == requestId; });
}

ServiceRequest* ServiceModule::activeForParcel(const std::string& parcelId) {
    return requests.find([&](const ServiceRequest& request) {
        return request.parcelId == parcelId && !isClosed(request);
    });
}

Result ServiceModule::peekNext(ServiceRequest*& out) {
    RequestKey key;
    while (heap.peek(key)) {
        ServiceRequest* request = find(key.requestId);
        if (request != nullptr && request->status == RequestStatus::Pending) {
            out = request;
            return Result::ok("Next request is " + request->id + ".");
        }
        // Lazy deletion: this entry was cancelled or already taken, so drop it now.
        heap.pop(key);
        if (request != nullptr) request->inHeap = false;
    }
    return Result::fail("There are no service requests waiting.");
}

void ServiceModule::popNext() {
    RequestKey key;
    if (heap.pop(key)) {
        ServiceRequest* request = find(key.requestId);
        if (request != nullptr) request->inHeap = false;
    }
}

void ServiceModule::pushBack(ServiceRequest& request) {
    if (request.inHeap) return;  // a cancelled entry that was never dropped can be reused
    RequestKey key;
    key.requestId = request.id;
    key.score = request.score;
    key.sequence = request.sequence;
    heap.push(key);
    request.inHeap = true;
}

void ServiceModule::setActive(ServiceRequest& request, RequestStatus status, const std::string& resourceId) {
    request.status = status;
    request.resourceId = resourceId;
    request.note.clear();
}

void ServiceModule::close(ServiceRequest& request, RequestStatus status, const std::string& note) {
    request.status = status;
    request.note = note;
}

void ServiceModule::syncRecipient(const Parcel& parcel) {
    ServiceRequest* active = activeForParcel(parcel.id);
    if (active != nullptr) {
        active->recipientId = parcel.recipientId;
        active->recipientName = parcel.recipientName;
    }
}
