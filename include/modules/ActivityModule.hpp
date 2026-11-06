#pragma once
#include <string>
#include "core/Models.hpp"
#include "core/Result.hpp"
#include "ds/LinkedList.hpp"
#include "ds/Stack.hpp"

// Task 4. Two structures with different jobs:
//  - a linked list is the permanent log, kept in time order for history and traces;
//  - a stack holds only the IDs of activities that can still be undone.
class ActivityModule {
public:
    // Assigns the ID, sequence and time, then returns the new Activity ID.
    std::string record(Activity activity);

    Activity* find(const std::string& activityId);
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

    Activity* peekUndoable();   // nullptr when nothing can be undone
    void popUndoable();

    // True if a permanent action happened to this parcel after the given sequence.
    bool hasPermanentAfter(const std::string& parcelId, long sequence);

private:
    LinkedList<Activity> log;
    Stack<std::string> undoStack;
    int nextNumber = 1;
};
