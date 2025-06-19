#include <iostream>

#include <bst.h>

#include <gtest/gtest.h>

struct Node {
    int key;
    Node* parent;
    Node* left;
    Node* right;
    unsigned int size;
};

template <typename OStream>
OStream& operator << (OStream& os, const Node& node) {
    return os << node.key;
}

typedef yfast::bst<Node> bst;

TEST(bst, smoke) {
    bst tree;
    std::cerr << tree << std::endl;
}

TEST(bst, insert) {
    bst tree;
    tree.insert(new Node(1, nullptr, nullptr, nullptr));
    tree.insert(new Node(2, nullptr, nullptr, nullptr));
    tree.insert(new Node(3, nullptr, nullptr, nullptr));
    std::cerr << tree << std::endl;
}

TEST(bst, remove) {
    bst tree;
    auto node = tree.insert(new Node(1, nullptr, nullptr, nullptr));
    tree.insert(new Node(2, nullptr, nullptr, nullptr));
    tree.insert(new Node(3, nullptr, nullptr, nullptr));
    tree.remove(node);
    std::cerr << tree << std::endl;
}
