#include "ds/Stack.hpp"
#include "test_util.hpp"

using TestUtil::Counted;

int main() {
    {
        Stack<int> stack;
        int out = -1;
        CHECK(stack.isEmpty());
        CHECK(!stack.pop(out));
        CHECK(!stack.peek(out));
        CHECK(out == -1);

        stack.push(1);
        stack.push(2);
        stack.push(3);
        CHECK(stack.size() == 3);
        CHECK(stack.peek(out) && out == 3);
        CHECK(stack.peek(out) && out == 3);
        CHECK(stack.size() == 3);

        CHECK(stack.pop(out) && out == 3);
        CHECK(stack.pop(out) && out == 2);
        stack.push(4);
        CHECK(stack.pop(out) && out == 4);
        CHECK(stack.pop(out) && out == 1);
        CHECK(stack.isEmpty());
        CHECK(!stack.pop(out));
    }
    {
        Stack<Counted> stack;
        for (int i = 0; i < 50; i++) stack.push(Counted(i));
        Counted out;
        for (int i = 0; i < 10; i++) stack.pop(out);
        CHECK(stack.size() == 40);
    }
    CHECK(Counted::live() == 0);
    return TestUtil::finish("Stack");
}
