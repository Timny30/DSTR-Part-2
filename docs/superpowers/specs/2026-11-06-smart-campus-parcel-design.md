# Smart Campus Parcel System: Design Spec

**Module:** CT077-3-2-DSTR, Assignment 2 (30 marks)
**Date:** 2026-11-06
**Status:** Implemented

## 1. Goal and scope

Build one integrated C++ prototype that takes a parcel from arrival to completion across five task modules. Each compulsory data structure must solve a real requirement.

**In scope:** the five tasks in the brief, demo data loaded from CSV, and automated tests.

**Out of scope:**
- Saving changes to disk. Changes last only until the program exits.
- Passwords and user accounts. "Authorised staff" is represented by a Staff ID entered at startup, which is recorded on each activity.
- A graphical interface. The program uses console menus.

**Constraints:**
- C++17, compiled with `g++`, with no external libraries.
- Banned as substitutes: STL `vector`, `list`, `stack`, `queue`, `priority_queue`, `deque`. The team also avoids `map`, `set` and their unordered versions.
- Allowed: `std::string`, file streams and `<ctime>`.

## 2. Architecture

### 2.1 Layers

Each module may call only the modules listed in its row, which prevents circular dependencies.

| Layer | Component | May call |
|---|---|---|
| Interface | `main.cpp` menus | `ParcelSystem` and all module read functions |
| Coordinator | `ParcelSystem` | All five modules |
| Workflow | `ResourceModule` (T3) | `core`, `ds` only |
| Workflow | `ServiceModule` (T2) | `ParcelModule` |
| Workflow | `ParcelModule` (T1) | `OrganisationModule`, `ActivityModule` |
| Foundation | `OrganisationModule` (T5) | `core`, `ds` only |
| Foundation | `ActivityModule` (T4) | `core`, `ds` only |

`ParcelSystem` owns the module instances and handles work that crosses modules:
- Routing a parcel, undo, and CSV loading.
- Completing or cancelling a request, or cancelling a parcel, because these also free a resource.
- Deleting a parcel, because the delete must first check for an active request.
- Marking a resource unavailable mid-job, because that also returns its request to waiting and restores the parcel.

### 2.2 Folder layout

```text
include/ds/       Stack, Queue, CircularQueue, PriorityQueue, BST, LinkedList
include/core/     Models, status enums, validation, date/time, Result, input helpers
include/modules/  One header per module
include/menus/    Console menu declarations
include/          ParcelSystem.hpp
src/core/         Core implementations
src/modules/      One .cpp per module
src/menus/        One .cpp per task menu, plus shared prompts and row printing
src/              ParcelSystem.cpp, main.cpp
data/             parcels.csv, resources.csv
tests/ds/         One test program per data structure
tests/integration/ System rules and exception tests, and the load test
tests/scenarios/  Scripted input files with expected output
archive/          main_draft.cpp, kept for reference and never built
```

### 2.3 Data ownership

Each kind of record has exactly one master copy. Every other structure stores only its ID, so copies can't go out of date.

| Structure | Owner | Contents | Key cost |
|---|---|---|---|
| BST | T5 | Master `Parcel` records, keyed by Parcel ID | O(log n) average, O(n) worst case |
| Queue (linked) | T1 | Parcel IDs waiting to be processed | O(1) enqueue and dequeue |
| Priority queue (binary heap on a growable array) | T2 | Request ID, score, arrival sequence | O(log n) push and pop |
| Circular queue (fixed-size array, capacity 16) | T3 | Resource IDs, one queue per resource type | O(1) rotate, O(k) to find a free resource |
| Stack (linked) | T4 | Activity IDs that can still be undone | O(1) push and pop |
| Linked list | Shared | Master requests, master resources, permanent activity log | O(1) append, O(n) scan |

### 2.4 Identifiers and formats

| Item | Format | Notes |
|---|---|---|
| Parcel ID | `P0001`–`P9999` | Generated. Fixed width, so text order matches number order. |
| Request ID | `REQ0001` | Generated |
| Resource ID | `RES001` | Generated |
| Activity ID | `ACT0001` | Generated |
| Recipient ID | 3–12 letters or digits | Converted to uppercase, for example `TP012345` |
| Recipient name | Not empty; letters, spaces, `-`, `'`, `.` | Multi-word names are valid |
| Date/time | `YYYY-MM-DD HH:MM` | Taken from the system clock |

