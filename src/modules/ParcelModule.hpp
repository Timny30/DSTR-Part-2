#pragma once
#include <string>
#include "core/DateTime.hpp"
#include "core/Models.hpp"
#include "core/Result.hpp"
#include "core/Validation.hpp"
#include "ds/Queue.hpp"
#include "modules/ActivityModule.hpp"
#include "modules/OrganisationModule.hpp"

struct StatusChangeOptions {
    ActivityType type = ActivityType::StatusChange;
    bool reversible = true;
    bool allowSameStatus = false;  
    std::string requestId;
    std::string resourceId;
    std::string note;
    bool hasRequestBefore = false;
    RequestStatus requestStatusBefore = RequestStatus::Pending;
    std::string resourceIdBefore;
};

class ParcelModule {
public:
    ParcelModule(OrganisationModule& organisationModule, ActivityModule& activityModule)
        : organisation(organisationModule), activities(activityModule) {}

    Result registerNew(Parcel input, const std::string& staffId, std::string& outId) {
        if (nextNumber > MAX_PARCEL_NUMBER) return Result::fail("All Parcel IDs up to P9999 have been used.");
        input.id = Validation::formatId("P", nextNumber, 4);
        Result result = addParcel(input, staffId);
        if (result.success) outId = input.id;
        return result;
    }

    Result registerWithId(Parcel input, const std::string& staffId) { return addParcel(input, staffId); }

    Result peekNext(std::string& outId, int& skippedStale) {
        skippedStale = 0;
        std::string parcelId;
        while (pending.peek(parcelId)) {
            if (isWaitingForRoute(organisation.find(parcelId))) {
                outId = parcelId;
                return Result::ok("Parcel " + parcelId + " is next to be routed.");
            }
            // It was cancelled, removed or reported missing while it waited, so drop it.
            pending.dequeue(parcelId);
            skippedStale++;
        }
        return Result::fail("There are no pending parcels to route.");
    }

    Result routeParcel(const std::string& parcelId, ParcelRoute route, const std::string& staffId) {
        Parcel* parcel = organisation.find(parcelId);
        if (parcel == nullptr) return Result::fail("Parcel " + parcelId + " was not found.");
        if (route == ParcelRoute::None) return Result::fail("Choose a route for parcel " + parcelId + ".");
        if (parcel->status != ParcelStatus::Pending) {
            return Result::fail("Only a Pending parcel can be routed. Parcel " + parcelId + " is " +
                                toString(parcel->status) + ".");
        }

        bool isFirstRoute = parcel->route == ParcelRoute::None;
        if (isFirstRoute) {
            std::string nextId;
            int skipped = 0;
            if (!peekNext(nextId, skipped).success || nextId != parcelId) {
                return Result::fail("Parcels are routed in arrival order. The next parcel is " + nextId + ".");
            }
        }

        StatusChangeOptions options;
        options.type = ActivityType::ParcelRouting;
        options.note = toString(route);
        ParcelStatus next = route == ParcelRoute::Hold ? ParcelStatus::OnHold : ParcelStatus::Pending;
        Result changed = changeStatus(parcelId, next, staffId, options);
        if (!changed.success) return changed;

        if (isFirstRoute) {
            std::string routedId;
            pending.dequeue(routedId);
        }
        parcel->route = route;
        if (route == ParcelRoute::Hold) return Result::ok("Parcel " + parcelId + " is now On Hold.");
        return Result::ok("Parcel " + parcelId + " routed to " + toString(route) +
                          ". Create its service request in Task 2.");
    }

    // The only place for changing status. (Includes checks rules and logging)
    Result changeStatus(const std::string& parcelId, ParcelStatus next, const std::string& staffId,
                        const StatusChangeOptions& options = StatusChangeOptions()) {
        Parcel* parcel = organisation.find(parcelId);
        if (parcel == nullptr) return Result::fail("Parcel " + parcelId + " was not found.");

        bool sameAllowed = parcel->status == next && !isFinalStatus(next) &&
                           (options.allowSameStatus || options.type != ActivityType::StatusChange);
        if (!sameAllowed && !canTransition(parcel->status, next)) {
            return Result::fail("Parcel " + parcelId + " can't change from " + toString(parcel->status) +
                                " to " + toString(next) + ".");
        }

        Activity activity;
        activity.parcelId = parcelId;
        activity.requestId = options.requestId;
        activity.resourceId = options.resourceId;
        activity.type = options.type;
        activity.previousStatus = parcel->status;
        activity.newStatus = next;
        activity.staffId = staffId;
        activity.note = options.note;
        // Once a parcel has physically left, the action cannot be undone.
        activity.reversible = options.reversible && next != ParcelStatus::Collected &&
                              next != ParcelStatus::Delivered && next != ParcelStatus::Returned;
        activity.hasParcelBefore = true;
        activity.parcelBefore = *parcel;
        activity.hasRequestBefore = options.hasRequestBefore;
        activity.requestStatusBefore = options.requestStatusBefore;
        activity.resourceIdBefore = options.resourceIdBefore;

        parcel->status = next;
        activities.record(activity);
        return Result::ok("Parcel " + parcelId + " is now " + toString(next) + ".");
    }

