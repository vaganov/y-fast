#include <yfast/impl/bst.h>
#include <yfast/internal/bst.h>

#include <gtest/gtest.h>

struct BSTNode: public yfast::internal::BSTNodeBase<int, BSTNode> {};

TEST(bst, empty) {
    yfast::impl::BST<BSTNode> tree;
    EXPECT_EQ(tree.size(), 0);
    EXPECT_EQ(tree.leftmost(), nullptr);
    EXPECT_EQ(tree.rightmost(), nullptr);
    EXPECT_EQ(tree.find(0), nullptr);
    EXPECT_EQ(tree.pred(0), nullptr);
    EXPECT_EQ(tree.succ(0), nullptr);
}

TEST(bst, insert_empty) {
    yfast::impl::BST<BSTNode> tree;
    auto node = new BSTNode { 1 };
    tree.insert(node);

    EXPECT_EQ(node->left(), nullptr);
    EXPECT_EQ(node->right(), nullptr);
    EXPECT_EQ(node->size, 1);

    EXPECT_EQ(tree.size(), 1);
    EXPECT_EQ(tree.root(), node);
}

TEST(bst, remove_single_root) {
    yfast::impl::BST<BSTNode> tree;
    auto node = new BSTNode { 1 };
    tree.insert(node);

    auto remove_report = tree.remove(node);

    EXPECT_EQ(remove_report.substitution, nullptr);
    EXPECT_EQ(remove_report.subtree_parent, nullptr);
    EXPECT_EQ(remove_report.subtree_child, nullptr);

    EXPECT_EQ(tree.size(), 0);
    EXPECT_EQ(tree.root(), nullptr);
}

TEST(bst, remove_root_with_left_child) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 2, 1 }) {
        tree.insert(new BSTNode { key });
    }

    auto node = tree.find(2);
    ASSERT_NE(node, nullptr);

    auto remove_report = tree.remove(node);

    EXPECT_NE(remove_report.substitution, nullptr);
    EXPECT_EQ(remove_report.subtree_parent, nullptr);
    EXPECT_NE(remove_report.subtree_child, nullptr);
}

TEST(bst, remove_root_with_right_child) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 1, 2 }) {
        tree.insert(new BSTNode { key });
    }

    auto node = tree.find(1);
    ASSERT_NE(node, nullptr);

    auto remove_report = tree.remove(node);

    EXPECT_NE(remove_report.substitution, nullptr);
    EXPECT_EQ(remove_report.subtree_parent, nullptr);
    EXPECT_NE(remove_report.subtree_child, nullptr);
}

TEST(bst, replace_root) {
    yfast::impl::BST<BSTNode> tree;
    tree.insert(new BSTNode { 1 });

    auto replacement = new BSTNode { 1 };
    auto node = tree.insert(replacement);

    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->key, replacement->key);

    EXPECT_EQ(tree.size(), 1);
}

TEST(bst, replace_left_child) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 2, 1 }) {
        tree.insert(new BSTNode { key });
    }

    auto replacement = new BSTNode { 1 };
    auto node = tree.insert(replacement);

    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->key, replacement->key);

    EXPECT_EQ(tree.size(), 2);
}

TEST(bst, replace_right_child) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 1, 2 }) {
        tree.insert(new BSTNode { key });
    }

    auto replacement = new BSTNode { 2 };
    auto node = tree.insert(replacement);

    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->key, replacement->key);

    EXPECT_EQ(tree.size(), 2);
}

//     4
//    / \
//   2   5
//  / \   \
// 1   3   6

TEST(bst, pred_child) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 4, 2, 5, 1, 3, 6 }) {
        tree.insert(new BSTNode { key });
    }
    auto node = tree.find(4);
    auto pred = yfast::impl::BST<BSTNode>::pred(node);
    EXPECT_EQ(pred->key, 3);
}

TEST(bst, pred_non_child) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 4, 2, 5, 1, 3, 6 }) {
        tree.insert(new BSTNode { key });
    }
    auto node = tree.find(3);
    auto pred = yfast::impl::BST<BSTNode>::pred(node);
    EXPECT_EQ(pred->key, 2);
}

TEST(bst, pred_by_key_lt) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 40, 20, 50, 10, 30, 60 }) {
        tree.insert(new BSTNode { key });
    }
    auto pred = tree.pred(35);
    EXPECT_LE(pred->key, 35);
}

TEST(bst, pred_by_key_match) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 4, 2, 5, 1, 3, 6 }) {
        tree.insert(new BSTNode { key });
    }
    auto pred = tree.pred(4);
    EXPECT_LE(pred->key, 4);
}

