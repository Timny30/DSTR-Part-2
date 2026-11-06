#pragma once
#include <string>
#include "core/Models.hpp"
#include "core/Result.hpp"
#include "ds/Queue.hpp"
#include "modules/ActivityModule.hpp"
#include "modules/OrganisationModule.hpp"

// Extra details logged with a status change.
struct StatusChangeOptions {
    ActivityType type = ActivityType::StatusChange;
    bool reversible = true;
    bool allowSameStatus = false;  // log an event that doesn't change the parcel's status
    std::string requestId;
    std::string resourceId;
    std::string note;
    bool hasRequestBefore = false;
    RequestStatus requestStatusBefore = RequestStatus::Pending;
    std::string resourceIdBefore;
};

// Task 1. The queue stores only Parcel IDs, and the master record stays in the
// BST, so a waiting parcel's details can never go out of date.
class ParcelModule {
public:
    ParcelModule(OrganisationModule& organisation, ActivityModule& activities);

    Result registerNew(Parcel input, const std::string& staffId, std::string& outId);
    Result registerWithId(Parcel input, const std::string& staffId);

    Result processNext(const std::string& staffId, std::string& outId, int& skippedStale);

    // The only route for a normal status change. Checks the rules and logs it.
    Result changeStatus(const std::string& parcelId, ParcelStatus next, const std::string& staffId,
                        const StatusChangeOptions& options = StatusChangeOptions());

    Result updateFields(const std::string& parcelId, Parcel changed, const std::string& staffId);

    // For undo only. Puts a saved record back and rejoins the queue if it was waiting.
    Result restoreSnapshot(const Parcel& saved);

    static bool canTransition(ParcelStatus from, ParcelStatus to);

    Parcel* find(const std::string& parcelId) { return organisation.find(parcelId); }

    // Visits waiting parcels in queue order. Stale entries are left out.
    template <typename Fn>
    void forEachPending(Fn fn) const {
        pending.forEach([&](const std::string& parcelId) {
            Parcel* parcel = organisation.find(parcelId);
            if (parcel != nullptr && parcel->status == ParcelStatus::Pending) fn(*parcel);
        });
    }

private:
    OrganisationModule& organisation;
    ActivityModule& activities;
    Queue<std::string> pending;
    int nextNumber = 1;

    Result addParcel(Parcel& input, const std::string& staffId);
    bool isQueued(const std::string& parcelId) const;
};