CSV rows and Task 5 inserts supply their own Parcel ID. The ID generator always continues above the highest ID in use.

### 2.5 Results and input

- Module functions return a `Result` holding a success flag and a message. Modules don't print results, and exceptions aren't used for expected failures.
- All console input goes through one shared helper that reads whole lines and validates numbers and menu choices.
- Every data structure has a destructor. Copying is disabled unless a structure needs it.

## 3. Status rules

### 3.1 Parcel statuses

`Delivered` and `On Hold` are additions to the brief's list. The brief says statuses "include" its list and describes both situations without naming a status.

| From | Allowed next status |
|---|---|
| Received | Pending |
| Pending | Processing |
| Processing | Ready for Collection, Out for Delivery, Returned, On Hold |
| Ready for Collection | Collected, Out for Delivery, Processing, On Hold |
| Out for Delivery | Delivered, Ready for Collection, Processing, On Hold |
| On Hold | Processing, Ready for Collection |
| Missing | On Hold |
| Any status except a final one | Cancelled, Missing |

**Final statuses:** Collected, Delivered, Returned and Cancelled. No further change is allowed, except by a permitted undo of a cancellation.

`ParcelModule::changeStatus` is the only function for normal status changes. It checks this table and logs the change. Undo instead restores the saved record through a separate `ParcelModule::restoreSnapshot`, which only `ParcelSystem` calls, and undo has its own checks (section 7).

### 3.2 Request statuses

`Pending → Processing → Completed` or `Returned`. `Pending` or `Processing` may also become `Cancelled`. Completed, Returned and Cancelled requests are closed and can't be processed again.

### 3.3 Resource statuses

Available, Occupied, Unavailable, Maintenance. Only Available resources can be assigned. Occupied can return to Available only by completing, cancelling, or undoing the assignment, or by the "resource lost" path in section 6.

## 4. End-to-end flow

1. Staff register a parcel: status `Received`, then `Pending` as it joins the queue.
2. Staff process the next parcel: status `Processing`, then choose a route (section 5).
3. `ParcelSystem` updates the parcel and creates the matching request.
4. Staff handle the next request: the highest-scoring request is previewed, and a matching resource is found.
5. The request is removed from the heap only after a resource is assigned. Resource becomes `Occupied`, request becomes `Processing`.
6. Staff complete or cancel the request, or mark a delivery failed. Parcel, request and resource all update.
7. Every step adds an entry to the activity log.

## 5. Task 1: Parcel intake and processing

**Features:** register, list pending, process next, view and search (through T5), update fields, report missing, cancel.

**Routing:**

| Route chosen | Parcel status after routing | Request created | Parcel status after completion |
|---|---|---|---|
| Collection | Ready for Collection | Recipient Collection | Collected |
| Delivery | Processing, then Out for Delivery on assignment | Campus Delivery | Delivered |
| Special handling | Processing | Special Handling | Ready for Collection |
| Return | Processing | Return to Sender | Returned |
| Hold | On Hold | None | Not applicable |

After special handling, staff create a follow-up request if the parcel needs delivery.

**Stale queue entries:** if a queued parcel was cancelled, removed, or marked missing, it's skipped when it reaches the front, and the skip is reported to staff.

**Exceptions:** missing or invalid fields, duplicate Parcel ID, parcel not found, already-final parcel, empty queue, missing parcel.

## 6. Task 2 and Task 3: Requests and resources

### 6.1 Service requests (T2)

**Score = service weight + urgency bonus.** The earliest arrival sequence wins a tie.

| Service type | Weight | Reason |
|---|---|---|
| Special Handling | 40 | Highest risk of damage |
| Re-delivery | 30 | The recipient has already had a failed delivery |
| Recipient Collection | 20 | The recipient is waiting |
| Campus Delivery | 20 | Standard service |
| Return to Sender | 10 | No recipient is waiting |
| Urgent flag | +50 | Set by staff |

