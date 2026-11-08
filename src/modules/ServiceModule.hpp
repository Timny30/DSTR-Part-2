#pragma once
#include <string>
#include "core/DateTime.hpp"
#include "core/Models.hpp"
#include "core/Result.hpp"
#include "core/Validation.hpp"
#include "ds/LinkedList.hpp"
#include "ds/PriorityQueue.hpp"
#include "modules/ParcelModule.hpp"

struct RequestKey {
    std::string requestId;
    int score = 0;
    long sequence = 0;
};

struct RequestKeyHigherPriority {
    bool operator()(const RequestKey& a, const RequestKey& b) const {
        if (a.score != b.score) return a.score > b.score;
        return a.sequence < b.sequence;
    }
};

class ServiceModule {
public:
    explicit ServiceModule(ParcelModule& parcelModule) : parcels(parcelModule) {}

    static int scoreFor(ServiceType type, bool urgent) {
        int score = 0;
        switch (type) {
            case ServiceType::SpecialHandling: score = 40; break;   
            case ServiceType::ReDelivery: score = 30; break;      
            case ServiceType::RecipientCollection: score = 20; break;  
            case ServiceType::CampusDelivery: score = 20; break;      
            case ServiceType::ReturnToSender: score = 10; break;       
        }
        return urgent ? score + 50 : score;
    }

    static bool typeForRoute(ParcelRoute route, ServiceType& out) {
        switch (route) {
            case ParcelRoute::Collection: out = ServiceType::RecipientCollection; return true;
            case ParcelRoute::Delivery: out = ServiceType::CampusDelivery; return true;
            case ParcelRoute::SpecialHandling: out = ServiceType::SpecialHandling; return true;
            case ParcelRoute::Return: out = ServiceType::ReturnToSender; return true;
            case ParcelRoute::Hold:
            case ParcelRoute::None: return false;
        }
        return false;
    }

    Result validateCreate(const std::string& parcelId) {
        Parcel* parcel = parcels.find(parcelId);
        if (parcel == nullptr) return Result::fail("Parcel " + parcelId + " was not found.");
        if (parcel->status == ParcelStatus::Pending && parcel->route == ParcelRoute::None) {
            return Result::fail("Parcel " + parcelId + " has no route. Choose its route in Task 1 first.");
        }
        if (parcel->status != ParcelStatus::Pending && parcel->status != ParcelStatus::Processing &&
            parcel->status != ParcelStatus::ReadyForCollection && parcel->status != ParcelStatus::OnHold) {
            return Result::fail("A request can't be created while parcel " + parcelId + " is " +
                                toString(parcel->status) + ".");
        }
        ServiceRequest* active = activeForParcel(parcelId);
        if (active != nullptr) {
            return Result::fail("Parcel " + parcelId + " already has active request " + active->id + ".");
        }
        return Result::ok("Request can be created.");
    }

    Result create(const std::string& parcelId, ServiceType type, bool urgent,
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

    ServiceRequest* find(const std::string& requestId) {
        return requests.find([&](const ServiceRequest& request) { return request.id == requestId; });
    }

    ServiceRequest* activeForParcel(const std::string& parcelId) {
        return requests.find([&](const ServiceRequest& request) {
            return request.parcelId == parcelId && !isClosed(request);
        });
    }

    Result peekNext(ServiceRequest*& out) {
        RequestKey key;
        while (heap.peek(key)) {
            ServiceRequest* request = find(key.requestId);
            if (request != nullptr && request->status == RequestStatus::Pending) {
                out = request;
                return Result::ok("Next request is " + request->id + ".");
            }
            heap.pop(key);
            if (request != nullptr) request->inHeap = false;
        }
        return Result::fail("There are no service requests waiting.");
    }

    void popNext() {
        RequestKey key;
        if (heap.pop(key)) {
            ServiceRequest* request = find(key.requestId);
            if (request != nullptr) request->inHeap = false;
        }
    }

    void pushBack(ServiceRequest& request) {
        if (request.inHeap) return;  
        RequestKey key;
        key.requestId = request.id;
        key.score = request.score;
        key.sequence = request.sequence;
        heap.push(key);
        request.inHeap = true;
    }

    void setActive(ServiceRequest& request, RequestStatus status, const std::string& resourceId) {
        request.status = status;
        request.resourceId = resourceId;
        request.note.clear();
    }

    void close(ServiceRequest& request, RequestStatus status, const std::string& note) {
        request.status = status;
        request.note = note;
    }

    void syncRecipient(const Parcel& parcel) {
        ServiceRequest* active = activeForParcel(parcel.id);
        if (active != nullptr) {
            active->recipientId = parcel.recipientId;
            active->recipientName = parcel.recipientName;
        }
    }

    static bool isClosed(const ServiceRequest& request) {
        return request.status == RequestStatus::Completed || request.status == RequestStatus::Cancelled ||
               request.status == RequestStatus::Returned;
    }

    template <typename Fn>
    void forEachActive(Fn fn) {
        requests.forEach([&](ServiceRequest& request) { if (!isClosed(request)) fn(request); });
    }

    template <typename Fn>
    void forEachClosed(Fn fn) {
        requests.forEach([&](ServiceRequest& request) { if (isClosed(request)) fn(request); });
    }

private:
    ParcelModule& parcels;
    LinkedList<ServiceRequest> requests;
    PriorityQueue<RequestKey, RequestKeyHigherPriority> heap;
    int nextNumber = 1;
};
