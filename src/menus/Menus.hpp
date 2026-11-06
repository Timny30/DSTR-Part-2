#pragma once
#include <string>
#include "ParcelSystem.hpp"

// All console output lives in the menus. Row output starts with the record's ID,
// and messages start with [OK], [ERROR] or [INFO], which the scenario tests rely on.
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

// Field prompts shared by Task 1 and Task 5. Return false if the user cancels.
bool promptParcelType(ParcelType& out);
bool promptServiceType(ServiceType& out);
bool promptNewParcelDetails(Parcel& parcel);
Result promptAndUpdateParcel(ParcelSystem& system);