**Features:** create, list active, preview next, process, complete, cancel, return, list closed.

**Rules:**
- A parcel may have only one active request.
- Staff may create a request manually only when the parcel is Processing, Ready for Collection or On Hold.
- A Recipient Collection request is completed only if the recipient ID entered matches the parcel.
- A cancelled request stays in the heap and is skipped when it reaches the top (lazy deletion).
- Cancelling a request moves its parcel to On Hold so staff can decide what happens next. An assigned resource is freed.
- A failed delivery cancels the request and returns the parcel to Ready for Collection. Staff may then create a Re-delivery request.

**Known limitation:** low-scoring requests can wait a long time at peak. Raising a score as a request waits (ageing) is an optional innovation.

**Exceptions:** no active requests, invalid Request or Parcel ID, duplicate active request, wrong recipient, closed request, invalid operation for the parcel's status.

### 6.2 Resources (T3)

| Service type | Resource type |
|---|---|
| Recipient Collection | Collection Counter |
| Campus Delivery, Re-delivery | Delivery Personnel |
| Special Handling | Special-Handling Counter |
| Return to Sender | Delivery Desk |

**Features:** register, list by availability, assign, change status, release, show utilisation.

**Allocation:** rotate through the matching circular queue at most once and take the first Available resource. This shares work evenly among resources of that type.

**No free resource:** the request stays at the front of the heap with its position unchanged, and staff see the reason.

**Resource lost mid-job:** if an Occupied resource is marked Unavailable or Maintenance, its request returns to `Pending` with its original arrival sequence and is pushed back into the heap. The parcel returns to the status it had before assignment.

**Transaction count:** increases by 1 when a job completes, not when it's assigned.

**Exceptions:** invalid Resource ID, full circular queue, all resources busy, no suitable type, unavailable resource selected, duplicate assignment, invalid release or update.

## 7. Task 4: Activity and undo

**Activity record:** Activity ID, Parcel ID, Request ID and Resource ID where applicable, activity type, date/time, previous and new parcel status, previous request and resource state where applicable, Staff ID, reversed flag.

**Features:** recent activities, full trace for one parcel, undo.

**Undo rule:** staff enter an Activity ID, and only the activity on top of the undo stack is accepted. `ParcelSystem` restores the saved state in every affected module.

| Activity | Reversible? |
|---|---|
| Parcel Update, Service Request Creation, Resource Assignment, Cancellation, non-final Status Change | Yes |
| Parcel Registration | Only if it's the parcel's only activity |
| Collection Confirmation, Service Completion, Return Processing, Parcel Removal | No, these are permanent |

**Effects of undo:**
- Undoing a registration removes the parcel from the BST. Its queue entry is skipped later as stale.
- Undoing a Resource Assignment frees the resource and pushes the request back into the heap with its original sequence.
- The original activity is marked `reversed`, and the undo adds its own log entry. Nothing is erased from the log.
- Undo entries are never pushed back on the stack, so an undo can't itself be undone.
- If restoring an activity needs a resource that's no longer Available, the undo is refused and nothing changes. The entry stays on the stack so staff can retry once the resource is free.

**After a permanent action:** that parcel's older undo entries can no longer be applied. When one reaches the top of the stack, the system discards it and tells staff why.

**Exceptions:** no history, invalid Parcel or Activity ID, permanent activity, already reversed, activity not on top of the stack, result would be an invalid status.

## 8. Task 5: Parcel organisation

**Key:** Parcel ID, because it's unique and every module looks parcels up by it.

**Fields:** all Task 1 parcel fields, plus Service Type from the parcel's current or latest request. Parcel Category uses Parcel Type (Standard, Large, Fragile, Document, Special-Handling).

| Operation | Method | Why |
|---|---|---|
| List parcels | In-order traversal | Gives ID order without a separate sort |
| Show tree structure | Pre-order traversal, indented | Shows each parent before its children |
| Search by ID | BST search | O(log n) on average |
| Search by category or recipient | Full traversal with a filter | These aren't the key, so O(n) |

**Rules:**
- Insert uses the same validation as Task 1 registration and rejects duplicate IDs.
- Update may change any field except Parcel ID, because changing the key would break BST ordering.
- Delete handles no child, one child and two children (using the in-order successor).
- A parcel with an active request can't be deleted.

