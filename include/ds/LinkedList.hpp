#pragma once

// Singly linked list with a tail pointer. Stores master records.
// Node addresses never change, so pointers returned by find() stay valid.
// Append is O(1). Find and forEach are O(n).
template <typename T>
class LinkedList {
public:
    LinkedList() : head(nullptr), tail(nullptr), count(0) {}
    ~LinkedList() {
        while (head != nullptr) {
            Node* next = head->next;
            delete head;
            head = next;
        }
    }
    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;

    void append(const T& value) {
        Node* node = new Node(value);
        if (tail == nullptr) head = node;
        else tail->next = node;
        tail = node;
        count++;
    }

    // Returns a pointer to the first item for which pred(item) is true, or nullptr.
    template <typename Pred>
    T* find(Pred pred) {
        for (Node* node = head; node != nullptr; node = node->next) {
            if (pred(node->value)) return &node->value;
        }
        return nullptr;
    }

    template <typename Fn>
    void forEach(Fn fn) {
        for (Node* node = head; node != nullptr; node = node->next) fn(node->value);
    }

    bool isEmpty() const { return count == 0; }
    int size() const { return count; }

private:
    struct Node {
        T value;
        Node* next;
        explicit Node(const T& item) : value(item), next(nullptr) {}
    };
    Node* head;
    Node* tail;
    int count;
};
