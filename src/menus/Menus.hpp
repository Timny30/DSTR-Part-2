#pragma once
#include <string>
#include "ParcelSystem.hpp"

void runTask1Menu(ParcelSystem& system);
void runTask2Menu(ParcelSystem& system);
void runTask3Menu(ParcelSystem& system);
void runTask4Menu(ParcelSystem& system);
void runTask5Menu(ParcelSystem& system);

void printResult(const Result& result);
void printInfo(const std::string& message);
void printHeading(const std::string& title);
void printParcelRow(const Parcel& parcel);
void printRequestRow(const ServiceRequest& request);
void printResourceRow(const Resource& resource);
void printActivityRow(const Activity& activity);

bool promptParcelType(ParcelType& out);
bool promptServiceType(ServiceType& out);
bool promptNewParcelDetails(Parcel& parcel);
Result promptAndUpdateParcel(ParcelSystem& system);
