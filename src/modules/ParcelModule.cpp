#include "modules/ParcelModule.hpp"
#include "core/DateTime.hpp"
#include "core/Validation.hpp"

namespace {
    const int MAX_PARCEL_NUMBER = 9999;
}

ParcelModule::ParcelModule(OrganisationModule& organisationModule, ActivityModule& activityModule)
    : organisation(organisationModule), activities(activityModule) {}

Result ParcelModule::registerNew(Parcel input, const std::string& staffId, std::string& outId) {
    if (nextNumber > MAX_PARCEL_NUMBER) return Result::fail("All Parcel IDs up to P9999 have been used.");
    input.id = Validation::formatId("P", nextNumber, 4);
    Result result = addParcel(input, staffId);
    if (result.success) outId = input.id;
    return result;
}

Result ParcelModule::registerWithId(Parcel input, const std::string& staffId) {
    return addParcel(input, staffId);
}

Result ParcelModule::addParcel(Parcel& input, const std::string& staffId) {
    input.recipientId = Validation::toUpper(Validation::trim(input.recipientId));
    input.recipientName = Validation::trim(input.recipientName);
    input.deliveryCompany = Validation::trim(input.deliveryCompany);
    if (input.arrival.empty()) input.arrival = DateTime::now();
    if (!Validation::isDateTime(input.arrival)) return Result::fail("Arrival must use YYYY-MM-DD HH:MM.");

    input.status = ParcelStatus::Pending;
    Result inserted = organisation.insert(input);
    if (!inserted.success) return inserted;

    // Keep generated IDs above any ID supplied by a CSV file or a Task 5 insert.
    int number = std::stoi(input.id.substr(1));
    if (number >= nextNumber) nextNumber = number + 1;

    // An undone registration leaves a stale entry behind, which this ID can reuse.
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

Result ParcelModule::processNext(const std::string& staffId, std::string& outId, int& skippedStale) {
    skippedStale = 0;
    std::string parcelId;
    while (pending.dequeue(parcelId)) {
        // A parcel may have been cancelled, removed or reported missing while it waited.
        Parcel* parcel = organisation.find(parcelId);
        if (parcel == nullptr || parcel->status != ParcelStatus::Pending) {
            skippedStale++;
            continue;
        }
        Result changed = changeStatus(parcelId, ParcelStatus::Processing, staffId);
        if (!changed.success) return changed;
        outId = parcelId;
        return Result::ok("Parcel " + parcelId + " selected for processing.");
    }
    return Result::fail("There are no pending parcels to process.");
}

bool ParcelModule::canTransition(ParcelStatus from, ParcelStatus to) {
    if (isFinalStatus(from) || from == to) return false;
    if (to == ParcelStatus::Cancelled || to == ParcelStatus::Missing) return true;

    switch (from) {
        case ParcelStatus::Received:
            return to == ParcelStatus::Pending;
        case ParcelStatus::Pending:
            return to == ParcelStatus::Processing;
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

Result ParcelModule::changeStatus(const std::string& parcelId, ParcelStatus next, const std::string& staffId,
                                  const StatusChangeOptions& options) {
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
    // Once a parcel has physically left, the action can't be taken back.
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

Result ParcelModule::updateFields(const std::string& parcelId, Parcel changed, const std::string& staffId) {
    Parcel* parcel = organisation.find(parcelId);
    if (parcel == nullptr) return organisation.update(parcelId, changed);
    if (isFinalStatus(parcel->status)) {
        return Result::fail("Parcel " + parcelId + " is " + toString(parcel->status) + " and can't be updated.");
    }

    Parcel before = *parcel;
    changed.recipientId = Validation::toUpper(Validation::trim(changed.recipientId));
    changed.recipientName = Validation::trim(changed.recipientName);
    changed.deliveryCompany = Validation::trim(changed.deliveryCompany);
    changed.status = before.status;  // status only changes through changeStatus
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

Result ParcelModule::restoreSnapshot(const Parcel& saved) {
    Parcel* parcel = organisation.find(saved.id);
    if (parcel == nullptr) return Result::fail("Parcel " + saved.id + " no longer exists.");
    *parcel = saved;
    if (saved.status == ParcelStatus::Pending && !isQueued(saved.id)) pending.enqueue(saved.id);
    return Result::ok("Parcel " + saved.id + " restored.");
}

bool ParcelModule::isQueued(const std::string& parcelId) const {
    bool found = false;
    pending.forEach([&](const std::string& queuedId) {
        if (queuedId == parcelId) found = true;
    });
    return found;
}
