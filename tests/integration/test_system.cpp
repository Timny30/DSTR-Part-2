// Exercises the rules and exceptions in the design spec through ParcelSystem.
#include "ParcelSystem.hpp"
#include "test_util.hpp"
#include <string>

namespace {

Parcel makeParcel(const std::string& recipientId = "TP012345", const std::string& name = "Amy Chan") {
    Parcel parcel;
    parcel.recipientId = recipientId;
    parcel.recipientName = name;
    parcel.deliveryCompany = "Pos Laju";
    return parcel;
}

// Registers a parcel, which then waits in the queue for Task 1 to route it.
std::string startParcel(ParcelSystem& sys, const std::string& recipientId = "TP012345") {
    std::string id;
    sys.parcels().registerNew(makeParcel(recipientId), "S1", id);
    return id;
}

// Task 1 chooses the route, then Task 2 creates the request. Hold creates no request.
Result routeAndRequest(ParcelSystem& sys, const std::string& parcelId, ParcelRoute route, bool urgent,
                       std::string& outRequestId) {
    Result routed = sys.parcels().routeParcel(parcelId, route, sys.staffId());
    ServiceType type;
    if (!routed.success || !ServiceModule::typeForRoute(route, type)) return routed;
    return sys.createRequest(parcelId, type, urgent, outRequestId);
}

std::string addResource(ParcelSystem& sys, ResourceType type) {
    std::string id;
    sys.resources().add(type, "Block A", id);
    return id;
}

ParcelStatus statusOf(ParcelSystem& sys, const std::string& parcelId) { return sys.parcels().find(parcelId)->status; }

std::string topActivity(ParcelSystem& sys) {
    Activity* top = sys.activities().peekUndoable();
    return top == nullptr ? "" : top->id;
}

void testRegistration() {
    ParcelSystem sys;
    std::string id;
    CHECK(!sys.parcels().registerNew(makeParcel(""), "S1", id).success);
    CHECK(!sys.parcels().registerNew(makeParcel("A!"), "S1", id).success);
    CHECK(!sys.parcels().registerNew(makeParcel("TP012345", ""), "S1", id).success);
    CHECK(!sys.parcels().registerNew(makeParcel("TP012345", "Amy123"), "S1", id).success);
    Parcel noCompany = makeParcel();
    noCompany.deliveryCompany = "   ";
    CHECK(!sys.parcels().registerNew(noCompany, "S1", id).success);
    CHECK(sys.organisation().isEmpty());

    CHECK(sys.parcels().registerNew(makeParcel("tp012345", "Amy Mei Chan"), "S1", id).success);
    CHECK(id == "P0001");
    CHECK(sys.parcels().find(id)->recipientId == "TP012345");
    CHECK(sys.parcels().find(id)->recipientName == "Amy Mei Chan");
    CHECK(statusOf(sys, id) == ParcelStatus::Pending);

    // Supplied IDs: invalid and duplicate are rejected, and generated IDs jump past them.
    Parcel explicitId = makeParcel();
    explicitId.id = "BAD";
    CHECK(!sys.parcels().registerWithId(explicitId, "S1").success);
    explicitId.id = "P0000";
    CHECK(!sys.parcels().registerWithId(explicitId, "S1").success);
    explicitId.id = "P0001";
    CHECK(!sys.parcels().registerWithId(explicitId, "S1").success);
    explicitId.id = "P0050";
    CHECK(sys.parcels().registerWithId(explicitId, "S1").success);
    CHECK(sys.parcels().registerNew(makeParcel(), "S1", id).success);
    CHECK(id == "P0051");
}

void testProcessing() {
    ParcelSystem sys;
    std::string out;
    int skipped = 0;
    CHECK(!sys.parcels().peekNext(out, skipped).success);  // empty collection

    std::string first, second, third;
    sys.parcels().registerNew(makeParcel(), "S1", first);
    sys.parcels().registerNew(makeParcel(), "S1", second);
    sys.parcels().registerNew(makeParcel(), "S1", third);

    // A cancelled parcel is skipped, and order is otherwise first in, first out.
    CHECK(sys.cancelParcel(first).success);
    CHECK(sys.parcels().peekNext(out, skipped).success);
    CHECK(out == second);
    CHECK(skipped == 1);

    // Looking at the next parcel doesn't remove it, so deciding later can't lose it.
    CHECK(sys.parcels().peekNext(out, skipped).success);
    CHECK(out == second);
    CHECK(skipped == 0);
    CHECK(statusOf(sys, second) == ParcelStatus::Pending);

    // Parcels are routed in arrival order.
    CHECK(!sys.parcels().routeParcel(third, ParcelRoute::Delivery, "S1").success);
    CHECK(sys.parcels().routeParcel(second, ParcelRoute::Delivery, "S1").success);
    CHECK(sys.parcels().peekNext(out, skipped).success);
    CHECK(out == third);
    CHECK(sys.parcels().routeParcel(third, ParcelRoute::Collection, "S1").success);
    CHECK(!sys.parcels().peekNext(out, skipped).success);
}

void testRouting() {
    ParcelSystem sys;
    std::string id = startParcel(sys);
    std::string req;

    // Task 1 only saves the route. The parcel doesn't reach Processing here.
    CHECK(!sys.parcels().routeParcel(id, ParcelRoute::None, "S1").success);
    CHECK(!sys.parcels().routeParcel("P9999", ParcelRoute::Delivery, "S1").success);
    CHECK(!sys.createRequest(id, ServiceType::CampusDelivery, false, req).success);  // no route yet
    CHECK(sys.parcels().routeParcel(id, ParcelRoute::Delivery, "S1").success);
    CHECK(statusOf(sys, id) == ParcelStatus::Pending);
    CHECK(sys.parcels().find(id)->route == ParcelRoute::Delivery);
    CHECK(sys.services().activeForParcel(id) == nullptr);

    // A route can change until the request exists, and the request must match it.
    CHECK(sys.parcels().routeParcel(id, ParcelRoute::Collection, "S1").success);
    CHECK(!sys.createRequest(id, ServiceType::CampusDelivery, false, req).success);
    CHECK(statusOf(sys, id) == ParcelStatus::Pending);

    // Creating the request is what moves the parcel to Processing.
    CHECK(sys.createRequest(id, ServiceType::RecipientCollection, true, req).success);
    CHECK(statusOf(sys, id) == ParcelStatus::Processing);
    CHECK(sys.services().find(req)->score == 70);
    CHECK(!sys.parcels().routeParcel(id, ParcelRoute::Delivery, "S1").success);

    // Hold goes straight to On Hold, and a cancelled parcel can't be routed.
    ParcelSystem other;
    std::string held = startParcel(other);
    CHECK(other.parcels().routeParcel(held, ParcelRoute::Hold, "S1").success);
    CHECK(statusOf(other, held) == ParcelStatus::OnHold);
    std::string gone = startParcel(other);
    CHECK(other.cancelParcel(gone).success);
    CHECK(!other.parcels().routeParcel(gone, ParcelRoute::Delivery, "S1").success);

    // A parcel reported missing while waiting can't be routed, and Task 1 skips it.
    ParcelSystem queued;
    std::string missing = startParcel(queued);
    std::string waiting = startParcel(queued);
    CHECK(queued.reportMissing(missing).success);
    CHECK(!queued.parcels().routeParcel(missing, ParcelRoute::Delivery, "S1").success);
    std::string out;
    int skipped = 0;
    CHECK(queued.parcels().peekNext(out, skipped).success);
    CHECK(out == waiting);
    CHECK(skipped == 1);
    int pendingCount = 0;
    queued.parcels().forEachPending([&](const Parcel&) { pendingCount++; });
    CHECK(pendingCount == 1);
}

void testUndoRouting() {
    ParcelSystem sys;
    std::string first = startParcel(sys);
    std::string second = startParcel(sys);
    CHECK(sys.parcels().routeParcel(first, ParcelRoute::Delivery, "S1").success);

    // Undoing a route puts the parcel back in the queue, at the back.
    CHECK(sys.undo(topActivity(sys)).success);
    CHECK(sys.parcels().find(first)->route == ParcelRoute::None);
    CHECK(statusOf(sys, first) == ParcelStatus::Pending);
    std::string out;
    int skipped = 0;
    CHECK(sys.parcels().peekNext(out, skipped).success);
    CHECK(out == second);

    // Undoing a change of route restores the old route, without joining the queue again.
    CHECK(sys.parcels().routeParcel(second, ParcelRoute::Delivery, "S1").success);
    CHECK(sys.parcels().routeParcel(second, ParcelRoute::Return, "S1").success);
    CHECK(sys.undo(topActivity(sys)).success);
    CHECK(sys.parcels().find(second)->route == ParcelRoute::Delivery);
    CHECK(sys.parcels().peekNext(out, skipped).success);
    CHECK(out == first);

    // Undoing Hold returns the parcel to the queue as Pending.
    CHECK(sys.parcels().routeParcel(first, ParcelRoute::Hold, "S1").success);
    CHECK(statusOf(sys, first) == ParcelStatus::OnHold);
    CHECK(sys.undo(topActivity(sys)).success);
    CHECK(statusOf(sys, first) == ParcelStatus::Pending);
    CHECK(sys.parcels().peekNext(out, skipped).success);
    CHECK(out == first);
}

void testFailureOutcomes() {
    struct Case {
        ParcelRoute route;
        ResourceType resource;
        ParcelStatus expected;
    };
    Case cases[] = {
        {ParcelRoute::Delivery, ResourceType::DeliveryPersonnel, ParcelStatus::ReadyForCollection},
        {ParcelRoute::Collection, ResourceType::CollectionCounter, ParcelStatus::OnHold},
        {ParcelRoute::SpecialHandling, ResourceType::SpecialHandlingCounter, ParcelStatus::OnHold},
        {ParcelRoute::Return, ResourceType::DeliveryDesk, ParcelStatus::OnHold},
    };
    for (const Case& test : cases) {
        ParcelSystem sys;
        std::string resource = addResource(sys, test.resource);
        std::string req, res;
        std::string p = startParcel(sys);
        CHECK(routeAndRequest(sys, p, test.route, false, req).success);
        std::string request = req;
        CHECK(!sys.recordFailure(request).success);  // no resource yet
        CHECK(sys.handleNextRequest(req, res).success);
        ParcelStatus beforeFailure = statusOf(sys, p);

        CHECK(sys.recordFailure(request).success);
        CHECK(statusOf(sys, p) == test.expected);
        CHECK(sys.services().find(request)->status == RequestStatus::Cancelled);
        CHECK(!sys.services().find(request)->note.empty());
        CHECK(sys.resources().find(resource)->status == ResourceStatus::Available);
        CHECK(sys.resources().find(resource)->transactions == 0);
        CHECK(!sys.recordFailure(request).success);  // already closed

        // A failure can be undone while the resource is still free.
        CHECK(sys.undo(topActivity(sys)).success);
        CHECK(statusOf(sys, p) == beforeFailure);
        CHECK(sys.services().find(request)->status == RequestStatus::Processing);
        CHECK(sys.resources().find(resource)->currentRequestId == request);
    }
    ParcelSystem empty;
    CHECK(!empty.recordFailure("REQ9999").success);
}

void testStatusRules() {
    CHECK(ParcelModule::canTransition(ParcelStatus::Processing, ParcelStatus::ReadyForCollection));
    CHECK(ParcelModule::canTransition(ParcelStatus::OutForDelivery, ParcelStatus::Delivered));
    CHECK(ParcelModule::canTransition(ParcelStatus::Missing, ParcelStatus::OnHold));
    CHECK(ParcelModule::canTransition(ParcelStatus::Pending, ParcelStatus::Missing));
    CHECK(ParcelModule::canTransition(ParcelStatus::Pending, ParcelStatus::OnHold));
    CHECK(!ParcelModule::canTransition(ParcelStatus::Pending, ParcelStatus::Collected));
    CHECK(!ParcelModule::canTransition(ParcelStatus::ReadyForCollection, ParcelStatus::Delivered));
    CHECK(!ParcelModule::canTransition(ParcelStatus::Collected, ParcelStatus::Processing));
    CHECK(!ParcelModule::canTransition(ParcelStatus::Delivered, ParcelStatus::Missing));
    CHECK(!ParcelModule::canTransition(ParcelStatus::Cancelled, ParcelStatus::Pending));
    CHECK(!ParcelModule::canTransition(ParcelStatus::Missing, ParcelStatus::Missing));

    ParcelSystem sys;
    std::string id = startParcel(sys);
    CHECK(sys.reportMissing(id).success);
    CHECK(!sys.reportMissing(id).success);
    CHECK(sys.markFound(id).success);
    CHECK(statusOf(sys, id) == ParcelStatus::OnHold);
    CHECK(!sys.markFound(id).success);
    CHECK(sys.cancelParcel(id).success);
    CHECK(!sys.cancelParcel(id).success);
    CHECK(!sys.reportMissing("P9999").success);
}

void testAllRoutes() {
    ParcelSystem sys;
    std::string counter = addResource(sys, ResourceType::CollectionCounter);
    std::string rider = addResource(sys, ResourceType::DeliveryPersonnel);
    std::string room = addResource(sys, ResourceType::SpecialHandlingCounter);
    std::string desk = addResource(sys, ResourceType::DeliveryDesk);
    std::string req, res;

    // Collection, including a wrong recipient.
    std::string p1 = startParcel(sys);
    CHECK(routeAndRequest(sys, p1, ParcelRoute::Collection, false, req).success);
    CHECK(statusOf(sys, p1) == ParcelStatus::Processing);
    CHECK(!sys.completeRequest(req, "TP012345").success);  // no resource yet
    CHECK(sys.handleNextRequest(req, res).success);
    CHECK(res == counter);
    CHECK(sys.resources().find(counter)->status == ResourceStatus::Occupied);
    CHECK(sys.resources().find(counter)->currentParcelId == p1);
    CHECK(sys.services().find(req)->status == RequestStatus::Processing);
    CHECK(!sys.completeRequest(req, "WRONG123").success);
    CHECK(statusOf(sys, p1) == ParcelStatus::ReadyForCollection);
    CHECK(sys.completeRequest(req, "tp012345").success);
    CHECK(statusOf(sys, p1) == ParcelStatus::Collected);
    CHECK(sys.services().find(req)->status == RequestStatus::Completed);
    CHECK(sys.resources().find(counter)->status == ResourceStatus::Available);
    CHECK(sys.resources().find(counter)->transactions == 1);
    CHECK(!sys.completeRequest(req, "TP012345").success);  // completed requests can't run again
    CHECK(!sys.cancelRequest(req).success);

    // Delivery.
    std::string p2 = startParcel(sys);
    CHECK(routeAndRequest(sys, p2, ParcelRoute::Delivery, false, req).success);
    CHECK(statusOf(sys, p2) == ParcelStatus::Processing);
    CHECK(sys.handleNextRequest(req, res).success);
    CHECK(res == rider);
    CHECK(statusOf(sys, p2) == ParcelStatus::OutForDelivery);
    CHECK(sys.completeRequest(req, "").success);
    CHECK(statusOf(sys, p2) == ParcelStatus::Delivered);

    // Special handling ends at Ready for Collection, and a follow-up request is allowed.
    std::string p3 = startParcel(sys);
    CHECK(routeAndRequest(sys, p3, ParcelRoute::SpecialHandling, false, req).success);
    CHECK(sys.parcels().find(p3)->serviceType == ServiceType::SpecialHandling);
    CHECK(sys.handleNextRequest(req, res).success);
    CHECK(res == room);
    CHECK(sys.completeRequest(req, "").success);
    CHECK(statusOf(sys, p3) == ParcelStatus::ReadyForCollection);
    CHECK(sys.createRequest(p3, ServiceType::CampusDelivery, false, req).success);

    // Return.
    std::string p4 = startParcel(sys);
    CHECK(routeAndRequest(sys, p4, ParcelRoute::Return, false, req).success);
    std::string returnReq = req;
    CHECK(sys.cancelRequest(sys.services().activeForParcel(p3)->id).success);  // clears p3 so return is next
    CHECK(sys.handleNextRequest(req, res).success);
    CHECK(req == returnReq);
    CHECK(res == desk);
    CHECK(sys.completeRequest(req, "").success);
    CHECK(statusOf(sys, p4) == ParcelStatus::Returned);
    CHECK(sys.services().find(req)->status == RequestStatus::Returned);

    // Hold creates no request, and only a Pending parcel can be routed.
    std::string p5 = startParcel(sys);
    CHECK(routeAndRequest(sys, p5, ParcelRoute::Hold, false, req).success);
    CHECK(statusOf(sys, p5) == ParcelStatus::OnHold);
    CHECK(sys.services().activeForParcel(p5) == nullptr);
    CHECK(!routeAndRequest(sys, p5, ParcelRoute::Collection, false, req).success);
    CHECK(!routeAndRequest(sys, "P9999", ParcelRoute::Collection, false, req).success);

    // A held parcel can get a request, which moves it on.
    CHECK(sys.createRequest(p5, ServiceType::RecipientCollection, false, req).success);
    CHECK(statusOf(sys, p5) == ParcelStatus::Processing);
}

void testRequestRules() {
    ParcelSystem sys;
    std::string req;
    ServiceRequest* next = nullptr;
    CHECK(!sys.services().peekNext(next).success);
    CHECK(!sys.createRequest("P9999", ServiceType::CampusDelivery, false, req).success);
    CHECK(!sys.completeRequest("REQ9999", "").success);
    CHECK(!sys.cancelRequest("REQ9999").success);

    std::string pendingId;
    sys.parcels().registerNew(makeParcel(), "S1", pendingId);
    CHECK(!sys.createRequest(pendingId, ServiceType::CampusDelivery, false, req).success);  // still queued
    CHECK(statusOf(sys, pendingId) == ParcelStatus::Pending);

    CHECK(sys.parcels().routeParcel(pendingId, ParcelRoute::Delivery, "S1").success);
    CHECK(sys.createRequest(pendingId, ServiceType::CampusDelivery, false, req).success);
    CHECK(!sys.createRequest(pendingId, ServiceType::CampusDelivery, false, req).success);  // one active request
    CHECK(sys.services().find(req)->score == 20);
    CHECK(ServiceModule::scoreFor(ServiceType::SpecialHandling, false) == 40);
    CHECK(ServiceModule::scoreFor(ServiceType::ReDelivery, false) == 30);
    CHECK(ServiceModule::scoreFor(ServiceType::ReturnToSender, true) == 60);
}

void testPriorityOrder() {
    ParcelSystem sys;
    std::string returnReq, collectionReq, deliveryReq, specialReq, urgentReq;
    routeAndRequest(sys, startParcel(sys), ParcelRoute::Return, false, returnReq);
    routeAndRequest(sys, startParcel(sys), ParcelRoute::Collection, false, collectionReq);
    routeAndRequest(sys, startParcel(sys), ParcelRoute::Delivery, false, deliveryReq);
    routeAndRequest(sys, startParcel(sys), ParcelRoute::SpecialHandling, false, specialReq);
    routeAndRequest(sys, startParcel(sys), ParcelRoute::Return, true, urgentReq);

    // A cancelled request is skipped when it reaches the top.
    CHECK(sys.cancelRequest(specialReq).success);

    std::string expected[] = {urgentReq, collectionReq, deliveryReq, returnReq};  // tie: earlier first
    for (const std::string& id : expected) {
        ServiceRequest* next = nullptr;
        CHECK(sys.services().peekNext(next).success);
        CHECK(next != nullptr && next->id == id);
        sys.services().popNext();
    }
    ServiceRequest* none = nullptr;
    CHECK(!sys.services().peekNext(none).success);
}

void testResourceRules() {
    ParcelSystem sys;
    std::string req, res;

    // No suitable resource: the request keeps its place.
    std::string p1 = startParcel(sys);
    routeAndRequest(sys, p1, ParcelRoute::Collection, false, req);
    std::string firstReq = req;
    CHECK(!sys.handleNextRequest(req, res).success);
    ServiceRequest* next = nullptr;
    CHECK(sys.services().peekNext(next).success && next->id == firstReq);
    CHECK(next->status == RequestStatus::Pending);

    std::string a = addResource(sys, ResourceType::CollectionCounter);
    std::string b = addResource(sys, ResourceType::CollectionCounter);
    std::string desk = addResource(sys, ResourceType::DeliveryDesk);

    std::string p2 = startParcel(sys);
    routeAndRequest(sys, p2, ParcelRoute::Collection, false, req);
    std::string secondReq = req;
    std::string p3 = startParcel(sys);
    routeAndRequest(sys, p3, ParcelRoute::Collection, false, req);
    std::string thirdReq = req;

    // Chosen resource errors.
    CHECK(!sys.handleNextRequestWith("RES999", req).success);
    CHECK(!sys.handleNextRequestWith(desk, req).success);
    CHECK(sys.setResourceStatus(a, ResourceStatus::Maintenance).success);
    CHECK(!sys.handleNextRequestWith(a, req).success);

    // Automatic allocation skips the unavailable resource.
    CHECK(sys.handleNextRequest(req, res).success);
    CHECK(req == firstReq);
    CHECK(res == b);
    CHECK(sys.setResourceStatus(a, ResourceStatus::Available).success);
    CHECK(sys.handleNextRequest(req, res).success);
    CHECK(res == a);

    // Everything busy: the third request waits.
    CHECK(!sys.handleNextRequest(req, res).success);
    CHECK(sys.services().peekNext(next).success && next->id == thirdReq);

    // Duplicate and invalid updates.
    CHECK(!sys.resources().assign(ServiceType::RecipientCollection, secondReq, p2, res).success);
    CHECK(!sys.setResourceStatus("RES999", ResourceStatus::Available).success);
    CHECK(!sys.setResourceStatus(a, ResourceStatus::Available).success);  // holds a job
    CHECK(!sys.setResourceStatus(a, ResourceStatus::Occupied).success);   // already occupied
    CHECK(!sys.resources().release(desk, false).success);                 // no assignment
    CHECK(!sys.resources().release("RES999", false).success);

    // A resource marked Occupied by hand can be set back, and isn't used for assignments.
    CHECK(sys.setResourceStatus(desk, ResourceStatus::Occupied).success);
    CHECK(sys.setResourceStatus(desk, ResourceStatus::Available).success);

    // Round robin: after both finish, work goes to each in turn.
    CHECK(sys.completeRequest(firstReq, "TP012345").success);
    CHECK(sys.completeRequest(secondReq, "TP012345").success);
    CHECK(sys.handleNextRequest(req, res).success);
    CHECK(req == thirdReq);
    std::string firstPick = res;
    CHECK(sys.completeRequest(thirdReq, "TP012345").success);
    routeAndRequest(sys, startParcel(sys), ParcelRoute::Collection, false, req);
    CHECK(sys.handleNextRequest(req, res).success);
    CHECK(res != firstPick);

    // The group has a fixed capacity.
    ParcelSystem full;
    std::string id;
    for (int i = 0; i < 16; i++) CHECK(full.resources().add(ResourceType::DeliveryDesk, "", id).success);
    CHECK(!full.resources().add(ResourceType::DeliveryDesk, "", id).success);
    CHECK(full.resources().add(ResourceType::CollectionCounter, "", id).success);
}

void testResourceLost() {
    ParcelSystem sys;
    std::string rider = addResource(sys, ResourceType::DeliveryPersonnel);
    std::string req, res, earlierReq;

    std::string p1 = startParcel(sys);
    routeAndRequest(sys, p1, ParcelRoute::Delivery, false, earlierReq);
    std::string p2 = startParcel(sys);
    routeAndRequest(sys, p2, ParcelRoute::Delivery, false, req);

    CHECK(sys.handleNextRequest(req, res).success);
    CHECK(req == earlierReq);
    CHECK(sys.setResourceStatus(rider, ResourceStatus::Unavailable).success);
    CHECK(sys.services().find(earlierReq)->status == RequestStatus::Pending);
    CHECK(sys.services().find(earlierReq)->resourceId.empty());
    CHECK(statusOf(sys, p1) == ParcelStatus::Processing);
    CHECK(sys.resources().find(rider)->currentRequestId.empty());
    CHECK(sys.resources().find(rider)->transactions == 0);

    // The request kept its original place, ahead of the later one.
    ServiceRequest* next = nullptr;
    CHECK(sys.services().peekNext(next).success && next->id == earlierReq);
    CHECK(!sys.handleNextRequest(req, res).success);
}

void testFailedDeliveryAndCancel() {
    ParcelSystem sys;
    std::string rider = addResource(sys, ResourceType::DeliveryPersonnel);
    std::string req, res;

    std::string p1 = startParcel(sys);
    routeAndRequest(sys, p1, ParcelRoute::Delivery, false, req);
    std::string delivery = req;
    CHECK(!sys.recordFailure(delivery).success);  // not out yet
    sys.handleNextRequest(req, res);
    CHECK(sys.recordFailure(delivery).success);
    CHECK(statusOf(sys, p1) == ParcelStatus::ReadyForCollection);
    CHECK(sys.services().find(delivery)->status == RequestStatus::Cancelled);
    CHECK(sys.resources().find(rider)->status == ResourceStatus::Available);
    CHECK(!sys.recordFailure(delivery).success);
    CHECK(!sys.recordFailure("REQ9999").success);

    CHECK(sys.createRequest(p1, ServiceType::ReDelivery, false, req).success);
    CHECK(sys.services().find(req)->score == 30);

    // Cancelling while assigned frees the resource and holds the parcel.
    std::string redelivery = req;
    sys.handleNextRequest(req, res);
    CHECK(sys.cancelRequest(redelivery).success);
    CHECK(statusOf(sys, p1) == ParcelStatus::OnHold);
    CHECK(sys.resources().find(rider)->status == ResourceStatus::Available);

    // Cancelling a parcel closes its request and frees the resource.
    CHECK(sys.createRequest(p1, ServiceType::CampusDelivery, false, req).success);
    std::string third = req;
    sys.handleNextRequest(req, res);
    CHECK(sys.cancelParcel(p1).success);
    CHECK(statusOf(sys, p1) == ParcelStatus::Cancelled);
    CHECK(sys.services().find(third)->status == RequestStatus::Cancelled);
    CHECK(sys.resources().find(rider)->status == ResourceStatus::Available);

    // Reporting a parcel missing does the same.
    std::string p2 = startParcel(sys);
    routeAndRequest(sys, p2, ParcelRoute::Delivery, false, req);
    std::string fourth = req;
    CHECK(sys.reportMissing(p2).success);
    CHECK(sys.services().find(fourth)->status == RequestStatus::Cancelled);
}

void testUndoBasics() {
    ParcelSystem sys;
    CHECK(!sys.undo("ACT0001").success);  // no history
    CHECK(sys.activities().isEmpty());

    std::string id;
    sys.parcels().registerNew(makeParcel(), "S1", id);
    CHECK(sys.undo(topActivity(sys)).success);
    CHECK(sys.parcels().find(id) == nullptr);
    CHECK(!sys.undo("ACT0001").success);  // already reversed
    CHECK(!sys.undo("ACT0002").success);  // the undo itself can't be undone
    CHECK(!sys.undo("BAD").success);
    CHECK(topActivity(sys).empty());

    std::string out;
    int skipped = 0;
    CHECK(!sys.parcels().peekNext(out, skipped).success);  // the queue entry is stale
    CHECK(skipped == 1);
}

void testUndoAssignment() {
    ParcelSystem sys;
    std::string counter = addResource(sys, ResourceType::CollectionCounter);
    std::string req, res;
    std::string p = startParcel(sys);
    routeAndRequest(sys, p, ParcelRoute::Collection, false, req);
    std::string request = req;
    sys.handleNextRequest(req, res);

    CHECK(sys.undo(topActivity(sys)).success);
    CHECK(sys.services().find(request)->status == RequestStatus::Pending);
    CHECK(sys.services().find(request)->resourceId.empty());
    CHECK(sys.resources().find(counter)->status == ResourceStatus::Available);
    CHECK(sys.resources().find(counter)->currentRequestId.empty());
    CHECK(statusOf(sys, p) == ParcelStatus::Processing);

    // The request can be handled again.
    CHECK(sys.handleNextRequest(req, res).success);
    CHECK(req == request);

    // Delivery assignment also restores the parcel's earlier status.
    std::string rider = addResource(sys, ResourceType::DeliveryPersonnel);
    std::string d = startParcel(sys);
    routeAndRequest(sys, d, ParcelRoute::Delivery, false, req);
    std::string delivery = req;
    sys.handleNextRequest(req, res);
    CHECK(statusOf(sys, d) == ParcelStatus::OutForDelivery);
    CHECK(sys.undo(topActivity(sys)).success);
    CHECK(statusOf(sys, d) == ParcelStatus::Processing);
    CHECK(sys.resources().find(rider)->status == ResourceStatus::Available);
    CHECK(sys.services().find(delivery)->status == RequestStatus::Pending);
}

void testUndoCancellation() {
    // A pending request is restored with exactly one heap entry.
    {
        ParcelSystem sys;
        std::string req;
        std::string p = startParcel(sys);
        routeAndRequest(sys, p, ParcelRoute::Collection, false, req);
        std::string request = req;
        sys.cancelRequest(request);
        CHECK(statusOf(sys, p) == ParcelStatus::OnHold);
        CHECK(sys.undo(topActivity(sys)).success);
        CHECK(sys.services().find(request)->status == RequestStatus::Pending);
        CHECK(statusOf(sys, p) == ParcelStatus::Processing);

        ServiceRequest* next = nullptr;
        CHECK(sys.services().peekNext(next).success && next->id == request);
        sys.services().popNext();
        CHECK(!sys.services().peekNext(next).success);
    }
    // An assigned request gets its resource back, but only if that resource is free.
    {
        ParcelSystem sys;
        std::string counter = addResource(sys, ResourceType::CollectionCounter);
        std::string req, res;
        std::string p = startParcel(sys);
        routeAndRequest(sys, p, ParcelRoute::Collection, false, req);
        std::string request = req;
        sys.handleNextRequest(req, res);
        sys.cancelRequest(request);

        sys.setResourceStatus(counter, ResourceStatus::Unavailable);
        std::string top = topActivity(sys);
        CHECK(!sys.undo(top).success);
        CHECK(topActivity(sys) == top);  // refused, not discarded
        CHECK(sys.services().find(request)->status == RequestStatus::Cancelled);
        CHECK(statusOf(sys, p) == ParcelStatus::OnHold);

        sys.setResourceStatus(counter, ResourceStatus::Available);
        CHECK(sys.undo(top).success);
        CHECK(sys.services().find(request)->status == RequestStatus::Processing);
        CHECK(sys.services().find(request)->resourceId == counter);
        CHECK(sys.resources().find(counter)->status == ResourceStatus::Occupied);
        CHECK(sys.resources().find(counter)->currentRequestId == request);

        // An undone cancellation can't leave a duplicate entry to process later.
        ServiceRequest* next = nullptr;
        CHECK(!sys.services().peekNext(next).success);
    }
    // A cancelled waiting parcel goes back in the queue.
    {
        ParcelSystem sys;
        std::string id, out;
        int skipped = 0;
        sys.parcels().registerNew(makeParcel(), "S1", id);
        sys.reportMissing(id);
        CHECK(!sys.parcels().peekNext(out, skipped).success);
        CHECK(sys.undo(topActivity(sys)).success);
        CHECK(statusOf(sys, id) == ParcelStatus::Pending);
        CHECK(sys.parcels().peekNext(out, skipped).success);
        CHECK(out == id);
    }
    // A failed delivery can be undone.
    {
        ParcelSystem sys;
        std::string rider = addResource(sys, ResourceType::DeliveryPersonnel);
        std::string req, res;
        std::string p = startParcel(sys);
        routeAndRequest(sys, p, ParcelRoute::Delivery, false, req);
        std::string request = req;
        sys.handleNextRequest(req, res);
        sys.recordFailure(request);
        CHECK(sys.undo(topActivity(sys)).success);
        CHECK(statusOf(sys, p) == ParcelStatus::OutForDelivery);
        CHECK(sys.services().find(request)->status == RequestStatus::Processing);
        CHECK(sys.resources().find(rider)->currentRequestId == request);
    }
}

void testUndoResourceLost() {
    ParcelSystem sys;
    std::string rider = addResource(sys, ResourceType::DeliveryPersonnel);
    std::string req, res;
    std::string p = startParcel(sys);
    routeAndRequest(sys, p, ParcelRoute::Delivery, false, req);
    std::string request = req;
    sys.handleNextRequest(req, res);
    sys.setResourceStatus(rider, ResourceStatus::Maintenance);

    std::string top = topActivity(sys);
    CHECK(!sys.undo(top).success);  // still in maintenance
    sys.setResourceStatus(rider, ResourceStatus::Available);
    CHECK(sys.undo(top).success);
    CHECK(sys.services().find(request)->status == RequestStatus::Processing);
    CHECK(statusOf(sys, p) == ParcelStatus::OutForDelivery);
    CHECK(sys.resources().find(rider)->currentRequestId == request);

    ServiceRequest* next = nullptr;
    CHECK(!sys.services().peekNext(next).success);  // the leftover heap entry is dropped
}

void testUndoOrderAndPermanent() {
    ParcelSystem sys;
    addResource(sys, ResourceType::CollectionCounter);
    std::string req, res;
    std::string p = startParcel(sys);
    routeAndRequest(sys, p, ParcelRoute::Collection, false, req);
    std::string request = req;
    sys.handleNextRequest(req, res);

    // Only the top of the stack is accepted.
    CHECK(!sys.undo("ACT0001").success);
    CHECK(sys.parcels().find(p) != nullptr);

    CHECK(sys.completeRequest(request, "TP012345").success);
    std::string permanentId;
    sys.activities().forEachForParcel(p, [&](const Activity& activity) { permanentId = activity.id; });
    CHECK(sys.activities().find(permanentId)->type == ActivityType::CollectionConfirmation);
    CHECK(!sys.undo(permanentId).success);

    // Older entries are discarded one at a time, and the collected parcel stays collected.
    int discarded = 0;
    while (!topActivity(sys).empty()) {
        CHECK(!sys.undo(topActivity(sys)).success);
        discarded++;
    }
    CHECK(discarded == 5);  // registration, processing, routing status, request, assignment
    CHECK(statusOf(sys, p) == ParcelStatus::Collected);
    CHECK(sys.resources().find(res)->transactions == 1);

    // Each permanent type is refused.
    ParcelSystem other;
    std::string removed = startParcel(other);
    CHECK(other.deleteParcel(removed).success);
    std::string removalId;
    other.activities().forEachRecent(1, [&](const Activity& activity) { removalId = activity.id; });
    CHECK(!other.undo(removalId).success);
    CHECK(!other.undo(topActivity(other)).success);  // discarded: the parcel no longer exists
}

void testUndoRequestCreationAndUpdate() {
    ParcelSystem sys;
    std::string req;
    std::string p = startParcel(sys);
    routeAndRequest(sys, p, ParcelRoute::Collection, false, req);
    std::string request = req;

    Parcel changed = *sys.parcels().find(p);
    changed.recipientId = "TP999999";
    changed.recipientName = "Ben Lee";
    CHECK(sys.updateParcel(p, changed).success);
    CHECK(sys.services().find(request)->recipientId == "TP999999");

    CHECK(sys.undo(topActivity(sys)).success);
    CHECK(sys.parcels().find(p)->recipientId == "TP012345");
    CHECK(sys.services().find(request)->recipientId == "TP012345");

    // Undoing creation, then the status change, gets back to the state before routing.
    CHECK(sys.undo(topActivity(sys)).success);
    CHECK(sys.services().find(request)->status == RequestStatus::Cancelled);
    CHECK(!sys.parcels().find(p)->hasService);
    CHECK(sys.undo(topActivity(sys)).success);
    CHECK(statusOf(sys, p) == ParcelStatus::Pending);
    CHECK(routeAndRequest(sys, p, ParcelRoute::Delivery, false, req).success);
}

void testOrganisationRules() {
    ParcelSystem sys;
    CHECK(!sys.deleteParcel("P0001").success);  // empty tree
    CHECK(!sys.organisation().update("P0001", Parcel()).success);
    CHECK(sys.organisation().find("P0001") == nullptr);

    std::string req;
    std::string p = startParcel(sys);
    CHECK(!sys.deleteParcel("P9999").success);
    CHECK(!sys.organisation().update("P9999", Parcel()).success);
    CHECK(!sys.updateParcel("P9999", Parcel()).success);

    // The key can't change, and invalid values are refused with the record kept.
    Parcel changed = *sys.parcels().find(p);
    changed.id = "P0002";
    CHECK(!sys.updateParcel(p, changed).success);
    changed.id = p;
    changed.recipientName = "12345";
    CHECK(!sys.updateParcel(p, changed).success);
    CHECK(sys.parcels().find(p)->recipientName == "Amy Chan");

    // An update can't change the status.
    changed = *sys.parcels().find(p);
    changed.status = ParcelStatus::Collected;
    changed.deliveryCompany = "DHL";
    CHECK(sys.updateParcel(p, changed).success);
    CHECK(statusOf(sys, p) == ParcelStatus::Pending);
    CHECK(sys.parcels().find(p)->deliveryCompany == "DHL");

    // Deletion is blocked by an active request.
    routeAndRequest(sys, p, ParcelRoute::Hold, false, req);
    CHECK(sys.createRequest(p, ServiceType::CampusDelivery, false, req).success);
    CHECK(!sys.deleteParcel(p).success);
    CHECK(sys.cancelRequest(req).success);
    CHECK(sys.deleteParcel(p).success);
    CHECK(sys.parcels().find(p) == nullptr);
    CHECK(sys.organisation().isEmpty());

    // A final parcel can't be updated.
    std::string q = startParcel(sys);
    sys.cancelParcel(q);
    changed = *sys.parcels().find(q);
    CHECK(!sys.updateParcel(q, changed).success);

    // Removing and reinserting an ID leaves the queue and undo stack usable.
    ParcelSystem again;
    Parcel input = makeParcel();
    input.id = "P0100";
    CHECK(again.parcels().registerWithId(input, "S1").success);
    CHECK(again.deleteParcel("P0100").success);
    CHECK(again.parcels().registerWithId(input, "S1").success);
    int pendingCount = 0;
    again.parcels().forEachPending([&](const Parcel&) { pendingCount++; });
    CHECK(pendingCount == 1);
    CHECK(again.undo(topActivity(again)).success);
    CHECK(again.parcels().find("P0100") == nullptr);

    // Filters and traversal.
    ParcelSystem filter;
    std::string id;
    Parcel fragile = makeParcel("TP000001");
    fragile.type = ParcelType::Fragile;
    filter.parcels().registerNew(fragile, "S1", id);
    filter.parcels().registerNew(makeParcel("TP000002"), "S1", id);
    filter.parcels().registerNew(fragile, "S1", id);
    int fragileCount = 0;
    int recipientCount = 0;
    filter.organisation().forEachByType(ParcelType::Fragile, [&](const Parcel&) { fragileCount++; });
    filter.organisation().forEachByRecipient("TP000001", [&](const Parcel&) { recipientCount++; });
    CHECK(fragileCount == 2);
    CHECK(recipientCount == 2);
}

void testActivityHistory() {
    ParcelSystem sys;
    int visits = 0;
    sys.activities().forEachForParcel("P0001", [&](const Activity&) { visits++; });
    CHECK(visits == 0);

    sys.setStaffId("STAFF7");
    std::string req;
    std::string p = startParcel(sys);
    routeAndRequest(sys, p, ParcelRoute::Collection, false, req);

    long previous = 0;
    bool ordered = true;
    int count = 0;
    ActivityType lastType = ActivityType::Undo;
    std::string lastStaff;
    std::string lastRequest;
    sys.activities().forEachForParcel(p, [&](const Activity& activity) {
        if (activity.sequence <= previous) ordered = false;
        previous = activity.sequence;
        lastType = activity.type;
        lastStaff = activity.staffId;
        lastRequest = activity.requestId;
        count++;
    });
    CHECK(ordered);
    CHECK(count == 4);
    CHECK(lastType == ActivityType::ServiceRequestCreation);
    CHECK(lastStaff == "STAFF7");
    CHECK(lastRequest == req);

    int recent = 0;
    sys.activities().forEachRecent(2, [&](const Activity&) { recent++; });
    CHECK(recent == 2);
}

void testDemoData() {
    ParcelSystem missing;
    CHECK(missing.loadDemoData("tests/integration/no_such_folder").message == "Loaded 0 parcels and 0 resources.");
    int warnings = 0;
    missing.forEachLoadWarning([&](const std::string&) { warnings++; });
    CHECK(warnings == 2);

    ParcelSystem good;
    CHECK(good.loadDemoData("data").message == "Loaded 8 parcels and 6 resources.");
    CHECK(good.parcels().find("P0005")->recipientName == "Amy Chan");
    CHECK(good.parcels().find("P0004")->type == ParcelType::SpecialHandling);
    warnings = 0;
    good.forEachLoadWarning([&](const std::string&) { warnings++; });
    CHECK(warnings == 0);

    // Queue order follows the file, while the tree sorts by ID.
    std::string first;
    int skipped = 0;
    good.parcels().peekNext(first, skipped);
    CHECK(first == "P0005");
    std::string sortedFirst;
    good.organisation().forEachInOrder([&](const Parcel& parcel) { if (sortedFirst.empty()) sortedFirst = parcel.id; });
    CHECK(sortedFirst == "P0001");
    std::string id;
    good.parcels().registerNew(makeParcel(), "S1", id);
    CHECK(id == "P0009");

    // Bad rows are reported and skipped, and the good ones still load.
    ParcelSystem bad;
    CHECK(bad.loadDemoData("tests/integration/bad_data").message == "Loaded 1 parcels and 1 resources.");
    warnings = 0;
    bad.forEachLoadWarning([&](const std::string&) { warnings++; });
    CHECK(warnings == 7);
}

}  // namespace

int main() {
    testRegistration();
    testProcessing();
    testRouting();
    testUndoRouting();
    testFailureOutcomes();
    testStatusRules();
    testAllRoutes();
    testRequestRules();
    testPriorityOrder();
    testResourceRules();
    testResourceLost();
    testFailedDeliveryAndCancel();
    testUndoBasics();
    testUndoAssignment();
    testUndoCancellation();
    testUndoResourceLost();
    testUndoOrderAndPermanent();
    testUndoRequestCreationAndUpdate();
    testOrganisationRules();
    testActivityHistory();
    testDemoData();
    return TestUtil::finish("System");
}
