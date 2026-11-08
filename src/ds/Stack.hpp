#pragma once

template <typename T>
class Stack {
public:
    Stack() : top(nullptr), count(0) {}
    ~Stack() {
        while (top != nullptr) {
            Node* next = top->next;
            delete top;
            top = next;
        }
    }
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    void push(const T& value) {
        top = new Node(value, top);
        count++;
    }

    bool pop(T& out) {
        if (top == nullptr) return false;
        Node* node = top;
        out = node->value;
        top = node->next;
        delete node;
        count--;
        return true;
    }

    bool peek(T& out) const {
        if (top == nullptr) return false;
        out = top->value;
        return true;
    }

    bool isEmpty() const { return count == 0; }
    int size() const { return count; }

private:
    struct Node {
        T value;
        Node* next;
        Node(const T& item, Node* below) : value(item), next(below) {}
    };
    Node* top;
    int count;
};
