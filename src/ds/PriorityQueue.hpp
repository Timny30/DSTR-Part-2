#pragma once

template <typename T, typename HigherPriority>
class PriorityQueue {
public:
    PriorityQueue() : items(new T[INITIAL_CAPACITY]), capacity(INITIAL_CAPACITY), count(0) {}
    ~PriorityQueue() { delete[] items; }
    PriorityQueue(const PriorityQueue&) = delete;
    PriorityQueue& operator=(const PriorityQueue&) = delete;

    void push(const T& value) {
        if (count == capacity) grow();
        items[count] = value;
        siftUp(count);
        count++;
    }

    bool pop(T& out) {
        if (count == 0) return false;
        out = items[0];
        count--;
        if (count > 0) {
            items[0] = items[count];
            siftDown(0);
        }
        return true;
    }

    bool peek(T& out) const {
        if (count == 0) return false;
        out = items[0];
        return true;
    }

    bool isEmpty() const { return count == 0; }
    int size() const { return count; }

private:
    static const int INITIAL_CAPACITY = 8;

    T* items;
    int capacity;
    int count;
    HigherPriority higherPriority;

    void grow() {
        int newCapacity = capacity * 2;
        T* larger = new T[newCapacity];
        for (int i = 0; i < count; i++) larger[i] = items[i];
        delete[] items;
        items = larger;
        capacity = newCapacity;
    }

    void swapItems(int a, int b) {
        T held = items[a];
        items[a] = items[b];
        items[b] = held;
    }

    void siftUp(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;
            if (!higherPriority(items[index], items[parent])) return;
            swapItems(index, parent);
            index = parent;
        }
    }

    void siftDown(int index) {
        while (true) {
            int left = 2 * index + 1;
            int right = left + 1;
            int best = index;
            if (left < count && higherPriority(items[left], items[best])) best = left;
            if (right < count && higherPriority(items[right], items[best])) best = right;
            if (best == index) return;
            swapItems(index, best);
            index = best;
        }
    }
};
