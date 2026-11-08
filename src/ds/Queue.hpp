#pragma once

template <typename T>
class Queue {
public:
    Queue() : front(nullptr), rear(nullptr), count(0) {}
    ~Queue() {
        while (front != nullptr) {
            Node* next = front->next;
            delete front;
            front = next;
        }
    }
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    void enqueue(const T& value) {
        Node* node = new Node(value);
        if (rear == nullptr) front = node;
        else rear->next = node;
        rear = node;
        count++;
    }

    bool dequeue(T& out) {
        if (front == nullptr) return false;
        Node* node = front;
        out = node->value;
        front = node->next;
        if (front == nullptr) rear = nullptr;
        delete node;
        count--;
        return true;
    }

    bool peek(T& out) const {
        if (front == nullptr) return false;
        out = front->value;
        return true;
    }

    template <typename Fn>
    void forEach(Fn fn) const {
        for (Node* node = front; node != nullptr; node = node->next) fn(node->value);
    }

    bool isEmpty() const { return count == 0; }
    int size() const { return count; }

private:
    struct Node {
        T value;
        Node* next;
        explicit Node(const T& item) : value(item), next(nullptr) {}
    };
    Node* front;
    Node* rear;
    int count;
};
