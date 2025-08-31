#include <utility>

#include <yfast/impl/avl.h>
#include <yfast/internal/avl.h>

#include <gtest/gtest.h>

struct AVLNode: public yfast::internal::AVLNodeBase<int, AVLNode> {
    explicit AVLNode(int key): AVLNodeBase(key) {}
};

TEST(avl, split_empty) {
    yfast::impl::AVL<AVLNode> tree;
    auto split_result = tree.split();
    EXPECT_EQ(tree.size(), 0);
    EXPECT_EQ(split_result.left.size(), 0);
    EXPECT_EQ(split_result.right.size(), 0);
    EXPECT_EQ(split_result.left_max, nullptr);
}

TEST(avl, insert_empty) {
    yfast::impl::AVL<AVLNode> tree;
    auto node = new AVLNode(1);
    tree.insert(node);

    EXPECT_FALSE(node->is_left_heavy());
    EXPECT_FALSE(node->is_right_heavy());

    EXPECT_EQ(tree.size(), 1);
    EXPECT_EQ(tree.height(), 1);
}

//     4
//    / \
//   2   5
//  / \   \
// 1   3   6

TEST(avl, insert) {
    yfast::impl::AVL<AVLNode> tree;
    for (auto key = 1; key < 7; ++key) {
        tree.insert(new AVLNode(key));
    }
    auto node = new AVLNode(7);
    tree.insert(node);

    EXPECT_EQ(node->left(), nullptr);
    EXPECT_EQ(node->right(), nullptr);
    EXPECT_EQ(tree.height(), 3);
    EXPECT_FALSE(tree.root()->is_left_heavy());
    EXPECT_FALSE(tree.root()->is_right_heavy());
}

TEST(avl, replace_balanced) {
    yfast::impl::AVL<AVLNode> tree;
    for (auto key = 1; key < 7; ++key) {
        tree.insert(new AVLNode(key));
    }
    auto node = new AVLNode(4);
    auto replaced = tree.insert(node);

    ASSERT_NE(replaced, nullptr);
    EXPECT_EQ(replaced->key, node->key);

    EXPECT_EQ(tree.size(), 6);
}

TEST(avl, replace_left_heavy) {
    yfast::impl::AVL<AVLNode> tree;
    for (auto key = 1; key < 7; ++key) {
        tree.insert(new AVLNode(key));
    }
    auto node = tree.find(3);
    ASSERT_NE(node, nullptr);
    tree.remove(node);

    node = new AVLNode(2);
    auto replaced = tree.insert(node);

    ASSERT_NE(replaced, nullptr);
    EXPECT_EQ(replaced->key, node->key);

    EXPECT_EQ(tree.size(), 5);
}

TEST(avl, replace_right_heavy) {
    yfast::impl::AVL<AVLNode> tree;
    for (auto key = 1; key < 7; ++key) {
        tree.insert(new AVLNode(key));
    }
    auto node = new AVLNode(5);
    auto replaced = tree.insert(node);

    ASSERT_NE(replaced, nullptr);
    EXPECT_EQ(replaced->key, node->key);

    EXPECT_EQ(tree.size(), 6);
}

TEST(avl, remove) {
    yfast::impl::AVL<AVLNode> tree;
    for (auto key = 1; key < 7; ++key) {
        tree.insert(new AVLNode(key));
    }
    auto node = tree.find(4);
    ASSERT_NE(node, nullptr);
    tree.remove(node);
    EXPECT_TRUE(tree.root()->is_left_heavy());
}

//   2       5
//  / \  +  / \
// 1   3   4   6

TEST(avl, merge_even) {
    yfast::impl::AVL<AVLNode> subtree1;
    for (auto key = 1; key < 4; ++key) {
        subtree1.insert(new AVLNode(key));
    }
    yfast::impl::AVL<AVLNode> subtree2;
    for (auto key = 4; key < 7; ++key) {
        subtree2.insert(new AVLNode(key));
    }
    auto tree = yfast::impl::AVL<AVLNode>::merge(std::move(subtree1), std::move(subtree2));
    EXPECT_EQ(tree.size(), 6);
    EXPECT_LE(tree.height(), 3);
    EXPECT_EQ(subtree1.root(), nullptr);
    EXPECT_EQ(subtree2.root(), nullptr);
}

//      4          11
//     / \         /\
//    /   \   +   /  \
//   2     6     10  12
//  / \   / \
// 1   3 5   8
//          / \
//         7   9

TEST(avl, merge_skew_left) {
    yfast::impl::AVL<AVLNode> subtree1;
    for (auto key = 1; key < 10; ++key) {
        subtree1.insert(new AVLNode(key));
    }
    yfast::impl::AVL<AVLNode> subtree2;
    for (auto key = 10; key < 13; ++key) {
        subtree2.insert(new AVLNode(key));
    }
    auto tree = yfast::impl::AVL<AVLNode>::merge(std::move(subtree2), std::move(subtree1));
    EXPECT_EQ(tree.size(), 12);
    EXPECT_LE(tree.height(), 4);
    EXPECT_EQ(subtree1.root(), nullptr);
    EXPECT_EQ(subtree2.root(), nullptr);
}

//   2        7
//  / \   +  / \
// 1   3    /   \
//         5     9
//        / \   / \
//       4   6 8  11
//                /\
//               /  \
//              10  12

TEST(avl, merge_skew_right) {
    yfast::impl::AVL<AVLNode> subtree1;
    for (auto key = 1; key < 4; ++key) {
        subtree1.insert(new AVLNode(key));
    }
    yfast::impl::AVL<AVLNode> subtree2;
    for (auto key = 4; key < 13; ++key) {
        subtree2.insert(new AVLNode(key));
    }
    auto tree = yfast::impl::AVL<AVLNode>::merge(std::move(subtree1), std::move(subtree2));
    EXPECT_EQ(tree.size(), 12);
    EXPECT_LE(tree.height(), 4);
    EXPECT_EQ(subtree1.root(), nullptr);
    EXPECT_EQ(subtree2.root(), nullptr);
}
