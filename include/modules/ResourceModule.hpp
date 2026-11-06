#pragma once
#include <string>
#include "core/Models.hpp"
#include "core/Result.hpp"
#include "ds/CircularQueue.hpp"
#include "ds/LinkedList.hpp"

// Task 3. Master records live in a linked list. Each resource type has its own
// circular queue of IDs, and rotating through it shares work evenly.
class ResourceModule {
public:
    Result add(ResourceType type, const std::string& location, std::string& outId);
    Resource* find(const std::string& resourceId);

    static ResourceType typeFor(ServiceType service);

    // Automatic: rotates through the matching queue at most once.
    Result assign(ServiceType service, const std::string& requestId, const std::string& parcelId,
                  std::string& outResourceId);
    // Manual: staff choose the resource.
    Result assignChosen(const std::string& resourceId, ServiceType service, const std::string& requestId,
                        const std::string& parcelId);

    Result release(const std::string& resourceId, bool jobCompleted);
    Result reoccupy(const std::string& resourceId, const std::string& requestId, const std::string& parcelId);
    Result setStatus(const std::string& resourceId, ResourceStatus next);

    template <typename Fn>
    void forEach(Fn fn) { resources.forEach(fn); }

    bool isEmpty() const { return resources.isEmpty(); }

private:
    LinkedList<Resource> resources;
    CircularQueue<std::string> pools[RESOURCE_TYPE_COUNT];
    int nextNumber = 1;

    bool isRequestAssigned(const std::string& requestId);
    void occupy(Resource& resource, const std::string& requestId, const std::string& parcelId);
};
