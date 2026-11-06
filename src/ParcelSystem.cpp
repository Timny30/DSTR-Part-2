#include "ParcelSystem.hpp"
#include "core/Validation.hpp"
#include <fstream>

namespace {
    // Splits one CSV line. Fields can't contain commas. Returns the field count.
    int splitCsv(const std::string& line, std::string fields[], int maxFields) {
        int count = 0;
        size_t start = 0;
        while (count < maxFields) {
            size_t comma = line.find(',', start);
            if (comma == std::string::npos) {
                fields[count++] = Validation::trim(line.substr(start));
                return count;
            }
            fields[count++] = Validation::trim(line.substr(start, comma - start));
            start = comma + 1;
        }
        return maxFields + 1;  // more fields than the file format allows
    }

    bool isDeliveryService(ServiceType type) {
        return type == ServiceType::CampusDelivery || type == ServiceType::ReDelivery;
    }

    // The parcel status once a resource has been allocated to this service.
    ParcelStatus statusAfterAllocation(ServiceType type, ParcelStatus current) {
        switch (type) {
            case ServiceType::RecipientCollection:
                return ParcelStatus::ReadyForCollection;
            case ServiceType::CampusDelivery:
            case ServiceType::ReDelivery:
                return ParcelStatus::OutForDelivery;
            case ServiceType::SpecialHandling:
            case ServiceType::ReturnToSender:
                return current;
        }
        return current;
    }
}

ParcelSystem::ParcelSystem()
    : parcelModule(organisationModule, activityModule), serviceModule(parcelModule), currentStaffId("SYSTEM") {}

// ---------- Demo data ----------

Result ParcelSystem::loadDemoData(const std::string& dataDir) {
    int parcelCount = 0;
    int resourceCount = 0;
    loadParcels(dataDir + "/parcels.csv", parcelCount);
    loadResources(dataDir + "/resources.csv", resourceCount);
    return Result::ok("Loaded " + std::to_string(parcelCount) + " parcels and " +
                      std::to_string(resourceCount) + " resources.");
}

void ParcelSystem::loadParcels(const std::string& path, int& loaded) {
    std::ifstream file(path);
    if (!file) {
        loadWarnings.append(path + " wasn't found, so no demo parcels were loaded.");
        return;
    }
    const int FIELD_COUNT = 6;
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        lineNumber++;
        if (lineNumber == 1 || Validation::trim(line).empty()) continue;  // header or blank

        std::string fields[FIELD_COUNT];
        std::string where = path + " line " + std::to_string(lineNumber) + ": ";
        if (splitCsv(line, fields, FIELD_COUNT) != FIELD_COUNT) {
            loadWarnings.append(where + "expected " + std::to_string(FIELD_COUNT) + " fields, row skipped.");
            continue;
        }
        Parcel parcel;
        parcel.id = fields[0];
        parcel.recipientId = fields[1];
        parcel.recipientName = fields[2];
        parcel.deliveryCompany = fields[3];
        parcel.arrival = fields[5];
        if (!parseParcelType(fields[4], parcel.type)) {
            loadWarnings.append(where + "unknown parcel type, row skipped.");
            continue;
        }
        Result result = parcelModule.registerWithId(parcel, currentStaffId);
        if (result.success) loaded++;
        else loadWarnings.append(where + result.message + " Row skipped.");
    }
}

void ParcelSystem::loadResources(const std::string& path, int& loaded) {
    std::ifstream file(path);
    if (!file) {
        loadWarnings.append(path + " wasn't found, so no demo resources were loaded.");
        return;
    }
    const int FIELD_COUNT = 2;
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        lineNumber++;
        if (lineNumber == 1 || Validation::trim(line).empty()) continue;

        std::string fields[FIELD_COUNT];
        std::string where = path + " line " + std::to_string(lineNumber) + ": ";
        ResourceType type;
        if (splitCsv(line, fields, FIELD_COUNT) != FIELD_COUNT || !parseResourceType(fields[0], type)) {
            loadWarnings.append(where + "invalid resource row, row skipped.");
            continue;
        }
        std::string newId;
        Result result = resourceModule.add(type, fields[1], newId);
        if (result.success) loaded++;
        else loadWarnings.append(where + result.message + " Row skipped.");
    }
}

