# Smart Campus Parcel System: Implementation Plan

**Spec:** [2026-11-06-smart-campus-parcel-design.md](../specs/2026-11-06-smart-campus-parcel-design.md)
**Goal:** one integrated C++17 prototype covering Tasks 1–5.
**Who does what:** Claude prepares Phase 0 on `setup/project-structure`. Task owners implement Phases 1–5 so that each member can explain their own work.

Tick boxes as work finishes. A task is finished only when its "Done when" check passes.

---

## Phase 0: Project setup

Phase 0 contains no feature logic. Function bodies are stubs that compile and return "not implemented".

### 0.1 Folders and draft

- [ ] Move `main.cpp` to `archive/main_draft.cpp` using `git mv`.
- [ ] Create the folders in spec section 2.2.
- [ ] Remove the `[cite: 2]` tags from the archived draft.

### 0.2 Build and ignore settings

- [ ] `.gitignore`: replace `.exe*`, which doesn't match `assignment.exe`, with `*.exe` and `build/`.
- [ ] Add `build.bat` and `build.sh`, both running:
  ```bash
  g++ -std=c++17 -g -Wall -Wextra -Iinclude src/*.cpp src/core/*.cpp src/modules/*.cpp -o assignment.exe
  ```
- [ ] `.vscode/tasks.json`: use the same flags and sources. `launch.json` needs no change because the program is still `assignment.exe`.

### 0.3 Core types (`include/core/`)

These are complete in Phase 0, because every module depends on them.

- [ ] `Enums.hpp`: `ParcelType`, `ParcelStatus`, `ServiceType`, `RequestStatus`, `ResourceType`, `ResourceStatus`, `ActivityType`, each with a `toString`.
- [ ] `Models.hpp`: `Parcel`, `ServiceRequest`, `Resource` and `Activity`, with the fields in spec sections 5–8.
- [ ] `Result.hpp`: `Result::ok(message)` and `Result::fail(message)`.
- [ ] `Validation.hpp`, `DateTime.hpp`, `Input.hpp`: declarations only, plus stubs. The integration lead implements them in Phase 1.

### 0.4 Data structure interfaces (`include/ds/`)

All are templates with stub bodies. Owners replace the stubs and may add private members, but public signatures are fixed.

```cpp
template <typename T> class LinkedList {
    void append(const T& value);
    template <typename Pred> T* find(Pred pred);
    template <typename Fn> void forEach(Fn fn);
    bool isEmpty() const;  int size() const;
};
template <typename T> class Queue {
    void enqueue(const T& value);
    bool dequeue(T& out);  bool peek(T& out) const;
    template <typename Fn> void forEach(Fn fn) const;
    bool isEmpty() const;  int size() const;
};
template <typename T> class Stack {
    void push(const T& value);
    bool pop(T& out);  bool peek(T& out) const;
    bool isEmpty() const;  int size() const;
};
template <typename T, int Capacity = 16> class CircularQueue {
    bool enqueue(const T& value);      // false when full
    bool rotate(T& out);               // returns the front and moves it to the rear
    bool isEmpty() const;  bool isFull() const;  int size() const;
};
template <typename T, typename HigherPriority> class PriorityQueue {
    void push(const T& value);
    bool pop(T& out);  bool peek(T& out) const;
    bool isEmpty() const;  int size() const;
};
template <typename K, typename V> class BST {
    bool insert(const K& key, const V& value);  // false on a duplicate
    V* find(const K& key);
    bool remove(const K& key);
    template <typename Fn> void inOrder(Fn fn);   // fn(const K&, V&)
    template <typename Fn> void preOrder(Fn fn);  // fn(const K&, V&, int depth)
    bool isEmpty() const;  int size() const;  int height() const;
};
```

### 0.5 Module interfaces (`include/modules/`, `include/ParcelSystem.hpp`)

Modules don't print. Lists are provided through visitor functions, and the menus do the printing.

