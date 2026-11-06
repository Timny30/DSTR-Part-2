#pragma once
#include <string>
#include "core/Models.hpp"
#include "core/Result.hpp"
#include "ds/LinkedList.hpp"
#include "ds/PriorityQueue.hpp"
#include "modules/ParcelModule.hpp"

// Heap entry for one request. The master record stays in the linked list.
struct RequestKey {
    std::string requestId;
    int score = 0;
    long sequence = 0;
};

// Higher score first. On a tie, the earlier request goes first.
struct RequestKeyHigherPriority {
    bool operator()(const RequestKey& a, const RequestKey& b) const {
        if (a.score != b.score) return a.score > b.score;
        return a.sequence < b.sequence;
    }
};

// Task 2. Cancelled requests stay in the heap and are skipped when they reach
// the top (lazy deletion). This avoids an O(n) search of the heap on every cancel.
class ServiceModule {
public:
    explicit ServiceModule(ParcelModule& parcels);

    static int scoreFor(ServiceType type, bool urgent);

    Result validateCreate(const std::string& parcelId);
    Result create(const std::string& parcelId, ServiceType type, bool urgent,
                  const std::string& staffId, std::string& outId);

    ServiceRequest* find(const std::string& requestId);
    ServiceRequest* activeForParcel(const std::string& parcelId);

    Result peekNext(ServiceRequest*& out);
    void popNext();
    void pushBack(ServiceRequest& request);  // keeps the original sequence, so its place in line

    void setActive(ServiceRequest& request, RequestStatus status, const std::string& resourceId);
    void close(ServiceRequest& request, RequestStatus status, const std::string& note);
    void syncRecipient(const Parcel& parcel);

    static bool isClosed(const ServiceRequest& request);

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