// ---------- Parcel flow ----------

Result ParcelSystem::updateParcel(const std::string& parcelId, const Parcel& changed) {
    Result updated = parcelModule.updateFields(parcelId, changed, currentStaffId);
    if (updated.success) serviceModule.syncRecipient(*parcelModule.find(parcelId));
    return updated;
}

Result ParcelSystem::closeParcel(const std::string& parcelId, ParcelStatus target, ActivityType type) {
    if (parcelModule.find(parcelId) == nullptr) return Result::fail("Parcel " + parcelId + " was not found.");

    StatusChangeOptions options;
    options.type = type;
    ServiceRequest* request = serviceModule.activeForParcel(parcelId);
    if (request != nullptr) {
        options.requestId = request->id;
        options.resourceId = request->resourceId;
        options.hasRequestBefore = true;
        options.requestStatusBefore = request->status;
        options.resourceIdBefore = request->resourceId;
    }
    Result changed = parcelModule.changeStatus(parcelId, target, currentStaffId, options);
    if (!changed.success) return changed;

    if (request != nullptr) {
        if (!request->resourceId.empty()) resourceModule.release(request->resourceId, false);
        serviceModule.close(*request, RequestStatus::Cancelled, "Parcel is now " + toString(target));
    }
    return changed;
}

Result ParcelSystem::reportMissing(const std::string& parcelId) {
    return closeParcel(parcelId, ParcelStatus::Missing, ActivityType::StatusChange);
}

Result ParcelSystem::cancelParcel(const std::string& parcelId) {
    return closeParcel(parcelId, ParcelStatus::Cancelled, ActivityType::Cancellation);
}

Result ParcelSystem::markFound(const std::string& parcelId) {
    Parcel* parcel = parcelModule.find(parcelId);
    if (parcel == nullptr) return Result::fail("Parcel " + parcelId + " was not found.");
    if (parcel->status != ParcelStatus::Missing) return Result::fail("Parcel " + parcelId + " isn't reported as missing.");
    return parcelModule.changeStatus(parcelId, ParcelStatus::OnHold, currentStaffId);
}

Result ParcelSystem::deleteParcel(const std::string& parcelId) {
    Parcel* parcel = parcelModule.find(parcelId);
    if (parcel == nullptr) return organisationModule.remove(parcelId);  // gives the right empty/not-found message

    ServiceRequest* active = serviceModule.activeForParcel(parcelId);
    if (active != nullptr) {
        return Result::fail("Parcel " + parcelId + " can't be removed while request " + active->id + " is active.");
    }
    Activity activity;
    activity.parcelId = parcelId;
    activity.type = ActivityType::ParcelRemoval;
    activity.previousStatus = parcel->status;
    activity.newStatus = parcel->status;
    activity.staffId = currentStaffId;
    activity.note = "Record removed";
    activity.reversible = false;
    activityModule.record(activity);
    return organisationModule.remove(parcelId);
}

// ---------- Request flow ----------

Result ParcelSystem::createRequest(const std::string& parcelId, ServiceType type, bool urgent, std::string& outRequestId) {
    Result valid = serviceModule.validateCreate(parcelId);
    if (!valid.success) return valid;

    // A parcel coming from Task 1 must get the service its route chose.
    Parcel* parcel = parcelModule.find(parcelId);
    ServiceType routedType;
    if (parcel->status == ParcelStatus::Pending && ServiceModule::typeForRoute(parcel->route, routedType) &&
        routedType != type) {
        return Result::fail("Parcel " + parcelId + " is routed to " + toString(routedType) +
                            ". Change its route in Task 1 first.");
    }

    // The status change is logged first, so undo reverses the request and then the status.
    if (parcel->status != ParcelStatus::Processing) {
        Result changed = parcelModule.changeStatus(parcelId, ParcelStatus::Processing, currentStaffId);
        if (!changed.success) return changed;
    }
    return serviceModule.create(parcelId, type, urgent, currentStaffId, outRequestId);
}