```cpp
class OrganisationModule {   // T5
    Result insert(const Parcel& parcel);
    Parcel* find(const std::string& parcelId);
    Result update(const std::string& parcelId, const Parcel& changed);  // Parcel ID can't change
    Result remove(const std::string& parcelId);
    template <typename Fn> void forEachInOrder(Fn fn);
    template <typename Fn> void forEachPreOrder(Fn fn);
    bool isEmpty() const;  int height() const;
};
class ActivityModule {       // T4
    std::string record(Activity activity, bool reversible);  // returns the Activity ID
    Activity* find(const std::string& activityId);
    template <typename Fn> void forEachRecent(int limit, Fn fn);
    template <typename Fn> void forEachForParcel(const std::string& parcelId, Fn fn);
    Result peekUndoable(Activity*& out);
    Result popUndoable();
};
class ParcelModule {         // T1
    ParcelModule(OrganisationModule&, ActivityModule&);
    Result registerNew(const Parcel& input, const std::string& staffId, std::string& outId);
    Result registerWithId(const Parcel& input, const std::string& staffId);
    Result processNext(const std::string& staffId, std::string& outId);
    Result changeStatus(const std::string& parcelId, ParcelStatus next, const std::string& staffId);
    Result restoreStatus(const std::string& parcelId, ParcelStatus saved);  // ParcelSystem only
    Result updateFields(const std::string& parcelId, const Parcel& changed, const std::string& staffId);
    template <typename Fn> void forEachPending(Fn fn) const;
};
class ServiceModule {        // T2
    ServiceModule(ParcelModule&, ActivityModule&);
    Result create(const std::string& parcelId, ServiceType type, bool urgent,
                  const std::string& staffId, std::string& outId);
    ServiceRequest* find(const std::string& requestId);
    ServiceRequest* activeForParcel(const std::string& parcelId);
    Result peekNext(ServiceRequest*& out);    // skips closed requests
    Result popNext();
    Result pushBack(const std::string& requestId);  // keeps the original sequence
    Result setStatus(const std::string& requestId, RequestStatus next);
    template <typename Fn> void forEachActive(Fn fn);
    template <typename Fn> void forEachClosed(Fn fn);
};
class ResourceModule {       // T3
    ResourceModule(ServiceModule&, ActivityModule&);
    Result add(ResourceType type, const std::string& location, std::string& outId);
    Resource* find(const std::string& resourceId);
    Result assign(ServiceType service, const std::string& requestId, std::string& outResourceId);
    Result release(const std::string& resourceId, bool jobCompleted);
    Result setStatus(const std::string& resourceId, ResourceStatus next);
    template <typename Fn> void forEach(Fn fn);
    static ResourceType typeFor(ServiceType service);
};
class ParcelSystem {         // Integration lead; undo logic by T4
    Result loadDemoData(const std::string& dataDir);
    Result routeParcel(const std::string& parcelId, const std::string& route, bool urgent);
    Result handleNextRequest(std::string& outRequestId, std::string& outResourceId);
    Result completeRequest(const std::string& requestId, const std::string& recipientId);
    Result cancelRequest(const std::string& requestId);
    Result failDelivery(const std::string& requestId);
    Result cancelParcel(const std::string& parcelId);
    Result deleteParcel(const std::string& parcelId);
    Result undo(const std::string& activityId);
    // plus accessors for the five modules and setStaffId
};
```

### 0.6 Program entry and sample data

- [ ] `src/main.cpp`: asks for a Staff ID and shows the five-task menu, with stub entries printing "not implemented".
- [ ] Support two test flags:
  - `--no-demo`: start without CSV data.
  - `--fixed-time "YYYY-MM-DD HH:MM"`: use a fixed clock, so scripted output is repeatable.
- [ ] `data/parcels.csv` and `data/resources.csv`: small samples with a header row, using the exact columns the loader will read.

### 0.7 Test tools

- [ ] `tests/test_util.hpp`: a small `CHECK(condition)` macro with a pass/fail summary.
- [ ] `tests/ds/test_smoke.cpp`: instantiates every structure, to prove that the stubs compile.
- [ ] `tests/scenarios/`: folder with one working example, `smoke.in` and `smoke.expected`.
- [ ] `scripts/test.sh` does three things:
  - Builds and runs every program in `tests/ds/`.
  - Pipes each scenario through the program and compares the output, ignoring line-ending differences.
  - Fails if `src/` or `include/` uses a banned container.

### 0.8 README and commit

- [ ] Update the README's build, run, test and folder sections, and close its unterminated code block.
- [ ] Commit to `setup/project-structure` without adding the assignment `.docx`.

**Done when:**
- `build.bat` and `build.sh` compile with no warnings.
- `assignment.exe` runs and exits from the menu.
- `scripts/test.sh` passes.
- The team has reviewed the interfaces before the branch merges to `main`.

---

## Phase 1: Data structures

Each owner works on their own branch and writes tests in `tests/ds/`.

### Integration lead: LinkedList and core helpers

- [ ] LinkedList, including `find` and `forEach`.
- [ ] `Validation`: rules from spec section 2.4.
- [ ] `DateTime`: system clock, plus the fixed-time override.
- [ ] `Input`: whole-line reading, number and menu choice validation.

**Done when:** tests cover an empty list, one item, many items, a failed `find`, multi-word input and non-numeric input.

### T1: Queue

- [ ] Linked implementation with front and rear pointers.

**Done when:** tests cover first-in-first-out order, empty dequeue, and reuse after emptying.

### T2: PriorityQueue

- [ ] Binary heap on a growable array, without `vector`.
- [ ] A `HigherPriority` comparison: higher score first, then lower sequence.

**Done when:** tests cover ordering, ties staying in arrival order, empty pop, growth past the starting size, and 1,000 random items.

### T3: CircularQueue

- [ ] Fixed-size array with head index and count.

**Done when:** tests cover full rejection, rotation order, wrap-around, and empty rotate.

### T4: Stack

- [ ] Linked implementation.

**Done when:** tests cover last-in-first-out order, empty pop and peek, and peek not changing contents.

### T5: BST

- [ ] Recursive insert, find and remove, plus in-order and pre-order traversal and height.

