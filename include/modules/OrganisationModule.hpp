#pragma once
#include <string>
#include "core/Models.hpp"
#include "core/Result.hpp"
#include "ds/BST.hpp"

// Task 5. Owns the master parcel records in a BST keyed by Parcel ID.
// Parcel ID is unique and is what every other module uses to look a parcel up.
class OrganisationModule {
public:
    Result insert(const Parcel& parcel);
    Parcel* find(const std::string& parcelId);
    Result update(const std::string& parcelId, const Parcel& changed);
    Result remove(const std::string& parcelId);

    // In-order gives parcels sorted by ID without a separate sort.
    template <typename Fn>
    void forEachInOrder(Fn fn) {
        tree.inOrder([&fn](const std::string&, Parcel& parcel) { fn(parcel); });
    }

    // Pre-order shows each parent before its children, which reveals the tree's shape.
    template <typename Fn>
    void forEachPreOrder(Fn fn) {
        tree.preOrder([&fn](const std::string&, Parcel& parcel, int depth) { fn(parcel, depth); });
    }

    // Category and recipient aren't the key, so these need a full traversal: O(n).
    template <typename Fn>
    void forEachByType(ParcelType type, Fn fn) {
        forEachInOrder([&](Parcel& parcel) { if (parcel.type == type) fn(parcel); });
    }

    template <typename Fn>
    void forEachByRecipient(const std::string& recipientId, Fn fn) {
        forEachInOrder([&](Parcel& parcel) { if (parcel.recipientId == recipientId) fn(parcel); });
    }

    bool isEmpty() const { return tree.isEmpty(); }
    int size() const { return tree.size(); }
    int height() const { return tree.height(); }

private:
    BST<std::string, Parcel> tree;
};
