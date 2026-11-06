// Peak load: 500 parcels through the full flow, plus the BST height comparison.
#include "ParcelSystem.hpp"
#include "ds/BST.hpp"
#include "test_util.hpp"
#include <cstdlib>
#include <ctime>
#include <string>

namespace {

void testFullFlow() {
    const int PARCELS = 500;
    ParcelSystem sys;
    std::string id;
    sys.resources().add(ResourceType::CollectionCounter, "A", id);
    sys.resources().add(ResourceType::CollectionCounter, "B", id);
    sys.resources().add(ResourceType::DeliveryPersonnel, "C", id);
    sys.resources().add(ResourceType::SpecialHandlingCounter, "D", id);
    sys.resources().add(ResourceType::DeliveryDesk, "E", id);

    std::clock_t start = std::clock();

    int registered = 0;
    for (int i = 0; i < PARCELS; i++) {
        Parcel parcel;
        parcel.recipientId = "TP" + std::to_string(100000 + i);
        parcel.recipientName = "Load Test";
        parcel.deliveryCompany = "Pos Laju";
        if (sys.parcels().registerNew(parcel, "S1", id).success) registered++;
    }
    CHECK(registered == PARCELS);

    // Route each parcel in turn, skipping Hold, so all four services are used.
    int routed = 0;
    for (int i = 0; i < PARCELS; i++) {
        std::string parcelId;
        std::string requestId;
        int skipped = 0;
        if (!sys.parcels().processNext("S1", parcelId, skipped).success) break;
        ParcelRoute route = static_cast<ParcelRoute>(i % 4);
        if (sys.routeParcel(parcelId, route, i % 10 == 0, requestId).success) routed++;
    }
    CHECK(routed == PARCELS);

    // Handle every request. One resource at a time is enough, because each job finishes at once.
    int completed = 0;
    int lastScore = 1000;
    bool scoresDescend = true;
    while (true) {
        std::string requestId;
        std::string resourceId;
        if (!sys.handleNextRequest(requestId, resourceId).success) break;
        ServiceRequest* request = sys.services().find(requestId);
        if (request->score > lastScore) scoresDescend = false;
        lastScore = request->score;
        if (sys.completeRequest(requestId, request->recipientId).success) completed++;
    }
    CHECK(completed == PARCELS);
    CHECK(scoresDescend);

    int finished = 0;
    sys.organisation().forEachInOrder([&](const Parcel& parcel) {
        if (isFinalStatus(parcel.status) || parcel.status == ParcelStatus::ReadyForCollection) finished++;
    });
    CHECK(finished == PARCELS);

    int totalJobs = 0;
    sys.resources().forEach([&](const Resource& resource) { totalJobs += resource.transactions; });
    CHECK(totalJobs == PARCELS);

    double seconds = static_cast<double>(std::clock() - start) / CLOCKS_PER_SEC;
    std::cout << "Full flow for " << PARCELS << " parcels: " << seconds << " seconds\n";
    CHECK(seconds < 5.0);
}

// Parcel IDs that arrive in order degrade a plain BST into a line. These numbers
// back up the efficiency discussion in the Q&A.
void reportBstHeight() {
    const int COUNT = 500;
    BST<int, int> inOrderKeys;
    BST<int, int> shuffledKeys;
    int keys[COUNT];
    for (int i = 0; i < COUNT; i++) {
        keys[i] = i;
        inOrderKeys.insert(i, i);
    }
    std::srand(42);
    for (int i = COUNT - 1; i > 0; i--) {
        int j = std::rand() % (i + 1);
        int held = keys[i];
        keys[i] = keys[j];
        keys[j] = held;
    }
    for (int i = 0; i < COUNT; i++) shuffledKeys.insert(keys[i], keys[i]);

    std::cout << "BST height with " << COUNT << " keys: in order = " << inOrderKeys.height()
              << ", shuffled = " << shuffledKeys.height() << "\n";
    CHECK(inOrderKeys.height() == COUNT);
    CHECK(shuffledKeys.height() < COUNT / 10);
}

}  // namespace

int main() {
    testFullFlow();
    reportBstHeight();
    return TestUtil::finish("Load");
}
