#include <stdint.h>

#include <string.h>

#include <gtest/gtest.h>

#include <trie.h>

namespace yfast {

typedef uint8_t Key;
typedef std::string Value;
typedef trie<Key, Value> Trie;

TEST(Trie, InsertZero) {
    Trie trie;
    const Key key = 0;
    const Value value = "zero";
    trie.insert(key, value);
    Value* p = trie.find(key);
    EXPECT_EQ(value, *p);
}

TEST(Trie, InsertOne) {
    Trie trie;
    const Key key = 1;
    const Value value = "one";
    trie.insert(key, value);
    Value* p = trie.find(key);
    EXPECT_EQ(value, *p);
}

TEST(Trie, InsertMinusOne) {
    Trie trie;
    const Key key = -1;
    const Value value = "minus one";
    trie.insert(key, value);
    Value* p = trie.find(key);
    EXPECT_EQ(value, *p);
}

TEST(Trie, InsertMeander) {
    Trie trie;
    const Key key = 0x55;
    const Value value = "meander";
    trie.insert(key, value);
    Value* p = trie.find(key);
    EXPECT_EQ(value, *p);
}

TEST(Trie, Fill) {
    Trie trie;
    Key key = 0;
    do {
        const Value value = std::to_string((int) key);
        trie.insert(key, value);
    }
    while (0 != ++key);

    do {
        const Value value = std::to_string((int) key);
        Value* p = trie.find(key);
        EXPECT_EQ(value, *p);
    }
    while (0 != ++key);
}

}
