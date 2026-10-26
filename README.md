# Smart Campus Parcel Collection and Delivery Management System

## Overview

This is a C++ prototype of a **Smart Campus Parcel Collection and Delivery Management System** developed for Asia Pacific University.  

The system manages the full lifecycle of parcels arriving on campus: registration, sorting, service requests, resource allocation, activity tracking, and category-based organisation. It is designed to handle high volumes of parcels efficiently while maintaining accurate operational records.

## Features

The system is built as **one integrated C++ application** consisting of five modules:

| Module | Description |
|--------|-------------|
| **Task 1** | Parcel Intake, Sorting and Processing |
| **Task 2** | Parcel Service and Collection Management |
| **Task 3** | Collection Counter and Delivery Resource Management |
| **Task 4** | Parcel Activity and Record Management |
| **Task 5** | Parcel Category Organisation |

### Key Capabilities
- Register and track parcels with unique IDs
- Create and prioritise service requests (collection, delivery, special handling, return, re-delivery)
- Allocate collection counters and delivery resources
- Maintain full activity history with limited undo support
- Organise and search parcels using a tree-based structure
- Handle common exception cases (duplicates, missing records, invalid operations, empty structures, etc.)

## Data Structures

All compulsory data structures are implemented from scratch (no STL `stack`, `queue`, `priority_queue`, `vector`, `list`, or `deque` as substitutes):

- **Stack**
- **Queue**
- **Circular Queue**
- **Priority Queue**
- **Binary Tree / Binary Search Tree (BST)**

Each structure is meaningfully integrated into the system to solve real operational requirements.

## Tech Stack

- **Language:** C++
- **Build:** Standard C++ compiler (g++ / clang++ recommended)
- **No external libraries** required beyond the standard library (containers listed above are forbidden as substitutes)

## Project Structure
```bash
├── src/                  # Source files (.cpp)
├── include/              # Header files (.hpp / .h)
├── data/                 # Sample data files (if any)
├── docs/                 # Additional documentation
└── README.md
```

## Building & Running

```bash
# Compile (example)
g++ -std=c++17 -Iinclude src/*.cpp -o SmartParcelSystem

# Run
./SmartParcelSystem
