#include "modules/ActivityModule.hpp"
#include "core/DateTime.hpp"
#include "core/Validation.hpp"

std::string ActivityModule::record(Activity activity) {
    activity.id = Validation::formatId("ACT", nextNumber, 4);
    activity.sequence = nextNumber;
    activity.time = DateTime::now();
    nextNumber++;
    log.append(activity);
    if (activity.reversible) undoStack.push(activity.id);
    return activity.id;
}

Activity* ActivityModule::find(const std::string& activityId) {
    return log.find([&](const Activity& activity) { return activity.id == activityId; });
}

Activity* ActivityModule::peekUndoable() {
    std::string topId;
    if (!undoStack.peek(topId)) return nullptr;
    return find(topId);
}

void ActivityModule::popUndoable() {
    std::string discarded;
    undoStack.pop(discarded);
}

bool ActivityModule::hasPermanentAfter(const std::string& parcelId, long sequence) {
    bool found = false;
    log.forEach([&](const Activity& activity) {
        if (activity.parcelId == parcelId && activity.sequence > sequence &&
            !activity.reversible && activity.type != ActivityType::Undo) {
            found = true;
        }
    });
    return found;
}
