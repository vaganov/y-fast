#include <unordered_map>

#include <yfast/impl/yfast.h>
#include <yfast/internal/avl.h>

#include <gtest/gtest.h>

struct YFastLeaf: public yfast::internal::AVLNodeBase<int, YFastLeaf> {
    explicit YFastLeaf(int key): AVLNodeBase(key) {}
};

TEST(yfast, empty) {
    yfast::impl::YFastTrie<YFastLeaf, 8> trie;

    auto where = trie.pred(0);
    EXPECT_EQ(where.trie, &trie);
    EXPECT_EQ(where.xleaf, nullptr);
    EXPECT_EQ(where.leaf, nullptr);

    where = trie.succ(0);
    EXPECT_EQ(where.trie, &trie);
    EXPECT_EQ(where.xleaf, nullptr);
    EXPECT_EQ(where.leaf, nullptr);

    where = trie.find(0);
    EXPECT_EQ(where.trie, &trie);
    EXPECT_EQ(where.xleaf, nullptr);
    EXPECT_EQ(where.leaf, nullptr);

    where = trie.leftmost();
    EXPECT_EQ(where.trie, &trie);
    EXPECT_EQ(where.xleaf, nullptr);
    EXPECT_EQ(where.leaf, nullptr);

    where = trie.rightmost();
    EXPECT_EQ(where.trie, &trie);
    EXPECT_EQ(where.xleaf, nullptr);
    EXPECT_EQ(where.leaf, nullptr);
}

TEST(yfast, pred_strict) {
    yfast::impl::YFastTrie<YFastLeaf, 8> trie;
    for (auto i = 0; i < 17; ++i) {
        trie.insert(new YFastLeaf(i));
    }
    auto leftmost = trie.leftmost();
    auto leftmost_xleaf = leftmost.xleaf;
    ASSERT_NE(leftmost_xleaf, nullptr);
    auto rightmost_xleaf = leftmost_xleaf->nxt;
    ASSERT_NE(rightmost_xleaf, nullptr);
    auto leaf = rightmost_xleaf->value.leftmost();
    ASSERT_NE(leaf, nullptr);

    auto where = trie.pred(leaf->key, true);
    EXPECT_EQ(where.xleaf, leftmost_xleaf);
    EXPECT_LT(where.leaf->key, leaf->key);
}

TEST(yfast, succ_strict) {
    yfast::impl::YFastTrie<YFastLeaf, 8> trie;
    for (auto i = 0; i < 17; ++i) {
        trie.insert(new YFastLeaf(i));
    }
    auto leftmost = trie.leftmost();
    auto leftmost_xleaf = leftmost.xleaf;
    ASSERT_NE(leftmost_xleaf, nullptr);
    auto rightmost_xleaf = leftmost_xleaf->nxt;
    ASSERT_NE(rightmost_xleaf, nullptr);
    auto leaf = leftmost_xleaf->value.rightmost();
    ASSERT_NE(leaf, nullptr);

    auto where = trie.succ(leaf->key, true);
    EXPECT_EQ(where.xleaf, rightmost_xleaf);
    EXPECT_GT(where.leaf->key, leaf->key);
}

TEST(yfast, find) {
    yfast::impl::YFastTrie<YFastLeaf, 8> trie;
    for (auto i = 0; i < 17; ++i) {
        trie.insert(new YFastLeaf(i));
    }
    auto leftmost = trie.leftmost();
    auto leftmost_xleaf = leftmost.xleaf;
    ASSERT_NE(leftmost_xleaf, nullptr);
    auto rightmost_xleaf = leftmost_xleaf->nxt;
    ASSERT_NE(rightmost_xleaf, nullptr);
    auto leaf = leftmost_xleaf->value.rightmost();
    ASSERT_NE(leaf, nullptr);

    auto where = trie.find(leaf->key);
    EXPECT_EQ(where.xleaf, leftmost_xleaf);
    EXPECT_EQ(where.leaf, leaf);

    leaf = rightmost_xleaf->value.leftmost();
    ASSERT_NE(leaf, nullptr);

    where = trie.find(leaf->key);
    EXPECT_EQ(where.xleaf, rightmost_xleaf);
    EXPECT_EQ(where.leaf, leaf);
}

TEST(yfast, xleaf_size) {
    yfast::impl::YFastTrie<YFastLeaf, 20> trie;
    for (auto i = 0; i < 1'000'000; ++i) {
        trie.insert(new YFastLeaf(i));
    }
    EXPECT_EQ(trie.size(), 1'000'000);

    for (auto xleaf = trie.leftmost().xleaf; xleaf != nullptr; xleaf = xleaf->nxt) {
        EXPECT_LE(xleaf->value.size(), 40);
        EXPECT_GE(xleaf->value.size(), 5);
    }
}
