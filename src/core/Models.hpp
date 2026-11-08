#pragma once
#include <string>
#include "core/Enums.hpp"

struct Parcel {
    std::string id;
    std::string recipientId;
    std::string recipientName;
    std::string deliveryCompany;
    ParcelType type = ParcelType::Standard;
    std::string arrival;
    ParcelStatus status = ParcelStatus::Received;
    ParcelRoute route = ParcelRoute::None;                   
    bool hasService = false;                               
    ServiceType serviceType = ServiceType::RecipientCollection;  
};

struct ServiceRequest {
    std::string id;
    std::string parcelId;
    std::string recipientId;
    std::string recipientName;
    ServiceType type = ServiceType::RecipientCollection;
    std::string requestTime;
    RequestStatus status = RequestStatus::Pending;
    bool urgent = false;
    int score = 0;
    long sequence = 0;   
    bool inHeap = false;    
    std::string resourceId;  
    ParcelStatus parcelStatusBeforeAssign = ParcelStatus::Processing;
    std::string note;   
};

struct Resource {
    std::string id;
    ResourceType type = ResourceType::CollectionCounter;
    std::string location;
    ResourceStatus status = ResourceStatus::Available;
    std::string currentParcelId;
    std::string currentRequestId;
    int transactions = 0;
};

struct Activity {
    std::string id;
    long sequence = 0;
    std::string parcelId;
    std::string requestId;
    std::string resourceId;
    ActivityType type = ActivityType::StatusChange;
    std::string time;
    ParcelStatus previousStatus = ParcelStatus::Received;
    ParcelStatus newStatus = ParcelStatus::Received;
    std::string staffId;
    std::string note;

    bool reversible = false;
    bool reversed = false;

    bool hasParcelBefore = false;
    Parcel parcelBefore;
    bool hasRequestBefore = false;
    RequestStatus requestStatusBefore = RequestStatus::Pending;
    std::string resourceIdBefore;
};