Result ParcelSystem::takeNextRequest(const std::string& chosenResourceId, std::string& outRequestId, std::string& outResourceId) {
    ServiceRequest* request = nullptr;
    Result next = serviceModule.peekNext(request);
    if (!next.success) return next;

    // The request only leaves the heap after a resource is found, so it keeps its place otherwise.
    Result assigned = chosenResourceId.empty()
        ? resourceModule.assign(request->type, request->id, request->parcelId, outResourceId)
        : resourceModule.assignChosen(chosenResourceId, request->type, request->id, request->parcelId);
    if (!assigned.success) return Result::fail("Request " + request->id + " must wait. " + assigned.message);
    if (!chosenResourceId.empty()) outResourceId = chosenResourceId;

    Parcel* parcel = parcelModule.find(request->parcelId);
    ParcelStatus target = statusAfterAllocation(request->type, parcel->status);

    StatusChangeOptions options;
    options.type = ActivityType::ResourceAssignment;
    options.requestId = request->id;
    options.resourceId = outResourceId;
    options.hasRequestBefore = true;
    options.requestStatusBefore = RequestStatus::Pending;
    ParcelStatus before = parcel->status;
    Result changed = parcelModule.changeStatus(parcel->id, target, currentStaffId, options);
    if (!changed.success) {
        resourceModule.release(outResourceId, false);
        return changed;
    }

    serviceModule.popNext();
    request->parcelStatusBeforeAssign = before;
    serviceModule.setActive(*request, RequestStatus::Processing, outResourceId);
    outRequestId = request->id;
    return Result::ok("Request " + request->id + " for parcel " + request->parcelId +
                      " assigned to resource " + outResourceId + ".");
}

Result ParcelSystem::handleNextRequest(std::string& outRequestId, std::string& outResourceId) {
    return takeNextRequest("", outRequestId, outResourceId);
}

Result ParcelSystem::handleNextRequestWith(const std::string& resourceId, std::string& outRequestId) {
    if (resourceId.empty()) return Result::fail("A Resource ID is required.");
    std::string usedResourceId;
    return takeNextRequest(resourceId, outRequestId, usedResourceId);
}

Result ParcelSystem::completeRequest(const std::string& requestId, const std::string& recipientId) {
    ServiceRequest* request = serviceModule.find(requestId);
    if (request == nullptr) return Result::fail("Request " + requestId + " was not found.");
    if (ServiceModule::isClosed(*request)) {
        return Result::fail("Request " + requestId + " is already " + toString(request->status) + ".");
    }
    if (request->status != RequestStatus::Processing) {
        return Result::fail("Request " + requestId + " has no resource yet. Process it first.");
    }

    ParcelStatus target = ParcelStatus::ReadyForCollection;
    ActivityType activityType = ActivityType::ServiceCompletion;
    RequestStatus requestStatus = RequestStatus::Completed;
    switch (request->type) {
        case ServiceType::RecipientCollection:
            if (Validation::toUpper(Validation::trim(recipientId)) != request->recipientId) {
                return Result::fail("Recipient ID doesn't match request " + requestId + ". The parcel wasn't released.");
            }
            target = ParcelStatus::Collected;
            activityType = ActivityType::CollectionConfirmation;
            break;
        case ServiceType::CampusDelivery:
        case ServiceType::ReDelivery:
            target = ParcelStatus::Delivered;
            break;
        case ServiceType::SpecialHandling:
            target = ParcelStatus::ReadyForCollection;
            break;
        case ServiceType::ReturnToSender:
            target = ParcelStatus::Returned;
            activityType = ActivityType::ReturnProcessing;
            requestStatus = RequestStatus::Returned;
            break;
    }

    StatusChangeOptions options;
    options.type = activityType;
    options.reversible = false;
    options.requestId = requestId;
    options.resourceId = request->resourceId;
    Result changed = parcelModule.changeStatus(request->parcelId, target, currentStaffId, options);
    if (!changed.success) return changed;

    resourceModule.release(request->resourceId, true);
    serviceModule.close(*request, requestStatus, "");
    return Result::ok("Request " + requestId + " finished. " + changed.message);
}

