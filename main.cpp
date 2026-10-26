#include <iostream>
#include <string>
#include <limits>

using namespace std;

// ==========================================
// CUSTOM DATA STRUCTURES (NO STL ALLOWED)
// ==========================================

// 1. Custom Stack (Task 4 - Activity Management)[cite: 2]
template <typename T>
class CustomStack {
private:
    struct Node {
        T data;
        Node* next;
        Node(T val) : data(val), next(nullptr) {}
    };
    Node* topNode;
public:
    CustomStack() : topNode(nullptr) {}
    void push(T val) {
        Node* newNode = new Node(val);
        newNode->next = topNode;
        topNode = newNode;
    }
    bool pop(T& outVal) {
        if (isEmpty()) return false;
        Node* temp = topNode;
        outVal = temp->data;
        topNode = topNode->next;
        delete temp;
        return true;
    }
    bool isEmpty() { return topNode == nullptr; }
};

// 2. Custom Queue (Task 1 - Pending Parcels)[cite: 2]
template <typename T>
class CustomQueue {
private:
    struct Node {
        T data;
        Node* next;
        Node(T val) : data(val), next(nullptr) {}
    };
    Node *frontNode, *rearNode;
public:
    CustomQueue() : frontNode(nullptr), rearNode(nullptr) {}
    void enqueue(T val) {
        Node* newNode = new Node(val);
        if (rearNode == nullptr) {
            frontNode = rearNode = newNode;
            return;
        }
        rearNode->next = newNode;
        rearNode = newNode;
    }
    bool dequeue(T& outVal) {
        if (isEmpty()) return false;
        Node* temp = frontNode;
        outVal = temp->data;
        frontNode = frontNode->next;
        if (frontNode == nullptr) rearNode = nullptr;
        delete temp;
        return true;
    }
    bool isEmpty() { return frontNode == nullptr; }
};

// 3. Custom Priority Queue (Task 2 - Service Requests)[cite: 2]
template <typename T>
class CustomPriorityQueue {
private:
    struct Node {
        T data;
        int priority;
        Node* next;
        Node(T val, int p) : data(val), priority(p), next(nullptr) {}
    };
    Node* head;
public:
    CustomPriorityQueue() : head(nullptr) {}
    void enqueue(T val, int priority) {
        Node* newNode = new Node(val, priority);
        if (head == nullptr || head->priority < priority) { // Higher number = higher priority
            newNode->next = head;
            head = newNode;
        } else {
            Node* temp = head;
            while (temp->next != nullptr && temp->next->priority >= priority) {
                temp = temp->next;
            }
            newNode->next = temp->next;
            temp->next = newNode;
        }
    }
    bool dequeue(T& outVal) {
        if (isEmpty()) return false;
        Node* temp = head;
        outVal = temp->data;
        head = head->next;
        delete temp;
        return true;
    }
    bool isEmpty() { return head == nullptr; }
};

// 4. Custom Circular Queue (Task 3 - Resource Management)
template <typename T>
class CustomCircularQueue {
private:
    struct Node {
        T data;
        Node* next;
        Node(T val) : data(val), next(nullptr) {}
    };
    Node* rear;
public:
    CustomCircularQueue() : rear(nullptr) {}
    void enqueue(T val) {
        Node* newNode = new Node(val);
        if (rear == nullptr) {
            rear = newNode;
            rear->next = rear; // Point to itself
        } else {
            newNode->next = rear->next;
            rear->next = newNode;
            rear = newNode;
        }
    }
    // Dequeues and immediately re-queues to simulate round-robin allocation
    bool getNextRoundRobin(T& outVal) {
        if (isEmpty()) return false;
        Node* front = rear->next;
        outVal = front->data;
        rear = front; // Rotate the rear pointer to move to the next item
        return true;
    }
    bool isEmpty() { return rear == nullptr; }
};

// 5. Binary Search Tree (Task 5 - Parcel Category Organization)[cite: 2]
template <typename T>
class CustomBST {
private:
    struct TreeNode {
        string key; 
        T data;
        TreeNode* left;
        TreeNode* right;
        TreeNode(string k, T val) : key(k), data(val), left(nullptr), right(nullptr) {}
    };
    TreeNode* root;

    void insertRec(TreeNode*& node, string key, T val) {
        if (node == nullptr) {
            node = new TreeNode(key, val);
        } else if (key < node->key) {
            insertRec(node->left, key, val);
        } else if (key > node->key) {
            insertRec(node->right, key, val);
        } else {
            // Update existing data
            node->data = val;
        }
    }

