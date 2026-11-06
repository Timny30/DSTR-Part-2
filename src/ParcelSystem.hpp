#pragma once
#include <string>
#include "core/Models.hpp"
#include "core/Result.hpp"
#include "ds/LinkedList.hpp"
#include "modules/ActivityModule.hpp"
#include "modules/OrganisationModule.hpp"
#include "modules/ParcelModule.hpp"
#include "modules/ResourceModule.hpp"
#include "modules/ServiceModule.hpp"

// Owns the five modules and handles every action that crosses module boundaries.
class ParcelSystem {
public:
    ParcelSystem();

    void setStaffId(const std::string& staffId) { currentStaffId = staffId; }
    const std::string& staffId() const { return currentStaffId; }

    OrganisationModule& organisation() { return organisationModule; }
    ActivityModule& activities() { return activityModule; }
    ParcelModule& parcels() { return parcelModule; }
    ServiceModule& services() { return serviceModule; }
    ResourceModule& resources() { return resourceModule; }

    // Demo data goes through normal registration, so it's validated and logged.
    Result loadDemoData(const std::string& dataDir);

    template <typename Fn>
    void forEachLoadWarning(Fn fn) { loadWarnings.forEach(fn); }

    // Parcel flow
    Result updateParcel(const std::string& parcelId, const Parcel& changed);
    Result reportMissing(const std::string& parcelId);
    Result markFound(const std::string& parcelId);
    Result cancelParcel(const std::string& parcelId);
    Result deleteParcel(const std::string& parcelId);

    // Request flow
    Result createRequest(const std::string& parcelId, ServiceType type, bool urgent, std::string& outRequestId);
    Result handleNextRequest(std::string& outRequestId, std::string& outResourceId);
    Result handleNextRequestWith(const std::string& resourceId, std::string& outRequestId);
    Result completeRequest(const std::string& requestId, const std::string& recipientId);
    Result cancelRequest(const std::string& requestId);
    Result recordFailure(const std::string& requestId);

    // Resource flow
    Result setResourceStatus(const std::string& resourceId, ResourceStatus next);

    // Reverses the activity on top of the undo stack.
    Result undo(const std::string& activityId);

private:
    ActivityModule activityModule;
    OrganisationModule organisationModule;
    ParcelModule parcelModule;
    ServiceModule serviceModule;
    ResourceModule resourceModule;
    std::string currentStaffId;
    LinkedList<std::string> loadWarnings;

    Result closeParcel(const std::string& parcelId, ParcelStatus target, ActivityType type);
    Result takeNextRequest(const std::string& chosenResourceId, std::string& outRequestId, std::string& outResourceId);
    Result restoreRequestState(const Activity& activity);
    void logUndo(const Activity& original, ParcelStatus statusNow, ParcelStatus statusAfter);
    void loadParcels(const std::string& path, int& loaded);
    void loadResources(const std::string& path, int& loaded);
};
