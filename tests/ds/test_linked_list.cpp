#include "ds/LinkedList.hpp"
#include "test_util.hpp"

using TestUtil::Counted;

int main() {
    {
        LinkedList<int> list;
        CHECK(list.isEmpty());
        CHECK(list.size() == 0);
        CHECK(list.find([](int) { return true; }) == nullptr);
        int visits = 0;
        list.forEach([&](int) { visits++; });
        CHECK(visits == 0);

        list.append(1);
        CHECK(!list.isEmpty());
        CHECK(*list.find([](int v) { return v == 1; }) == 1);

        list.append(2);
        list.append(3);
        CHECK(list.size() == 3);

        int order = 0;
        list.forEach([&](int v) { order = order * 10 + v; });
        CHECK(order == 123);
        CHECK(list.find([](int v) { return v == 9; }) == nullptr);

        // Items can be changed through the pointer, and the pointer stays valid after appends.
        int* found = list.find([](int v) { return v == 2; });
        *found = 20;
        list.append(4);
        CHECK(*found == 20);
        CHECK(list.find([](int v) { return v == 20; }) == found);
    }
    {
        LinkedList<Counted> list;
        for (int i = 0; i < 100; i++) list.append(Counted(i));
        CHECK(Counted::live() == 100);
    }
    CHECK(Counted::live() == 0);
    return TestUtil::finish("LinkedList");
}