Result ParcelSystem::cancelRequest(const std::string& requestId) {
    ServiceRequest* request = serviceModule.find(requestId);
    if (request == nullptr) return Result::fail("Request " + requestId + " was not found.");
    if (ServiceModule::isClosed(*request)) {
        return Result::fail("Request " + requestId + " is already " + toString(request->status) + ".");
    }
    StatusChangeOptions options;
    options.type = ActivityType::Cancellation;
    options.requestId = requestId;
    options.resourceId = request->resourceId;
    options.hasRequestBefore = true;
    options.requestStatusBefore = request->status;
    options.resourceIdBefore = request->resourceId;
    Result changed = parcelModule.changeStatus(request->parcelId, ParcelStatus::OnHold, currentStaffId, options);
    if (!changed.success) return changed;

    if (!request->resourceId.empty()) resourceModule.release(request->resourceId, false);
    serviceModule.close(*request, RequestStatus::Cancelled, "Cancelled by staff");
    return Result::ok("Request " + requestId + " cancelled. " + changed.message);
}

Result ParcelSystem::recordFailure(const std::string& requestId) {
    ServiceRequest* request = serviceModule.find(requestId);
    if (request == nullptr) return Result::fail("Request " + requestId + " was not found.");
    if (ServiceModule::isClosed(*request)) {
        return Result::fail("Request " + requestId + " is already " + toString(request->status) + ".");
    }
    if (request->status != RequestStatus::Processing) {
        return Result::fail("Request " + requestId + " has no resource yet. Allocate one in Task 3 first.");
    }

    // A failed delivery can still be collected. Anything else needs staff to decide what's next.
    ParcelStatus target = isDeliveryService(request->type) ? ParcelStatus::ReadyForCollection : ParcelStatus::OnHold;
    std::string reason = toString(request->type) + " failed";

    StatusChangeOptions options;
    options.requestId = requestId;
    options.resourceId = request->resourceId;
    options.note = reason;
    options.hasRequestBefore = true;
    options.requestStatusBefore = RequestStatus::Processing;
    options.resourceIdBefore = request->resourceId;
    Result changed = parcelModule.changeStatus(request->parcelId, target, currentStaffId, options);
    if (!changed.success) return changed;

    resourceModule.release(request->resourceId, false);
    serviceModule.close(*request, RequestStatus::Cancelled, reason);
    return Result::ok("Failure recorded for request " + requestId + ". " + changed.message);
}

// ---------- Resource flow ----------

Result ParcelSystem::setResourceStatus(const std::string& resourceId, ResourceStatus next) {
    Resource* resource = resourceModule.find(resourceId);
    if (resource == nullptr) return Result::fail("Resource " + resourceId + " was not found.");

    bool losingJob = !resource->currentRequestId.empty() &&
                     (next == ResourceStatus::Unavailable || next == ResourceStatus::Maintenance);
    if (!losingJob) return resourceModule.setStatus(resourceId, next);

    // The resource became unavailable mid-job: its request rejoins the heap with
    // its original sequence, so it doesn't lose its place.
    ServiceRequest* request = serviceModule.find(resource->currentRequestId);
    if (request == nullptr) return Result::fail("Request " + resource->currentRequestId + " was not found.");

    StatusChangeOptions options;
    options.allowSameStatus = true;
    options.requestId = request->id;
    options.resourceId = resourceId;
    options.note = "Resource became " + toString(next);
    options.hasRequestBefore = true;
    options.requestStatusBefore = RequestStatus::Processing;
    options.resourceIdBefore = resourceId;
    Result changed = parcelModule.changeStatus(request->parcelId, request->parcelStatusBeforeAssign, currentStaffId, options);
    if (!changed.success) return changed;

    resourceModule.release(resourceId, false);
    serviceModule.setActive(*request, RequestStatus::Pending, "");
    serviceModule.pushBack(*request);
    Result updated = resourceModule.setStatus(resourceId, next);
    if (!updated.success) return updated;
    return Result::ok(updated.message + " Request " + request->id + " is waiting again.");
}

