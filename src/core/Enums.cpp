#include "core/Enums.hpp"

std::string toString(ParcelType value) {
    switch (value) {
        case ParcelType::Standard: return "Standard Parcel";
        case ParcelType::Large: return "Large Parcel";
        case ParcelType::Fragile: return "Fragile Parcel";
        case ParcelType::Document: return "Document";
        case ParcelType::SpecialHandling: return "Special-Handling Parcel";
    }
    return "Unknown";
}

std::string toString(ParcelStatus value) {
    switch (value) {
        case ParcelStatus::Received: return "Received";
        case ParcelStatus::Pending: return "Pending";
        case ParcelStatus::Processing: return "Processing";
        case ParcelStatus::ReadyForCollection: return "Ready for Collection";
        case ParcelStatus::OutForDelivery: return "Out for Delivery";
        case ParcelStatus::Delivered: return "Delivered";
        case ParcelStatus::Collected: return "Collected";
        case ParcelStatus::Returned: return "Returned";
        case ParcelStatus::OnHold: return "On Hold";
        case ParcelStatus::Missing: return "Missing";
        case ParcelStatus::Cancelled: return "Cancelled";
    }
    return "Unknown";
}

std::string toString(ServiceType value) {
    switch (value) {
        case ServiceType::RecipientCollection: return "Recipient Collection";
        case ServiceType::CampusDelivery: return "Campus Delivery";
        case ServiceType::SpecialHandling: return "Special Handling";
        case ServiceType::ReturnToSender: return "Return to Sender";
        case ServiceType::ReDelivery: return "Re-delivery";
    }
    return "Unknown";
}

std::string toString(RequestStatus value) {
    switch (value) {
        case RequestStatus::Pending: return "Pending";
        case RequestStatus::Processing: return "Processing";
        case RequestStatus::Completed: return "Completed";
        case RequestStatus::Cancelled: return "Cancelled";
        case RequestStatus::Returned: return "Returned";
    }
    return "Unknown";
}

std::string toString(ResourceType value) {
    switch (value) {
        case ResourceType::CollectionCounter: return "Collection Counter";
        case ResourceType::DeliveryDesk: return "Delivery Desk";
        case ResourceType::SpecialHandlingCounter: return "Special-Handling Counter";
        case ResourceType::DeliveryPersonnel: return "Delivery Personnel";
    }
    return "Unknown";
}

std::string toString(ResourceStatus value) {
    switch (value) {
        case ResourceStatus::Available: return "Available";
        case ResourceStatus::Occupied: return "Occupied";
        case ResourceStatus::Unavailable: return "Unavailable";
        case ResourceStatus::Maintenance: return "Maintenance";
    }
    return "Unknown";
}

std::string toString(ActivityType value) {
    switch (value) {
        case ActivityType::ParcelRegistration: return "Parcel Registration";
        case ActivityType::ParcelUpdate: return "Parcel Update";
        case ActivityType::ServiceRequestCreation: return "Service Request Creation";
        case ActivityType::ResourceAssignment: return "Resource Assignment";
        case ActivityType::StatusChange: return "Status Change";
        case ActivityType::CollectionConfirmation: return "Collection Confirmation";
        case ActivityType::ServiceCompletion: return "Service Completion";
        case ActivityType::Cancellation: return "Cancellation";
        case ActivityType::ReturnProcessing: return "Return Processing";
        case ActivityType::ParcelRemoval: return "Parcel Removal";
        case ActivityType::Undo: return "Undo";
    }
    return "Unknown";
}

bool parseParcelType(const std::string& text, ParcelType& out) {
    for (int i = 0; i < PARCEL_TYPE_COUNT; i++) {
        ParcelType candidate = static_cast<ParcelType>(i);
        if (toString(candidate) == text) {
            out = candidate;
            return true;
        }
    }
    return false;
}

bool parseResourceType(const std::string& text, ResourceType& out) {
    for (int i = 0; i < RESOURCE_TYPE_COUNT; i++) {
        ResourceType candidate = static_cast<ResourceType>(i);
        if (toString(candidate) == text) {
            out = candidate;
            return true;
        }
    }
    return false;
}

bool isFinalStatus(ParcelStatus status) {
    return status == ParcelStatus::Collected || status == ParcelStatus::Delivered ||
           status == ParcelStatus::Returned || status == ParcelStatus::Cancelled;
}
