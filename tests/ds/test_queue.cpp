#include "ds/Queue.hpp"
#include "test_util.hpp"

using TestUtil::Counted;

int main() {
    {
        Queue<int> queue;
        int out = -1;
        CHECK(queue.isEmpty());
        CHECK(!queue.dequeue(out));
        CHECK(!queue.peek(out));
        CHECK(out == -1);

        queue.enqueue(1);
        queue.enqueue(2);
        queue.enqueue(3);
        CHECK(queue.size() == 3);
        CHECK(queue.peek(out) && out == 1);
        CHECK(queue.size() == 3);

        int order = 0;
        queue.forEach([&](int v) { order = order * 10 + v; });
        CHECK(order == 123);

        CHECK(queue.dequeue(out) && out == 1);
        CHECK(queue.dequeue(out) && out == 2);
        CHECK(queue.dequeue(out) && out == 3);
        CHECK(queue.isEmpty());
        CHECK(!queue.dequeue(out));

        // The queue must still work after being emptied, including its rear pointer.
        queue.enqueue(4);
        queue.enqueue(5);
        CHECK(queue.dequeue(out) && out == 4);
        CHECK(queue.dequeue(out) && out == 5);
    }
    {
        Queue<Counted> queue;
        for (int i = 0; i < 50; i++) queue.enqueue(Counted(i));
        Counted out;
        for (int i = 0; i < 10; i++) queue.dequeue(out);
        CHECK(queue.size() == 40);
    }
    CHECK(Counted::live() == 0);
    return TestUtil::finish("Queue");
}
