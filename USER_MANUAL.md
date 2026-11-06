# Smart Campus Parcel System User Manual

This console program tracks a parcel from arrival on campus until it's collected, delivered or returned. Staff use five task menus, and each one handles a different stage.

## Contents

- [Getting started](#getting-started)
- [How a parcel moves through the system](#how-a-parcel-moves-through-the-system)
- [Walkthrough: one parcel from arrival to collection](#walkthrough-one-parcel-from-arrival-to-collection)
- [Task 1: Parcel intake and routing](#task-1-parcel-intake-and-routing)
- [Task 2: Parcel service and collection](#task-2-parcel-service-and-collection)
- [Task 3: Counters and delivery resources](#task-3-counters-and-delivery-resources)
- [Task 4: Parcel activity and records](#task-4-parcel-activity-and-records)
- [Task 5: Parcel organisation](#task-5-parcel-organisation)
- [Troubleshooting](#troubleshooting)

## Getting started

You need the `g++` compiler with C++17 support. Open a terminal in the project folder, the one that contains `build.bat` and the `data` folder.

1. Build the program. You only need to do this once, and again after any code change.

   ```bash
   .\build.bat
   ```

2. Start the program.

   ```bash
   .\assignment.exe
   ```

3. Enter your Staff ID, using 2 to 12 letters or digits, such as `S1`. It's recorded on every activity you perform.
4. Choose a task from the main menu.

The program starts with 8 sample parcels and 6 sample resources, so you can try it straight away.

| Main menu option | Use it to |
| --- | --- |
| 1. Task 1 | Register parcels and choose their routes |
| 2. Task 2 | Create service requests, finish them and handle missing parcels |
| 3. Task 3 | Manage counters and delivery staff, and allocate them to requests |
| 4. Task 4 | View parcel history and undo recent actions |
| 5. Task 5 | Search and organise parcel records |
| 0. Exit | Close the program |

**Using the menus**

- Type a number and press Enter to choose an option. Enter `0` to go back or cancel.
- Answer yes or no questions with `y` or `n`.
- Parcel, request, resource and activity IDs can be typed in upper or lower case.
- Messages start with `[OK]`, `[ERROR]` or `[INFO]`, so you can see the result of each action.

**Your changes aren't saved.** The program only reads its sample files, so closing it discards any new data and the next run starts again from the sample data.

## How a parcel moves through the system

Each parcel moves through the tasks in a fixed order, and each task does one job. A parcel reaches Processing only when Task 2 creates its request.

```text
Task 1           Task 1            Task 2            Task 3             Task 2
Register    -->  Choose route -->  Create request -> Allocate      -->  Finish
Pending          Stays Pending     Processing        Ready, Out for     Complete, or
                      |                              Delivery, or       record a failure
                      |                              Processing            |
                      v                                                    |
                  If held:                         If failed: Ready for <--+
                  On Hold,                         Collection, or On Hold  |
                  no request                                               |
                                                   If completed: Collected,
                                                   Delivered, or Returned <+

Until a final status, Task 2 can report the parcel missing at any stage.
```

Special handling ends at Ready for Collection instead of a final status, so the parcel can then get a follow-up request. Final statuses are Collected, Delivered, Returned and Cancelled.

## Walkthrough: one parcel from arrival to collection

This example takes sample parcel `P0005` through a recipient collection. It assumes you've just started the program and entered your Staff ID, so the IDs match what you'll see.

| Step | From the main menu | Then enter | You should see | Parcel status |
| --- | --- | --- | --- | --- |
| 1. Choose the route | `1` Task 1 | `3`, then route `1`, then `0` to go back | `Parcel P0005 routed to Recipient Collection.` | Pending |
| 2. Create the request | `2` Task 2 | `1`, `P0005`, `n`, then `0` to go back | `Request REQ0001 created for parcel P0005 with priority score 20.` | Processing |
| 3. Allocate a resource | `3` Task 3 | `3`, then `0` to go back | `Request REQ0001 for parcel P0005 assigned to resource RES001.` | Ready for Collection |
| 4. Complete the request | `2` Task 2 | `4`, `1`, `REQ0001`, `TP012345`, then `0` and `0` to go back | `Request REQ0001 finished. Parcel P0005 is now Collected.` | Collected |
| 5. Check the history | `4` Task 4 | `2`, `P0005`, then `0` to go back | Every step above, in order | Collected |

`TP012345` in step 4 is the recipient's ID, which staff check before releasing a parcel. A wrong ID is refused, and the parcel stays where it is.

## Task 1: Parcel intake and routing

Use Task 1 to register parcels as they arrive and choose what happens to each one next.

| Option | What it does | What you enter |
| --- | --- | --- |
| 1. Register a new parcel | Adds a parcel, gives it an ID and places it in the queue | Recipient ID, recipient name, delivery company, parcel type |
| 2. List parcels waiting for a route | Shows the queue, next parcel first | Nothing |
| 3. Choose route for next parcel | Shows the parcel waiting longest and saves its route | A route, or `0` to decide later |
| 4. Change a parcel's route | Changes the route of a parcel that has no request yet | Parcel ID, new route |
| 5. View a parcel | Shows one parcel's details | Parcel ID |
| 6. List parcels by status | Shows every parcel with a chosen status | A status |
| 7. Update parcel details | Changes recipient, company or type. Leave a field blank to keep it | Parcel ID, new details |
| 8. Cancel a parcel | Cancels the parcel, along with any active request | Parcel ID |

**Field rules**

- **Recipient ID:** 3 to 12 letters or digits, such as `TP012345`.
- **Recipient name:** letters, spaces, hyphens, apostrophes and full stops. Full names are fine, such as `Amy Mei Chan`.
- **Delivery company:** required.
- **Parcel type:** Standard Parcel, Large Parcel, Fragile Parcel, Document or Special-Handling Parcel.

**Routes**

| Route | What happens next |
| --- | --- |
| 1. Recipient collection | Create a Recipient Collection request in Task 2 |
| 2. Campus delivery | Create a Campus Delivery request in Task 2 |
| 3. Special handling | Create a Special Handling request in Task 2 |
| 4. Return to sender | Create a Return to Sender request in Task 2 |
| 5. Hold for investigation | The parcel goes straight to On Hold, with no request |

Parcels are routed in arrival order. Choosing `0` leaves the parcel at the front of the queue. Reporting a parcel missing is done in Task 2.

## Task 2: Parcel service and collection

Use Task 2 to create service requests, finish them, and report missing parcels.

| Option | What it does |
| --- | --- |
| 1. Create a Service Request | Creates a request and moves the parcel to Processing |
| 2. Maintain Active Service Requests | Lists active requests, views one by ID, or lists closed requests |
| 3. Determine the Next Request to be Handled | Shows which request Task 3 will allocate next |
| 4. Process and Update Service Requests | Completes or cancels requests, records failures, and handles missing parcels |
| 5. Handle Exceptional Service Conditions | Lists the cases the system rejects, and lets you try some of them |

**Creating a request:** enter the Parcel ID and say whether it's urgent. A parcel routed in Task 1 uses the service from its route. A follow-up request for a Ready for Collection or On Hold parcel asks you to choose the service type, including Re-delivery. A parcel can only have one active request.

**A request and its parcel each have their own status.** A new request shows `Pending` because it's waiting for a resource, while its parcel shows `Processing`. To see the parcel's status, use Task 1 option 5.

**Options under Process and Update Service Requests**

| Option | When to use it | Parcel status afterwards |
| --- | --- | --- |
| 1. Complete a request | The service finished, after a resource was allocated in Task 3 | See the table below |
| 2. Cancel a request | The request is no longer needed | On Hold |
| 3. Record a failure | The service couldn't be finished | Ready for Collection for a delivery, otherwise On Hold |
| 4. Report a parcel missing | The parcel can't be found | Missing, and any active request is cancelled |
| 5. Mark a missing parcel as found | A missing parcel turns up | On Hold |

| Service type | Priority score | Status after completing |
| --- | --- | --- |
| Special Handling | 40 | Ready for Collection |
| Re-delivery | 30 | Delivered |
| Recipient Collection | 20 | Collected, after the recipient's ID is checked |
| Campus Delivery | 20 | Delivered |
| Return to Sender | 10 | Returned |

Marking a request urgent adds 50 to its score. The highest score goes first, and the earlier request wins a tie.

## Task 3: Counters and delivery resources

Use Task 3 to manage counters, desks and delivery staff, and to allocate them to waiting requests. This is the only place resources are allocated.

| Option | What it does |
| --- | --- |
| 1. Register a resource | Adds a resource of a chosen type, with an optional location |
| 2. List resources and availability | Shows each resource's status, current job and completed job count |
| 3. Assign next request automatically | Gives the next request the next free resource of the right type |
| 4. Assign next request to a chosen resource | Does the same using a Resource ID you pick |
| 5. Update resource status | Sets a resource to Available, Occupied, Unavailable or Maintenance |
| 6. Show utilisation | Shows totals for each resource type |

Each service needs a particular resource type, and each type holds up to 16 resources.

| Service type | Resource type | Parcel status after allocation |
| --- | --- | --- |
| Recipient Collection | Collection Counter | Ready for Collection |
| Campus Delivery, Re-delivery | Delivery Personnel | Out for Delivery |
| Special Handling | Special-Handling Counter | Processing |
| Return to Sender | Delivery Desk | Processing |

Automatic allocation takes turns between resources of the same type, skipping any that aren't available.

**If no resource is free,** the request keeps its place and waits. Free a resource or register another, then try again.

**If a resource becomes Unavailable or goes into Maintenance during a job,** its request goes back to waiting in its original place, and the parcel returns to Processing.

## Task 4: Parcel activity and records

Use Task 4 to see what happened to a parcel and reverse a recent mistake. The system records activities automatically.

| Option | What it does |
| --- | --- |
| 1. Show recent activity | Shows the latest 15 activities, oldest first |
| 2. Show a parcel's activity trace | Shows one parcel's full history, most recent activity and current status |
| 3. Undo recent activity | Reverses the most recent activity that can be undone |

**To undo an action**

1. Choose option 3. The program shows the most recent activity that can be undone.
2. Enter that Activity ID.
3. Enter `y` to confirm.

Undo fixes the parcel and any related request and resource together. The original activity stays in the history, marked `REVERSED`, along with a new Undo entry.

| Can be undone | Can't be undone |
| --- | --- |
| Registering, if nothing else still stands on that parcel | Collection confirmation |
| Choosing or changing a route | Service completion, including deliveries |
| Updating parcel details | Return processing |
| Creating a request | Removing a parcel record |
| Allocating a resource | An undo |
| Status changes before a final status, failures and cancellations | |

Undoing a route sends the parcel to the back of the queue. If an older activity can't be restored because a permanent action happened later, trying to undo it discards it and shows the reason, so you can move on to the next one.

## Task 5: Parcel organisation

Use Task 5 to find parcel records and manage them directly. Records are organised by Parcel ID.

| Option | What it does | What you enter |
| --- | --- | --- |
| 1. Insert a parcel record | Adds a parcel with an ID you choose, and places it in the queue | Parcel ID from `P0001` to `P9999`, then the Task 1 details |
| 2. Search by Parcel ID | Finds one parcel | Parcel ID |
| 3. Search by category | Lists parcels of one parcel type | A parcel type |
| 4. Search by recipient | Lists one recipient's parcels | Recipient ID |
| 5. Display all, sorted by ID | Lists every parcel in ID order | Nothing |
| 6. Display tree structure | Shows how records are stored, with the parcel count and tree height | Nothing |
| 7. Update a parcel record | Changes details, the same as Task 1 option 7 | Parcel ID, new details |
| 8. Remove a parcel record | Permanently removes the record | Parcel ID |

A Parcel ID can't be changed or used twice. A parcel with an active request can't be removed, so finish or cancel the request first.

## Troubleshooting

The program explains most problems on screen. In the messages below, IDs are examples.

| Message or problem | What it means | What to do |
| --- | --- | --- |
| `Parcel P0005 has no route. Choose its route in Task 1 first.` | A request needs a route first | Use Task 1 option 3 |
| `Parcels are routed in arrival order. The next parcel is P0005.` | Another parcel has been waiting longer | Route the parcel it names first |
| `Parcel P0005 already has active request REQ0001.` | A parcel can only have one active request | Finish or cancel that request |
| `Request REQ0001 has no resource yet. Allocate one in Task 3 first.` | It can't be completed before allocation | Use Task 3 option 3 |
| `Request REQ0001 must wait. Every Collection Counter is busy or unavailable.` | No matching resource is free | Complete a job, or update or add a resource |
| `Request REQ0001 must wait. No Collection Counter has been registered.` | That resource type doesn't exist yet | Use Task 3 option 1 |
| `Recipient ID doesn't match request REQ0001. The parcel wasn't released.` | The ID entered isn't the recipient's | Check the ID and try again |
| `Resource RES001 is handling request REQ0001. Complete or cancel that request first.` | The resource is still on a job | Finish or cancel the request |
| `Only the most recent reversible activity can be undone, which is ACT0012.` | Undo works newest first | Undo the activity it names first |
| `Undo refused, nothing changed. Resource RES001 is Unavailable.` | Undo needs that resource back | Make it Available, then try again |
| `[WARNING] data/parcels.csv wasn't found` | The program can't find its sample data | Start it from the project folder |
| `cannot open output file assignment.exe: Permission denied` when building | The program is still running | Close it, then build again |
| A parcel's status looks wrong | You may be seeing the request's status | Use Task 1 option 5 to see the parcel's status |
| The menus don't match this manual | You may be running an old build | Close the program and run `.\build.bat` again |