    Result updateFields(const std::string& parcelId, Parcel changed, const std::string& staffId) {
        Parcel* parcel = organisation.find(parcelId);
        if (parcel == nullptr) return organisation.update(parcelId, changed);
        if (isFinalStatus(parcel->status)) {
            return Result::fail("Parcel " + parcelId + " is " + toString(parcel->status) + " and can't be updated.");
        }

        Parcel before = *parcel;
        changed.recipientId = Validation::toUpper(Validation::trim(changed.recipientId));
        changed.recipientName = Validation::trim(changed.recipientName);
        changed.deliveryCompany = Validation::trim(changed.deliveryCompany);
        changed.status = before.status; 
        changed.arrival = before.arrival;

        Result updated = organisation.update(parcelId, changed);
        if (!updated.success) return updated;

        Activity activity;
        activity.parcelId = parcelId;
        activity.type = ActivityType::ParcelUpdate;
        activity.previousStatus = before.status;
        activity.newStatus = before.status;
        activity.staffId = staffId;
        activity.reversible = true;
        activity.hasParcelBefore = true;
        activity.parcelBefore = before;
        activities.record(activity);
        return updated;
    }


    Result restoreSnapshot(const Parcel& saved) {
        Parcel* parcel = organisation.find(saved.id);
        if (parcel == nullptr) return Result::fail("Parcel " + saved.id + " no longer exists.");
        *parcel = saved;
        if (isWaitingForRoute(&saved) && !isQueued(saved.id)) pending.enqueue(saved.id);
        return Result::ok("Parcel " + saved.id + " restored.");
    }

    static bool canTransition(ParcelStatus from, ParcelStatus to) {
        if (isFinalStatus(from) || from == to) return false;
        if (to == ParcelStatus::Cancelled || to == ParcelStatus::Missing) return true;

        switch (from) {
            case ParcelStatus::Received:
                return to == ParcelStatus::Pending;
            case ParcelStatus::Pending:
                return to == ParcelStatus::Processing || to == ParcelStatus::OnHold;
            case ParcelStatus::Processing:
                return to == ParcelStatus::ReadyForCollection || to == ParcelStatus::OutForDelivery ||
                       to == ParcelStatus::Returned || to == ParcelStatus::OnHold;
            case ParcelStatus::ReadyForCollection:
                return to == ParcelStatus::Collected || to == ParcelStatus::OutForDelivery ||
                       to == ParcelStatus::Processing || to == ParcelStatus::OnHold;
            case ParcelStatus::OutForDelivery:
                return to == ParcelStatus::Delivered || to == ParcelStatus::ReadyForCollection ||
                       to == ParcelStatus::Processing || to == ParcelStatus::OnHold;
            case ParcelStatus::OnHold:
                return to == ParcelStatus::Processing || to == ParcelStatus::ReadyForCollection;
            case ParcelStatus::Missing:
                return to == ParcelStatus::OnHold;
            default:
                return false;
        }
    }

    Parcel* find(const std::string& parcelId) { return organisation.find(parcelId); }

    template <typename Fn>
    void forEachPending(Fn fn) const {
        pending.forEach([&](const std::string& parcelId) {
            Parcel* parcel = organisation.find(parcelId);
            if (isWaitingForRoute(parcel)) fn(*parcel);
        });
    }

private:
    static const int MAX_PARCEL_NUMBER = 9999;

    OrganisationModule& organisation;
    ActivityModule& activities;
    Queue<std::string> pending;
    int nextNumber = 1;

    Result addParcel(Parcel& input, const std::string& staffId) {
        input.recipientId = Validation::toUpper(Validation::trim(input.recipientId));
        input.recipientName = Validation::trim(input.recipientName);
        input.deliveryCompany = Validation::trim(input.deliveryCompany);
        if (input.arrival.empty()) input.arrival = DateTime::now();
        if (!Validation::isDateTime(input.arrival)) return Result::fail("Arrival must use YYYY-MM-DD HH:MM.");

        input.status = ParcelStatus::Pending;
        Result inserted = organisation.insert(input);
        if (!inserted.success) return inserted;

        int number = std::stoi(input.id.substr(1));
        if (number >= nextNumber) nextNumber = number + 1;

        if (!isQueued(input.id)) pending.enqueue(input.id);

        Activity activity;
        activity.parcelId = input.id;
        activity.type = ActivityType::ParcelRegistration;
        activity.previousStatus = ParcelStatus::Received;
        activity.newStatus = ParcelStatus::Pending;
        activity.staffId = staffId;
        activity.reversible = true;
        activities.record(activity);

        return Result::ok("Parcel " + input.id + " registered and queued for processing.");
    }

    static bool isWaitingForRoute(const Parcel* parcel) {
        return parcel != nullptr && parcel->status == ParcelStatus::Pending && parcel->route == ParcelRoute::None;
    }

    bool isQueued(const std::string& parcelId) const {
        bool found = false;
        pending.forEach([&](const std::string& queuedId) {
            if (queuedId == parcelId) found = true;
        });
        return found;
    }
};
