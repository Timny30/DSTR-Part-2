#include "ds/BST.hpp"
#include "test_util.hpp"
#include <string>

using TestUtil::Counted;

namespace {
    // Reads the keys back in order, as digits, e.g. keys 2,5,8 give 258.
    int inOrderDigits(BST<int, int>& tree) {
        int digits = 0;
        tree.inOrder([&](const int& key, int&) { digits = digits * 10 + key; });
        return digits;
    }

    void buildSample(BST<int, int>& tree) {
        // Root 5. Left child 3 has children 2 and 4. Right child 8 has left child 7.
        int keys[] = {5, 3, 8, 2, 4, 7};
        for (int key : keys) tree.insert(key, key * 10);
    }
}

int main() {
    {
        BST<int, int> tree;
        CHECK(tree.isEmpty());
        CHECK(tree.height() == 0);
        CHECK(tree.find(1) == nullptr);
        CHECK(!tree.remove(1));
        int visits = 0;
        tree.inOrder([&](const int&, int&) { visits++; });
        CHECK(visits == 0);
    }
    {
        BST<int, int> tree;
        buildSample(tree);
        CHECK(tree.size() == 6);
        CHECK(tree.height() == 3);
        CHECK(inOrderDigits(tree) == 234578);
        CHECK(*tree.find(4) == 40);
        CHECK(tree.find(9) == nullptr);

        // A duplicate is rejected and doesn't overwrite the value.
        CHECK(!tree.insert(4, 999));
        CHECK(*tree.find(4) == 40);
        CHECK(tree.size() == 6);

        // Pre-order visits each parent first, and reports the depth.
        int order = 0;
        int rootDepth = -1;
        tree.preOrder([&](const int& key, int&, int depth) {
            if (order == 0) rootDepth = depth;
            order = order * 10 + key;
        });
        CHECK(order == 532487);
        CHECK(rootDepth == 0);
    }
    {
        // Delete: no child.
        BST<int, int> tree;
        buildSample(tree);
        CHECK(tree.remove(2));
        CHECK(tree.find(2) == nullptr);
        CHECK(inOrderDigits(tree) == 34578);
        CHECK(tree.size() == 5);
    }
    {
        // Delete: one child.
        BST<int, int> tree;
        buildSample(tree);
        CHECK(tree.remove(8));
        CHECK(*tree.find(7) == 70);
        CHECK(inOrderDigits(tree) == 23457);
    }
    {
        // Delete: two children. The successor's value must move with its key.
        BST<int, int> tree;
        buildSample(tree);
        CHECK(tree.remove(3));
        CHECK(*tree.find(4) == 40);
        CHECK(*tree.find(2) == 20);
        CHECK(inOrderDigits(tree) == 24578);
    }
    {
        // Delete: root with two children, where the successor isn't the immediate right child.
        BST<int, int> tree;
        buildSample(tree);
        CHECK(tree.remove(5));
        CHECK(*tree.find(7) == 70);
        CHECK(inOrderDigits(tree) == 23478);
    }
    {
        // Delete everything, then reuse the tree.
        BST<int, int> tree;
        buildSample(tree);
        int keys[] = {5, 3, 8, 2, 4, 7};
        for (int key : keys) CHECK(tree.remove(key));
        CHECK(tree.isEmpty());
        CHECK(tree.height() == 0);
        CHECK(tree.insert(1, 10));
        CHECK(*tree.find(1) == 10);
    }
    {
        // Sorted keys make the tree a line. This is the known limitation of a plain BST.
        BST<int, int> line;
        for (int i = 1; i <= 100; i++) line.insert(i, i);
        CHECK(line.height() == 100);
    }
    {
        BST<std::string, Counted> tree;
        tree.insert("b", Counted(1));
        tree.insert("a", Counted(2));
        tree.insert("c", Counted(3));
        tree.insert("d", Counted(4));
        tree.remove("b");
        tree.remove("d");
        CHECK(Counted::live() == 2);
    }
    CHECK(Counted::live() == 0);
    return TestUtil::finish("BST");
}
