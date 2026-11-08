#pragma once


template <typename K, typename V>
class BST {
public:
    BST() : root(nullptr), count(0) {}
    ~BST() { destroy(root); }
    BST(const BST&) = delete;
    BST& operator=(const BST&) = delete;

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

    V* find(const K& key) {
        Node* node = root;
        while (node != nullptr) {
            if (key == node->key) return &node->value;
            node = key < node->key ? node->left : node->right;
        }
        return nullptr;
    }

    bool remove(const K& key) {
        Node** link = &root;
        while (*link != nullptr && !(key == (*link)->key)) {
            link = key < (*link)->key ? &(*link)->left : &(*link)->right;
        }
        Node* target = *link;
        if (target == nullptr) return false;

        if (target->left != nullptr && target->right != nullptr) {
            Node** successorLink = &target->right;
            while ((*successorLink)->left != nullptr) successorLink = &(*successorLink)->left;
            Node* successor = *successorLink;
            target->key = successor->key;
            target->value = successor->value;
            *successorLink = successor->right;
            delete successor;
        } else {
            *link = target->left != nullptr ? target->left : target->right;
            delete target;
        }
        count--;
        return true;
    }

    template <typename Fn>
    void inOrder(Fn fn) { walkInOrder(root, fn); }

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
