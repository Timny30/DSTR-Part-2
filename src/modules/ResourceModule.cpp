#include "modules/ResourceModule.hpp"
#include "core/Validation.hpp"

Result ResourceModule::add(ResourceType type, const std::string& location, std::string& outId) {
    CircularQueue<std::string>& pool = pools[static_cast<int>(type)];
    if (pool.isFull()) return Result::fail("The " + toString(type) + " group is full.");

    Resource resource;
    resource.id = Validation::formatId("RES", nextNumber, 3);
    resource.type = type;
    resource.location = location.empty() ? "-" : location;
    nextNumber++;

    pool.enqueue(resource.id);
    resources.append(resource);
    outId = resource.id;
    return Result::ok("Resource " + resource.id + " registered as " + toString(type) + ".");
}

Resource* ResourceModule::find(const std::string& resourceId) {
    return resources.find([&](const Resource& resource) { return resource.id == resourceId; });
}

ResourceType ResourceModule::typeFor(ServiceType service) {
    switch (service) {
        case ServiceType::RecipientCollection: return ResourceType::CollectionCounter;
        case ServiceType::CampusDelivery: return ResourceType::DeliveryPersonnel;
        case ServiceType::ReDelivery: return ResourceType::DeliveryPersonnel;
        case ServiceType::SpecialHandling: return ResourceType::SpecialHandlingCounter;
        case ServiceType::ReturnToSender: return ResourceType::DeliveryDesk;
    }
    return ResourceType::CollectionCounter;
}

bool ResourceModule::isRequestAssigned(const std::string& requestId) {
    return resources.find([&](const Resource& resource) { return resource.currentRequestId == requestId; }) != nullptr;
}

void ResourceModule::occupy(Resource& resource, const std::string& requestId, const std::string& parcelId) {
    resource.status = ResourceStatus::Occupied;
    resource.currentRequestId = requestId;
    resource.currentParcelId = parcelId;
}

Result ResourceModule::assign(ServiceType service, const std::string& requestId, const std::string& parcelId,
                              std::string& outResourceId) {
    if (isRequestAssigned(requestId)) return Result::fail("Request " + requestId + " already has a resource.");

    ResourceType type = typeFor(service);
    CircularQueue<std::string>& pool = pools[static_cast<int>(type)];
    if (pool.isEmpty()) return Result::fail("No " + toString(type) + " has been registered.");

    // One full rotation at most. The next search starts after the resource chosen here.
    for (int checked = 0; checked < pool.size(); checked++) {
        std::string candidateId;
        pool.rotate(candidateId);
        Resource* candidate = find(candidateId);
        if (candidate != nullptr && candidate->status == ResourceStatus::Available) {
            occupy(*candidate, requestId, parcelId);
            outResourceId = candidate->id;
            return Result::ok("Resource " + candidate->id + " assigned to request " + requestId + ".");
        }
    }
    return Result::fail("Every " + toString(type) + " is busy or unavailable.");
}

Result ResourceModule::assignChosen(const std::string& resourceId, ServiceType service,
                                    const std::string& requestId, const std::string& parcelId) {
    if (isRequestAssigned(requestId)) return Result::fail("Request " + requestId + " already has a resource.");
    Resource* resource = find(resourceId);
    if (resource == nullptr) return Result::fail("Resource " + resourceId + " was not found.");
    if (resource->type != typeFor(service)) {
        return Result::fail("Resource " + resourceId + " is a " + toString(resource->type) +
                            ", but this request needs a " + toString(typeFor(service)) + ".");
    }
    if (resource->status != ResourceStatus::Available) {
        return Result::fail("Resource " + resourceId + " is " + toString(resource->status) + ".");
    }
    occupy(*resource, requestId, parcelId);
    return Result::ok("Resource " + resourceId + " assigned to request " + requestId + ".");
}

Result ResourceModule::release(const std::string& resourceId, bool jobCompleted) {
    Resource* resource = find(resourceId);
    if (resource == nullptr) return Result::fail("Resource " + resourceId + " was not found.");
    if (resource->currentRequestId.empty()) return Result::fail("Resource " + resourceId + " has no assignment to release.");
    resource->status = ResourceStatus::Available;
    resource->currentRequestId.clear();
    resource->currentParcelId.clear();
    if (jobCompleted) resource->transactions++;
    return Result::ok("Resource " + resourceId + " released.");
}

Result ResourceModule::reoccupy(const std::string& resourceId, const std::string& requestId,
                                const std::string& parcelId) {
    Resource* resource = find(resourceId);
    if (resource == nullptr) return Result::fail("Resource " + resourceId + " was not found.");
    if (resource->status != ResourceStatus::Available) {
        return Result::fail("Resource " + resourceId + " is " + toString(resource->status) + ".");
    }
    occupy(*resource, requestId, parcelId);
    return Result::ok("Resource " + resourceId + " reassigned.");
}

Result ResourceModule::setStatus(const std::string& resourceId, ResourceStatus next) {
    Resource* resource = find(resourceId);
    if (resource == nullptr) return Result::fail("Resource " + resourceId + " was not found.");
    if (resource->status == next) return Result::fail("Resource " + resourceId + " is already " + toString(next) + ".");
    if (!resource->currentRequestId.empty()) {
        return Result::fail("Resource " + resourceId + " is handling request " + resource->currentRequestId +
                            ". Complete or cancel that request first.");
    }
    if (next == ResourceStatus::Occupied && resource->status != ResourceStatus::Available) {
        return Result::fail("Only an Available resource can be marked Occupied.");
    }
    resource->status = next;
    return Result::ok("Resource " + resourceId + " is now " + toString(next) + ".");
}
