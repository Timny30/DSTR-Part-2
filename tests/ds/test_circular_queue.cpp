#include "ds/CircularQueue.hpp"
#include "test_util.hpp"

using TestUtil::Counted;

int main() {
    {
        CircularQueue<int, 3> queue;
        int out = -1;
        CHECK(queue.isEmpty());
        CHECK(!queue.isFull());
        CHECK(!queue.rotate(out));
        CHECK(out == -1);

        CHECK(queue.enqueue(1));
        CHECK(queue.rotate(out) && out == 1);  // a single item rotates to itself
        CHECK(queue.rotate(out) && out == 1);

        CHECK(queue.enqueue(2));
        CHECK(queue.enqueue(3));
        CHECK(queue.isFull());
        CHECK(!queue.enqueue(4));
        CHECK(queue.size() == 3);

        // Rotating many times wraps the index around the array and keeps the order.
        int order = 0;
        for (int i = 0; i < 7; i++) {
            queue.rotate(out);
            order = order * 10 + out;
        }
        CHECK(order == 1231231);
        CHECK(queue.size() == 3);
    }
    {
        // Enqueue after rotating when not full must go to the logical rear.
        CircularQueue<int, 4> queue;
        queue.enqueue(1);
        queue.enqueue(2);
        int out = 0;
        queue.rotate(out);        // 2, 1
        queue.enqueue(3);         // 2, 1, 3
        int order = 0;
        for (int i = 0; i < 3; i++) {
            queue.rotate(out);
            order = order * 10 + out;
        }
        CHECK(order == 213);
    }
    {
        CircularQueue<Counted, 5> queue;
        CHECK(Counted::live() == 5);  // the array is allocated up front
    }
    CHECK(Counted::live() == 0);
    return TestUtil::finish("CircularQueue");
}