**Done when:** tests cover duplicates, all three delete cases, deleting the root, an empty tree, sorted traversal after deletes, and a height comparison for in-order and shuffled keys.

**All owners:** tests also confirm that every allocated node is freed, using a counting test type.

---

## Phase 2: Foundation modules

### T5: OrganisationModule (spec section 8)

- [ ] Insert, find, update and remove, with the exceptions in section 8.
- [ ] Reject any update that changes the Parcel ID.
- [ ] In-order and pre-order visitors, plus filtered search by category and recipient.
- [ ] Menu screens for Task 5.

**Done when:** each section 8 exception has a test, and the tree display looks right on the sample data.

### T4: ActivityModule (spec section 7)

- [ ] Permanent log, undo stack and activity ID generator.
- [ ] Recent list and per-parcel trace, in time order.
- [ ] Mark-reversed support and top-of-stack checks.
- [ ] Menu screens for Task 4, excluding undo execution.

**Done when:** recording, recent list, per-parcel trace and the empty-history message work.

---

## Phase 3: Workflow modules

Work in this order, because each module calls the previous one. Owners may code in parallel against the agreed headers.

### T1: ParcelModule (spec sections 3 and 5)

- [ ] Registration: ID generation, validation, arrival time, queueing.
- [ ] `processNext`, skipping stale entries.
- [ ] `changeStatus`, enforcing the table in section 3.1.
- [ ] `restoreStatus` and `updateFields`.
- [ ] Task 1 menus.

**Done when:** every transition in section 3.1 has an allowed and a rejected test, and regression scenarios 1 and 6 pass.

### T2: ServiceModule (spec section 6.1)

- [ ] Request creation with score calculation and one-active-request rule.
- [ ] `peekNext` and `popNext` with lazy deletion.
- [ ] `pushBack` keeping the original sequence.
- [ ] Active and closed lists, and Task 2 menus.

**Done when:** tests confirm priority order, ties, skipped cancelled requests and rejected closed requests, and regression scenario 4 passes.

### T3: ResourceModule (spec section 6.2)

- [ ] Registration into per-type circular queues.
- [ ] `assign`: one full rotation at most.
- [ ] `release`, with the transaction count increasing only on completion.
- [ ] `setStatus`, including the resource-lost path.
- [ ] Availability and utilisation views, and Task 3 menus.

**Done when:** tests confirm fair rotation, skipped busy resources, full queue, all-busy, wrong type, and duplicate assignment.

---

## Phase 4: Integration

### Integration lead

- [ ] `ParcelSystem`: routing table in spec section 5, `handleNextRequest`, complete, cancel, failed delivery, parcel cancel and delete.
- [ ] CSV loading through the normal registration functions, with spec section 9 error handling.
- [ ] Final menus connecting all five tasks.

### T4: Undo in `ParcelSystem`

- [ ] Eligibility checks and multi-module restore (spec section 7).
- [ ] Discard old entries after a permanent action.
- [ ] Refuse undo when the required resource isn't Available.

**Done when:** regression scenarios 2, 3 and 5 pass, and one parcel completes each route in section 5 with a correct activity trace.

---

## Phase 5: Hardening

**Each owner:**
- [ ] One scripted scenario for every exception in their spec section.
- [ ] Remove debug output and stubs.
- [ ] Comment non-obvious logic, and be ready to trace it in the Q&A.

**Integration lead:**
- [ ] Load test: 500 parcels through the full flow.
- [ ] Record BST height for in-order and shuffled IDs. Owners can use these real numbers in the Q&A.
- [ ] Full run of `scripts/test.sh` on at least two members' computers.

**Optional innovation, only after all of the above passes:**
- [ ] AVL balancing (T5), keeping the same interface.
- [ ] Request ageing (T2).
- [ ] Utilisation summary (T3).

---

## Phase 6: Submission

- [ ] Get answers to the lecturer questions in spec section 14.
- [ ] Remove `archive/` from the submission zip if the lecturer doesn't want it.
- [ ] Clean build from a fresh copy of the zip.
- [ ] Zip named to the required format.
- [ ] Workload matrix in Word, with an explanation for each member.
- [ ] One signed AI Usage Policy form per student, declaring any AI use, including this plan and its setup work.
- [ ] Each member records their roughly 5-minute video: demo, code, data structure reasons, algorithm reasons, and one technical challenge.
- [ ] Each member practises tracing their own code, stating its time complexity, naming edge cases and making a small live change.
- [ ] Attend the face-to-face Q&A.

---

## Requirement coverage

| Spec section | Plan phase |
|---|---|
| 2 Architecture | 0 |
| 3 Status rules | 3 (T1), 4 |
| 4 End-to-end flow | 4 |
| 5 Task 1 | 1, 3, 4 |
| 6 Tasks 2 and 3 | 1, 3, 4 |
| 7 Task 4 | 1, 2, 4 |
| 8 Task 5 | 1, 2 |
| 9 Demo data | 0, 4 |
| 10 Team and workflow | All |
| 12 Testing | 0–5 |
| 13 Submission | 6 |
