#pragma once
#include <string>
#include "core/Models.hpp"
#include "core/Result.hpp"
#include "core/Validation.hpp"
#include "ds/BST.hpp"

// Task 5. Owns the master parcel records in a BST keyed by Parcel ID.
// Parcel ID is unique and is what every other module uses to look a parcel up.
//
// This module is header-only. Functions defined inside a class are automatically
// inline, so including this header in several .cpp files is safe.
class OrganisationModule {
public:
    Result insert(const Parcel& parcel) {
        if (!Validation::isParcelId(parcel.id)) return Result::fail("Invalid Parcel ID. Use P0001 to P9999.");
        std::string error = Validation::parcelFieldError(parcel);
        if (!error.empty()) return Result::fail(error);
        if (!tree.insert(parcel.id, parcel)) return Result::fail("Parcel " + parcel.id + " already exists.");
        return Result::ok("Parcel " + parcel.id + " inserted.");
    }

    Parcel* find(const std::string& parcelId) { return tree.find(parcelId); }

    Result update(const std::string& parcelId, const Parcel& changed) {
        if (tree.isEmpty()) return Result::fail("There are no parcel records to update.");
        Parcel* parcel = tree.find(parcelId);
        if (parcel == nullptr) return Result::fail("Parcel " + parcelId + " was not found.");
        // Changing the key in place would break the BST's ordering.
        if (changed.id != parcelId) return Result::fail("A Parcel ID can't be changed.");
        std::string error = Validation::parcelFieldError(changed);
        if (!error.empty()) return Result::fail(error);
        *parcel = changed;
        return Result::ok("Parcel " + parcelId + " updated.");
    }

    Result remove(const std::string& parcelId) {
        if (tree.isEmpty()) return Result::fail("There are no parcel records to remove.");
        if (!tree.remove(parcelId)) return Result::fail("Parcel " + parcelId + " was not found.");
        return Result::ok("Parcel " + parcelId + " removed.");
    }

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
