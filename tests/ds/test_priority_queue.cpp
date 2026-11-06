#include "ds/PriorityQueue.hpp"
#include "test_util.hpp"
#include <cstdlib>

using TestUtil::Counted;

namespace {
    struct Item {
        int score = 0;
        int sequence = 0;
    };
    struct ItemHigher {
        bool operator()(const Item& a, const Item& b) const {
            if (a.score != b.score) return a.score > b.score;
            return a.sequence < b.sequence;
        }
    };
    struct CountedHigher {
        bool operator()(const Counted& a, const Counted& b) const { return a.value > b.value; }
    };
    Item item(int score, int sequence) {
        Item result;
        result.score = score;
        result.sequence = sequence;
        return result;
    }
}

int main() {
    {
        PriorityQueue<Item, ItemHigher> heap;
        Item out;
        CHECK(heap.isEmpty());
        CHECK(!heap.pop(out));
        CHECK(!heap.peek(out));

        heap.push(item(10, 1));
        heap.push(item(40, 2));
        heap.push(item(20, 3));
        heap.push(item(60, 4));
        CHECK(heap.peek(out) && out.score == 60);
        CHECK(heap.size() == 4);
        CHECK(heap.pop(out) && out.score == 60);
        CHECK(heap.pop(out) && out.score == 40);
        CHECK(heap.pop(out) && out.score == 20);
        CHECK(heap.pop(out) && out.score == 10);
        CHECK(heap.isEmpty());
    }
    {
        // Equal scores leave in arrival order, even though a heap alone isn't stable.
        PriorityQueue<Item, ItemHigher> heap;
        for (int i = 1; i <= 50; i++) heap.push(item(20, i));
        Item out;
        bool inOrder = true;
        for (int i = 1; i <= 50; i++) {
            heap.pop(out);
            if (out.sequence != i) inOrder = false;
        }
        CHECK(inOrder);
    }
    {
        // Growth past the starting size, with random order.
        PriorityQueue<Item, ItemHigher> heap;
        std::srand(7);
        for (int i = 0; i < 1000; i++) heap.push(item(std::rand() % 100, i));
        CHECK(heap.size() == 1000);
        Item previous;
        heap.pop(previous);
        bool sorted = true;
        Item current;
        while (heap.pop(current)) {
            if (ItemHigher()(current, previous)) sorted = false;
            previous = current;
        }
        CHECK(sorted);
    }
    {
        PriorityQueue<Counted, CountedHigher> heap;
        for (int i = 0; i < 100; i++) heap.push(Counted(i));
    }
    CHECK(Counted::live() == 0);
    return TestUtil::finish("PriorityQueue");
}
