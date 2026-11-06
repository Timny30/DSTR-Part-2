#pragma once

// Binary search tree with unique keys. Holds the master parcel records.
// Find, insert and remove are O(log n) on average, but O(n) if keys arrive
// in sorted order, because the tree then degrades into a line.
template <typename K, typename V>
class BST {
public:
    BST() : root(nullptr), count(0) {}
    ~BST() { destroy(root); }
    BST(const BST&) = delete;
    BST& operator=(const BST&) = delete;

    // Returns false if the key already exists.
    bool insert(const K& key, const V& value) {
        Node** link = &root;
        while (*link != nullptr) {
            if (key == (*link)->key) return false;
            link = key < (*link)->key ? &(*link)->left : &(*link)->right;
        }
        *link = new Node(key, value);
        count++;
        return true;
    }

    // Returns a pointer to the value, or nullptr if the key isn't found.
    V* find(const K& key) {
        Node* node = root;
        while (node != nullptr) {
            if (key == node->key) return &node->value;
            node = key < node->key ? node->left : node->right;
        }
        return nullptr;
    }

    // Returns false if the key isn't found.
    bool remove(const K& key) {
        Node** link = &root;
        while (*link != nullptr && !(key == (*link)->key)) {
            link = key < (*link)->key ? &(*link)->left : &(*link)->right;
        }
        Node* target = *link;
        if (target == nullptr) return false;

        if (target->left != nullptr && target->right != nullptr) {
            // Two children: copy in the in-order successor, the smallest key
            // on the right, then unlink that node instead. It has no left child.
            Node** successorLink = &target->right;
            while ((*successorLink)->left != nullptr) successorLink = &(*successorLink)->left;
            Node* successor = *successorLink;
            target->key = successor->key;
            target->value = successor->value;
            *successorLink = successor->right;
            delete successor;
        } else {
            // No child or one child: the parent link skips over the target.
            *link = target->left != nullptr ? target->left : target->right;
            delete target;
        }
        count--;
        return true;
    }

    // In-order visits keys in ascending order. Calls fn(key, value).
    template <typename Fn>
    void inOrder(Fn fn) { walkInOrder(root, fn); }

    // Pre-order visits each parent before its children. Calls fn(key, value, depth).
    template <typename Fn>
    void preOrder(Fn fn) { walkPreOrder(root, 0, fn); }

    bool isEmpty() const { return count == 0; }
    int size() const { return count; }
    int height() const { return heightOf(root); }

private:
    struct Node {
        K key;
        V value;
        Node* left;
        Node* right;
        Node(const K& k, const V& v) : key(k), value(v), left(nullptr), right(nullptr) {}
    };
    Node* root;
    int count;

    static void destroy(Node* node) {
        if (node == nullptr) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

    template <typename Fn>
    static void walkInOrder(Node* node, Fn& fn) {
        if (node == nullptr) return;
        walkInOrder(node->left, fn);
        fn(node->key, node->value);
        walkInOrder(node->right, fn);
    }

    template <typename Fn>
    static void walkPreOrder(Node* node, int depth, Fn& fn) {
        if (node == nullptr) return;
        fn(node->key, node->value, depth);
        walkPreOrder(node->left, depth + 1, fn);
        walkPreOrder(node->right, depth + 1, fn);
    }

    static int heightOf(const Node* node) {
        if (node == nullptr) return 0;
        int left = heightOf(node->left);
        int right = heightOf(node->right);
        return 1 + (left > right ? left : right);
    }
};
