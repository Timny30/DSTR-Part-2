# Smart Campus Parcel Collection and Delivery Management System

A C++17 console prototype for CT077-3-2-DSTR Assignment 2. It takes a parcel from arrival on campus to collection, delivery or return, as one integrated system of five modules. The data structures are implemented from scratch, without STL containers.

## Build and run

You need `g++` with C++17 support. Run these commands from the project folder.

On Windows:

```bash
.\build.bat
```

In Git Bash, Linux or macOS:

```bash
bash build.sh
```

Then start the program:

```bash
./assignment.exe
```

The program loads sample parcels and resources from `data/` and asks for a Staff ID, which it records on each activity. In VS Code, the default build task produces the same program.

| Option | Effect |
|---|---|
| `--no-demo` | Starts without loading the CSV files |
| `--fixed-time "YYYY-MM-DD HH:MM"` | Uses a fixed clock, so output is repeatable |

## Modules and data structures

| Task | Module | Main structure | Why it fits |
|---|---|---|---|
| 1. Intake and processing | `ParcelModule` | Queue | Parcels are handled in arrival order |
| 2. Service and collection | `ServiceModule` | Priority queue (binary heap) | The most urgent request goes first, and ties stay in arrival order |
| 3. Counters and delivery resources | `ResourceModule` | Circular queue | Rotating through each resource type shares work evenly |
| 4. Activity and records | `ActivityModule` | Stack, plus a linked list log | Undo reverses the most recent action, and the log keeps full history |
| 5. Parcel organisation | `OrganisationModule` | Binary search tree | Fast lookup by Parcel ID, and in-order traversal gives a sorted list |

A parcel moves through the tasks in this order:

1. **Task 1:** register the parcel and choose its route. It stays Pending.
2. **Task 2:** create the service request, which moves the parcel to Processing.
3. **Task 3:** allocate a counter, desk or delivery person.
4. **Task 2:** complete the request or record a failure. Reporting a parcel missing also happens in Task 2.

`ParcelSystem` owns the modules and handles actions that cross them, such as creating and completing a request, and undo. Each record has one master copy, and other structures store only its ID, so copies can't go out of date.

## Project structure

```text
src/               ParcelSystem and main
src/ds/            Stack, Queue, CircularQueue, PriorityQueue, BST, LinkedList
src/core/          Models, status enums, validation, date/time, input helpers
src/modules/       One header-only file per task module, holding all of its code
src/menus/         One console menu per task, plus shared prompts and row printing
data/              Sample parcels.csv and resources.csv
tests/             Data structure, integration, load and scripted scenario tests
docs/              Design spec and implementation plan
archive/           The earlier draft, kept for reference and never built
```

## Tests

Run the whole suite from Git Bash:

```bash
bash scripts/test.sh
```

It checks for banned containers, then runs the data structure tests, the system rules and exception tests, a 500-parcel load test, and scripted menu scenarios.

## Design

See the [design spec](docs/superpowers/specs/2026-11-06-smart-campus-parcel-design.md) for the status rules, priority scores, allocation rules and undo rules.