**Balance:** IDs generated in order make a plain BST degrade toward O(n). The baseline is a plain BST. AVL balancing is an optional innovation for the T5 owner and must not change the module's interface.

**Exceptions:** empty tree, duplicate key, not found on search, update or delete, invalid ID, invalid fields, delete blocked by an active request.

## 9. Demo data

`ParcelSystem` loads `data/parcels.csv` and `data/resources.csv` at startup, using the normal registration functions so that validation and logging still apply.

- A missing file shows a warning, and the program starts without that data.
- An invalid row is reported with its line number and skipped, and loading continues.

## 10. Team and workflow

| Owner | Builds | Also responsible for |
|---|---|---|
| T1 | `ParcelModule`, Queue | Validation rules |
| T2 | `ServiceModule`, PriorityQueue | Justifying the scores |
| T3 | `ResourceModule`, CircularQueue | `resources.csv` |
| T4 | `ActivityModule`, Stack | Undo logic in `ParcelSystem` |
| T5 | `OrganisationModule`, BST | `parcels.csv` |
| Integration lead | `core`, LinkedList, `ParcelSystem`, menus | Merging, final testing |

The integration lead must also own one task, because the brief requires each member to implement at least one. In a team of 4, one member takes a second task.

**Git:** one branch per person, merged by pull request, with each member committing under their own account. `main` must always compile. Header changes after phase 0 need team agreement.

## 11. Build order

| Phase | Work | Finished when |
|---|---|---|
| 0. Setup | Folders, core models, module headers, build scripts | An empty program compiles for everyone |
| 1. Data structures | Each owner builds and tests their own | All structure tests pass |
| 2. Foundation | T5, T4 | Parcels can be stored and activities logged |
| 3. Workflow | T1, T2, T3 | Each module's scenarios pass |
| 4. Integration | `ParcelSystem`, routing, undo, CSV, menus | A parcel completes the flow in section 4 |
| 5. Hardening | Exceptions, memory checks, load test | All scenarios pass |
| 6. Submission | Remove draft leftovers, zip, forms, matrix, recordings | Submission checklist complete |

Setup is the only phase in the current preparation work. Task owners implement phases 1–5.

## 12. Testing

1. **Data structure tests:** one program per structure, covering empty, full and boundary cases, including all three BST delete cases and circular queue wrap-around.
2. **System tests and scripted scenarios:** `tests/integration/test_system.cpp` covers the rules and exceptions in the brief through `ParcelSystem`. The six regression scenarios below pipe input through the real menus and compare the output against expected files.
3. **Load test:** 500 parcels through the full flow, plus a comparison of BST depth for in-order and shuffled IDs.
4. **Memory:** structure tests confirm that every allocated node is freed.

**Regression scenarios from the draft (all must pass):**

| # | Scenario | Required result |
|---|---|---|
| 1 | Register a duplicate Parcel ID | Rejected; original record and queue unchanged |
| 2 | Create a request, then process the same queued parcel | Parcel status is never overwritten from an old copy |
| 3 | Undo a registration | Parcel removed; its queue entry is skipped |
| 4 | Create a second active request for a parcel | Rejected |
| 5 | Undo a request-related activity | Parcel, request and resource all return to a consistent state |
| 6 | Enter a multi-word recipient name | Saved in full; later prompts unaffected |

## 13. Submission

- Zip with source code and data files, named as the brief requires.
- One signed AI Usage Policy form per student, outside the zip.
- Workload matrix in Word.
- Individual recording of about 5 minutes per student.
- Attendance at the face-to-face Q&A.

## 14. Questions for the lecturer

These don't block the design. The working assumption is in brackets.

1. The brief says "four compulsory" tasks but lists five. (Build all five.)
2. How should recordings be submitted? They're missing from the checklist. (Ask before submission.)
3. Should the zip name end in `_LW2`, as in the Moodle sample? (Ask before submission.)
4. Can one person hold an integration role? (Yes, provided they also implement a task.)
5. What is the exact due date? The brief gives only Week 10–Week 14. (Ask now.)