    void inorderRec(TreeNode* node) {
        if (node != nullptr) {
            inorderRec(node->left);
            cout << "Parcel ID: " << node->key << " | Status: " << node->data.status << endl;
            inorderRec(node->right);
        }
    }

    bool searchRec(TreeNode* node, string key, T& outVal) {
        if (node == nullptr) return false;
        if (node->key == key) {
            outVal = node->data;
            return true;
        }
        if (key < node->key) return searchRec(node->left, key, outVal);
        return searchRec(node->right, key, outVal);
    }

public:
    CustomBST() : root(nullptr) {}
    void insert(string key, T val) { insertRec(root, key, val); }
    void displayInorder() { inorderRec(root); }
    bool search(string key, T& outVal) { return searchRec(root, key, outVal); }
};

// ==========================================
// ENTITY MODELS[cite: 2]
// ==========================================

struct Parcel {
    string parcelID;
    string recipientID;
    string recipientName;
    string deliveryCompany;
    string parcelType;
    string arrivalDateTime;
    string status;
};

struct ServiceRequest {
    string requestID;
    string parcelID;
    string serviceType;
    string status;
};

struct Resource {
    string resourceID;
    string resourceType;
    string status; 
};

struct Activity {
    string activityID;
    string parcelID;
    string activityType;
    string previousStatus;
    string newStatus;
};

// ==========================================
// INTEGRATED SYSTEM MANAGER[cite: 2]
// ==========================================

