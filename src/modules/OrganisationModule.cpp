#include "modules/OrganisationModule.hpp"
#include "core/Validation.hpp"

Result OrganisationModule::insert(const Parcel& parcel) {
    if (!Validation::isParcelId(parcel.id)) return Result::fail("Invalid Parcel ID. Use P0001 to P9999.");
    std::string error = Validation::parcelFieldError(parcel);
    if (!error.empty()) return Result::fail(error);
    if (!tree.insert(parcel.id, parcel)) return Result::fail("Parcel " + parcel.id + " already exists.");
    return Result::ok("Parcel " + parcel.id + " inserted.");
}

Parcel* OrganisationModule::find(const std::string& parcelId) { return tree.find(parcelId); }

Result OrganisationModule::update(const std::string& parcelId, const Parcel& changed) {
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

Result OrganisationModule::remove(const std::string& parcelId) {
    if (tree.isEmpty()) return Result::fail("There are no parcel records to remove.");
    if (!tree.remove(parcelId)) return Result::fail("Parcel " + parcelId + " was not found.");
    return Result::ok("Parcel " + parcelId + " removed.");
}
