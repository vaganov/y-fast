#include <yfast/impl/xfast.h>
#include <yfast/internal/xfast.h>

#include <gtest/gtest.h>

struct XFastLeaf: public yfast::internal::XFastLeafBase<int, XFastLeaf> {};

TEST(xfast, empty) {
    yfast::impl::XFastTrie<XFastLeaf, 8> trie;
    EXPECT_EQ(trie.size(), 0);
    EXPECT_EQ(trie.pred(0), nullptr);
    EXPECT_EQ(trie.succ(0), nullptr);
}

TEST(xfast, on_target) {
    yfast::impl::XFastTrie<XFastLeaf, 8> trie;
    auto leaf = new XFastLeaf { 1 };
    EXPECT_EQ(trie.insert(leaf), nullptr);

    EXPECT_EQ(trie.pred(1), leaf);
    EXPECT_EQ(trie.pred(1, true), nullptr);
    EXPECT_EQ(trie.succ(1), leaf);
    EXPECT_EQ(trie.succ(1, true), nullptr);

    auto replacement = new XFastLeaf { 1 };
    EXPECT_EQ(trie.insert(replacement), leaf);
}

TEST(xfast, replace) {
    yfast::impl::XFastTrie<XFastLeaf, 8> trie;
    for (auto i = 1; i < 4; ++i) {
        trie.insert(new XFastLeaf { i });
    }
    EXPECT_NE(trie.insert(new XFastLeaf { 2 }), nullptr);
}
