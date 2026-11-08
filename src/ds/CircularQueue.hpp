#pragma once

template <typename T, int Capacity = 16>
class CircularQueue {
public:
    CircularQueue() : head(0), count(0) {}

    bool enqueue(const T& value) {
        if (isFull()) return false;
        items[(head + count) % Capacity] = value;
        count++;
        return true;
    }

    bool rotate(T& out) {
        if (isEmpty()) return false;
        out = items[head];
        items[(head + count) % Capacity] = out;
        head = (head + 1) % Capacity;
        return true;
    }

    bool isEmpty() const { return count == 0; }
    bool isFull() const { return count == Capacity; }
    int size() const { return count; }

private:
    T items[Capacity];
    int head;
    int count;
};