TEST(bst, pred_by_key_strict) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 4, 2, 5, 1, 3, 6 }) {
        tree.insert(new BSTNode { key });
    }
    auto pred = tree.pred(4, true);
    EXPECT_LT(pred->key, 4);
}

TEST(bst, succ_child) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 4, 2, 5, 1, 3, 6 }) {
        tree.insert(new BSTNode { key });
    }
    auto node = tree.find(4);
    auto succ = yfast::impl::BST<BSTNode>::succ(node);
    EXPECT_EQ(succ->key, 5);
}

TEST(bst, succ_non_child) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 4, 2, 5, 1, 3, 6 }) {
        tree.insert(new BSTNode { key });
    }
    auto node = tree.find(3);
    auto succ = yfast::impl::BST<BSTNode>::succ(node);
    EXPECT_EQ(succ->key, 4);
}

TEST(bst, succ_by_key_lt) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 40, 20, 50, 10, 30, 60 }) {
        tree.insert(new BSTNode { key });
    }
    auto succ = tree.succ(35);
    EXPECT_GE(succ->key, 35);
}

TEST(bst, succ_by_key_match) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 4, 2, 5, 1, 3, 6 }) {
        tree.insert(new BSTNode { key });
    }
    auto succ = tree.succ(3);
    EXPECT_GE(succ->key, 3);
}

TEST(bst, succ_by_key_strict) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 4, 2, 5, 1, 3, 6 }) {
        tree.insert(new BSTNode { key });
    }
    auto succ = tree.succ(3, true);
    EXPECT_GT(succ->key, 3);
}

TEST(bst, insert_non_empty) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 4, 2, 5, 1, 3, 6 }) {
        tree.insert(new BSTNode { key });
    }
    auto node = new BSTNode { 7 };
    tree.insert(node);

    EXPECT_EQ(node->left(), nullptr);
    EXPECT_EQ(node->right(), nullptr);
    EXPECT_EQ(node->size, 1);

    EXPECT_EQ(tree.size(), 7);
    EXPECT_NE(tree.root(), node);
}

TEST(bst, remove_leaf) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 4, 2, 5, 1, 3, 6 }) {
        tree.insert(new BSTNode { key });
    }
    auto node = tree.find(6);
    ASSERT_NE(node, nullptr);
    auto remove_report = tree.remove(node);
    EXPECT_EQ(remove_report.substitution, nullptr);
    EXPECT_NE(remove_report.subtree_parent, nullptr);
    EXPECT_EQ(remove_report.subtree_child, nullptr);
}

TEST(bst, remove_single_child) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 4, 2, 5, 1, 3, 6 }) {
        tree.insert(new BSTNode { key });
    }
    auto node = tree.find(5);
    ASSERT_NE(node, nullptr);
    auto remove_report = tree.remove(node);
    ASSERT_NE(remove_report.substitution, nullptr);
    EXPECT_EQ(remove_report.substitution->key, 6);
    ASSERT_NE(remove_report.subtree_parent, nullptr);
    EXPECT_NE(remove_report.subtree_child, nullptr);
}

TEST(bst, remove_child_succ) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 4, 2, 5, 1, 3, 6 }) {
        tree.insert(new BSTNode { key });
    }
    auto node = tree.find(2);
    ASSERT_NE(node, nullptr);
    auto remove_report = tree.remove(node);
    ASSERT_NE(remove_report.substitution, nullptr);
    EXPECT_EQ(remove_report.substitution->key, 3);
    ASSERT_NE(remove_report.subtree_parent, nullptr);
    EXPECT_EQ(remove_report.subtree_parent->key, 3);
}

//     3
//    / \
//   2   5
//  /   / \
// 1   4   6

TEST(bst, pred_by_key_gt) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 30, 20, 50, 10, 40, 60 }) {
        tree.insert(new BSTNode { key });
    }
    auto pred = tree.pred(35);
    EXPECT_LE(pred->key, 35);
}

TEST(bst, succ_by_key_gt) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 30, 20, 50, 10, 40, 60 }) {
        tree.insert(new BSTNode { key });
    }
    auto succ = tree.succ(35);
    EXPECT_GE(succ->key, 35);
}

TEST(bst, remove_deep_succ) {
    yfast::impl::BST<BSTNode> tree;
    for (auto key: { 3, 2, 5, 1, 4, 6 }) {
        tree.insert(new BSTNode { key });
    }
    auto node = tree.find(3);
    ASSERT_NE(node, nullptr);
    auto remove_report = tree.remove(node);
    ASSERT_NE(remove_report.substitution, nullptr);
    EXPECT_EQ(remove_report.substitution->key, 4);
    ASSERT_NE(remove_report.subtree_parent, nullptr);
    EXPECT_EQ(remove_report.subtree_parent->key, 5);
}
