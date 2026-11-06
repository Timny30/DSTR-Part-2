#pragma once

// Fixed-size circular queue on an array. Holds resources of one type, and
// rotating through them shares work evenly (round-robin allocation).
// Enqueue and rotate are O(1), because indexes wrap instead of items moving.
template <typename T, int Capacity = 16>
class CircularQueue {
public:
    CircularQueue() : head(0), count(0) {}

    // Returns false when the queue is full.
    bool enqueue(const T& value) {
        if (isFull()) return false;
        items[(head + count) % Capacity] = value;
        count++;
        return true;
    }

    // Returns the front item and moves it to the rear.
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