class SystemManager {
private:
    CustomQueue<Parcel> pendingParcels;                  
    CustomPriorityQueue<ServiceRequest> serviceRequests; 
    CustomCircularQueue<Resource> collectionCounters;    
    CustomStack<Activity> activityHistory;               
    CustomBST<Parcel> parcelDirectory;                   
    int activityCounter = 1;

public:
    void clearInput() {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    // Task 1: Parcel Intake
    void registerParcelMenu() {
        string pID, rID, rName, type;
        cout << "Enter Parcel ID: "; cin >> pID;
        cout << "Enter Recipient ID: "; cin >> rID;
        cout << "Enter Recipient Name: "; cin >> rName;
        cout << "Enter Parcel Type (Standard/Fragile): "; cin >> type;

        Parcel p = {pID, rID, rName, "DefaultCorp", type, "Today", "Received"};
        pendingParcels.enqueue(p);
        parcelDirectory.insert(pID, p);
        recordActivity(pID, "Registration", "None", "Received");
        cout << "\n[SUCCESS] Parcel " << pID << " registered and queued for processing.\n";
    }

    void processNextParcel() {
        Parcel p;
        if (pendingParcels.dequeue(p)) {
            p.status = "Processed";
            parcelDirectory.insert(p.parcelID, p); // Update BST
            recordActivity(p.parcelID, "Processing", "Received", "Processed");
            cout << "\n[SUCCESS] Parcel " << p.parcelID << " processed. Ready for service request.\n";
        } else {
            cout << "\n[ERROR] No pending parcels to process.\n";
        }
    }

    // Task 2: Service Request Creation
    void createServiceRequestMenu() {
        string reqID, pID, type;
        int priority;
        cout << "Enter Request ID: "; cin >> reqID;
        cout << "Enter Parcel ID: "; cin >> pID;
        cout << "Enter Service Type (Collection/Delivery): "; cin >> type;
        cout << "Enter Priority (1-10, 10 is highest): "; cin >> priority;

        // Verify Parcel exists
        Parcel p;
        if (!parcelDirectory.search(pID, p)) {
            cout << "\n[ERROR] Parcel ID not found. Register parcel first.\n";
            return;
        }

        ServiceRequest req = {reqID, pID, type, "Pending"};
        serviceRequests.enqueue(req, priority);
        recordActivity(pID, "Service Requested", p.status, "Awaiting Service");
        
        p.status = "Awaiting Service";
        parcelDirectory.insert(pID, p);
        
        cout << "\n[SUCCESS] Service request " << reqID << " created for Parcel " << pID << ".\n";
    }

    // Task 3: Resource Management 
    void addResourceMenu() {
        string resID, type;
        cout << "Enter Resource ID (e.g., Counter1): "; cin >> resID;
        cout << "Enter Resource Type (Counter/Desk): "; cin >> type;
        
        Resource res = {resID, type, "Available"};
        collectionCounters.enqueue(res);
        cout << "\n[SUCCESS] Resource " << resID << " added to rotation.\n";
    }

    void processHighestPriorityRequest() {
        ServiceRequest req;
        Resource res;
        
        if (serviceRequests.isEmpty()) {
            cout << "\n[ERROR] No active service requests.\n";
            return;
        }
        if (collectionCounters.isEmpty()) {
            cout << "\n[ERROR] No resources available. Please add a resource first.\n";
            return;
        }

        serviceRequests.dequeue(req);
        collectionCounters.getNextRoundRobin(res); // Assign using Circular Queue

        cout << "\n[PROCESSING] Request " << req.requestID << " for Parcel " << req.parcelID 
             << " assigned to Resource: " << res.resourceID << " (" << res.resourceType << ")\n";

        // Update Parcel Status
        Parcel p;
        parcelDirectory.search(req.parcelID, p);
        string oldStatus = p.status;
        p.status = "Completed";
        parcelDirectory.insert(p.parcelID, p);
        
        recordActivity(req.parcelID, "Service Completed", oldStatus, "Completed");
        cout << "[SUCCESS] Service completed and parcel updated.\n";
    }

    // Task 4: Activity Tracking[cite: 2]
    void recordActivity(string pID, string type, string prevStatus, string newStatus) {
        string actID = "ACT-" + to_string(activityCounter++);
        Activity act = {actID, pID, type, prevStatus, newStatus};
        activityHistory.push(act);
    }

    void undoLastActivity() {
        Activity act;
        if (activityHistory.pop(act)) {
            cout << "\n[UNDO] Undoing Activity " << act.activityID << " (" << act.activityType << ")\n";
            Parcel p;
            if (parcelDirectory.search(act.parcelID, p)) {
                p.status = act.previousStatus;
                parcelDirectory.insert(p.parcelID, p);
                cout << "[SUCCESS] Parcel " << act.parcelID << " reverted to status: " << act.previousStatus << "\n";
            }
        } else {
            cout << "\n[ERROR] No activities to undo.\n";
        }
    }

    // Task 5: Traversal[cite: 2]
    void displayAllParcels() {
        cout << "\n--- Parcel Directory (In-Order BST Traversal) ---\n";
        parcelDirectory.displayInorder();
    }
};

// ==========================================
// MAIN INTERFACE[cite: 2]
// ==========================================

int main() {
    SystemManager sys;
    bool isRunning = true;
    int mainChoice, subChoice;

    while (isRunning) {
        cout << "\n=========================================";
        cout << "\n SMART CAMPUS PARCEL MANAGEMENT SYSTEM ";
        cout << "\n=========================================";
        cout << "\n1. Task 1: Parcel Intake & Processing";
        cout << "\n2. Task 2: Service & Collection Management";
        cout << "\n3. Task 3: Resource Management";
        cout << "\n4. Task 4: Activity & Record Management";
        cout << "\n5. Task 5: Parcel Category Organization";
        cout << "\n0. Exit System";
        cout << "\n-----------------------------------------";
        cout << "\nEnter your choice: ";
        
        if (!(cin >> mainChoice)) {
            sys.clearInput();
            continue;
        }

        switch (mainChoice) {
            case 1:
                cout << "\n--- Parcel Intake ---\n1. Register New Parcel\n2. Process Next Pending Parcel\nEnter choice: ";
                cin >> subChoice;
                if (subChoice == 1) sys.registerParcelMenu();
                else if (subChoice == 2) sys.processNextParcel();
                break;
            case 2:
                cout << "\n--- Service Management ---\n1. Create Service Request\n2. Process Highest Priority Request\nEnter choice: ";
                cin >> subChoice;
                if (subChoice == 1) sys.createServiceRequestMenu();
                else if (subChoice == 2) sys.processHighestPriorityRequest();
                break;
            case 3:
                cout << "\n--- Resource Management ---\n1. Register Collection Counter/Resource\nEnter choice: ";
                cin >> subChoice;
                if (subChoice == 1) sys.addResourceMenu();
                break;
            case 4:
                cout << "\n--- Activity History ---\n1. Undo Last System Activity (Pop Stack)\nEnter choice: ";
                cin >> subChoice;
                if (subChoice == 1) sys.undoLastActivity();
                break;
            case 5:
                cout << "\n--- Parcel Directory ---\n1. Display All Parcels\nEnter choice: ";
                cin >> subChoice;
                if (subChoice == 1) sys.displayAllParcels();
                break;
            case 0:
                cout << "Exiting system. Goodbye!\n";
                isRunning = false;
                break;
            default:
                cout << "Invalid choice.\n";
        }
    }
    return 0;
}