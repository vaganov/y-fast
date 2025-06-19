#include <iostream>

#include <avl.h>

#include <gtest/gtest.h>

struct Node {
    int key;
    Node* parent;
    Node* left;
    Node* right;
    unsigned int size;
    int balance_factor;
};

template <typename OStream>
OStream& operator << (OStream& os, const Node& node) {
    return os << node.key << " <" << node.balance_factor << ">";
}

typedef yfast::avl<Node> avl;

TEST(avl, smoke) {
    avl tree;
    std::cerr << tree << std::endl;
}

TEST(avl, insert) {
    avl tree;
    for (auto i = 1; i < 16; ++i) {
        tree.insert(new Node(i));
    }
    std::cerr << tree << std::endl;
}

TEST(avl, remove) {
    avl tree;
    Node* node;
    for (auto i = 1; i < 16; ++i) {
        if (i == 8) {
            node = tree.insert(new Node(i));
        }
        else {
            tree.insert(new Node(i));
        }
    }
    tree.remove(node);
    std::cerr << tree << std::endl;
}

TEST(avl, split) {
    avl tree;
    for (auto i = 1; i < 16; ++i) {
        tree.insert(new Node(i));
    }
    auto split_result = tree.split();
    std::cerr << "tree=" << tree << std::endl;
    std::cerr << "left=" << split_result.left << std::endl;
    std::cerr << "right=" << split_result.right << std::endl;
}
