#pragma once
#include <string>
#include "core/DateTime.hpp"
#include "core/Models.hpp"
#include "core/Result.hpp"
#include "core/Validation.hpp"
#include "ds/LinkedList.hpp"
#include "ds/Stack.hpp"

// linked list is the permanent log, ordered by time order;
// stack holds only the IDs of activities that can still be undone.

class ActivityModule {
public:
    // Assigns the ID, sequence and time, then returns the new Activity ID.
    std::string record(Activity activity) {
        activity.id = Validation::formatId("ACT", nextNumber, 4);
        activity.sequence = nextNumber;
        activity.time = DateTime::now();
        nextNumber++;
        log.append(activity);
        if (activity.reversible) undoStack.push(activity.id);
        return activity.id;
    }

    Activity* find(const std::string& activityId) {
        return log.find([&](const Activity& activity) { return activity.id == activityId; });
    }

    bool isEmpty() const { return log.isEmpty(); }

    // Oldest first, limited to the most recent `limit` activities.
    template <typename Fn>
    void forEachRecent(int limit, Fn fn) {
        int toSkip = log.size() - limit;
        log.forEach([&](Activity& activity) {
            if (toSkip > 0) toSkip--;
            else fn(activity);
        });
    }

    template <typename Fn>
    void forEachForParcel(const std::string& parcelId, Fn fn) {
        log.forEach([&](Activity& activity) { if (activity.parcelId == parcelId) fn(activity); });
    }

    // Returns nullptr when nothing can be undone.
    Activity* peekUndoable() {
        std::string topId;
        if (!undoStack.peek(topId)) return nullptr;
        return find(topId);
    }

    void popUndoable() {
        std::string discarded;
        undoStack.pop(discarded);
    }

    // Return True if permanent action happened to this parcel.
    bool hasPermanentAfter(const std::string& parcelId, long sequence) {
        bool found = false;
        log.forEach([&](const Activity& activity) {
            if (activity.parcelId == parcelId && activity.sequence > sequence &&
                !activity.reversible && activity.type != ActivityType::Undo) {
                found = true;
            }
        });
        return found;
    }

private:
    LinkedList<Activity> log;
    Stack<std::string> undoStack;
    int nextNumber = 1;
};
