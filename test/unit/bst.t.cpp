#include <iostream>

#include <bst.h>

#include <gtest/gtest.h>

struct Node {
    typedef int Key;

    Key key;
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
    for (auto i = 1; i < 4; ++i) {
        tree.insert(new Node(i));
    }
    std::cerr << tree << std::endl;
}

TEST(bst, remove) {
    bst tree;
    for (auto i = 1; i < 4; ++i) {
        tree.insert(new Node(i));
    }
    auto node = tree.find(1);
    tree.remove(node);
    std::cerr << tree << std::endl;
}