// ---------- Undo ----------

Result ParcelSystem::restoreRequestState(const Activity& activity) {
    ServiceRequest* request = serviceModule.find(activity.requestId);
    if (request == nullptr) return Result::fail("Request " + activity.requestId + " was not found.");

    if (activity.type == ActivityType::ServiceRequestCreation) {
        serviceModule.close(*request, RequestStatus::Cancelled, "Creation undone");
    } else if (activity.type == ActivityType::ResourceAssignment) {
        resourceModule.release(activity.resourceId, false);
        serviceModule.setActive(*request, RequestStatus::Pending, "");
        serviceModule.pushBack(*request);
    } else if (activity.hasRequestBefore && activity.requestStatusBefore == RequestStatus::Processing) {
        Result occupied = resourceModule.reoccupy(activity.resourceIdBefore, request->id, request->parcelId);
        if (!occupied.success) return occupied;
        serviceModule.setActive(*request, RequestStatus::Processing, activity.resourceIdBefore);
    } else if (activity.hasRequestBefore) {
        serviceModule.setActive(*request, RequestStatus::Pending, "");
        serviceModule.pushBack(*request);
    }
    return Result::ok("Request restored.");
}

void ParcelSystem::logUndo(const Activity& original, ParcelStatus statusNow, ParcelStatus statusAfter) {
    Activity entry;
    entry.parcelId = original.parcelId;
    entry.requestId = original.requestId;
    entry.resourceId = original.resourceId;
    entry.type = ActivityType::Undo;
    entry.previousStatus = statusNow;
    entry.newStatus = statusAfter;
    entry.staffId = currentStaffId;
    entry.note = "Reversed " + original.id;
    entry.reversible = false;
    activityModule.record(entry);
}

Result ParcelSystem::undo(const std::string& activityId) {
    Activity* activity = activityModule.find(activityId);
    if (activity == nullptr) return Result::fail("Activity " + activityId + " was not found.");
    if (activity->reversed) return Result::fail("Activity " + activityId + " has already been reversed.");
    if (activity->type == ActivityType::Undo) return Result::fail("An undo can't be undone.");
    if (!activity->reversible) {
        return Result::fail("Activity " + activityId + " (" + toString(activity->type) + ") is permanent.");
    }

    Activity* top = activityModule.peekUndoable();
    if (top == nullptr) return Result::fail("There is nothing to undo.");
    if (top != activity) {
        return Result::fail("Only the most recent reversible activity can be undone, which is " + top->id + ".");
    }

    // A later permanent action means this older state can no longer be restored.
    if (activityModule.hasPermanentAfter(activity->parcelId, activity->sequence)) {
        activityModule.popUndoable();
        return Result::fail("Activity " + activityId + " was discarded because parcel " + activity->parcelId +
                            " has since had a permanent action.");
    }

    Parcel* parcel = parcelModule.find(activity->parcelId);
    if (parcel == nullptr) {
        activityModule.popUndoable();
        return Result::fail("Activity " + activityId + " was discarded because parcel " +
                            activity->parcelId + " no longer exists.");
    }
    ParcelStatus statusNow = parcel->status;

    if (activity->type == ActivityType::ParcelRegistration) {
        // Reaching here means nothing later still stands: reversible activities would be
        // above this on the stack, and permanent ones were caught by the check above.
        // The queue entry stays behind and is skipped as stale.
        organisationModule.remove(activity->parcelId);
    } else {
        // Touch the request and resource first, because this step can be refused.
        if (!activity->requestId.empty()) {
            Result restored = restoreRequestState(*activity);
            if (!restored.success) return Result::fail("Undo refused, nothing changed. " + restored.message);
        }
        parcelModule.restoreSnapshot(activity->parcelBefore);
        serviceModule.syncRecipient(activity->parcelBefore);
    }

    activityModule.popUndoable();
    activity->reversed = true;
    ParcelStatus statusAfter = activity->previousStatus;
    logUndo(*activity, statusNow, statusAfter);
    return Result::ok("Activity " + activityId + " (" + toString(activity->type) + ") reversed.");
}
