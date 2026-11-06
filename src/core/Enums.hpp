#pragma once
#include <string>

enum class ParcelType { Standard, Large, Fragile, Document, SpecialHandling };
const int PARCEL_TYPE_COUNT = 5;

enum class ParcelStatus {
    Received, Pending, Processing, ReadyForCollection, OutForDelivery,
    Delivered, Collected, Returned, OnHold, Missing, Cancelled
};
const int PARCEL_STATUS_COUNT = 11;

enum class ServiceType { RecipientCollection, CampusDelivery, SpecialHandling, ReturnToSender, ReDelivery };
const int SERVICE_TYPE_COUNT = 5;

enum class RequestStatus { Pending, Processing, Completed, Cancelled, Returned };

enum class ResourceType { CollectionCounter, DeliveryDesk, SpecialHandlingCounter, DeliveryPersonnel };
const int RESOURCE_TYPE_COUNT = 4;

enum class ResourceStatus { Available, Occupied, Unavailable, Maintenance };
const int RESOURCE_STATUS_COUNT = 4;

enum class ActivityType {
    ParcelRegistration, ParcelUpdate, ServiceRequestCreation, ResourceAssignment, StatusChange,
    CollectionConfirmation, ServiceCompletion, Cancellation, ReturnProcessing, ParcelRemoval, Undo
};

std::string toString(ParcelType value);
std::string toString(ParcelStatus value);
std::string toString(ServiceType value);
std::string toString(RequestStatus value);
std::string toString(ResourceType value);
std::string toString(ResourceStatus value);
std::string toString(ActivityType value);

// Used when reading CSV files. Return false if the text matches no value.
bool parseParcelType(const std::string& text, ParcelType& out);
bool parseResourceType(const std::string& text, ResourceType& out);

// Collected, Delivered, Returned and Cancelled end a parcel's journey.
bool isFinalStatus(ParcelStatus status);
