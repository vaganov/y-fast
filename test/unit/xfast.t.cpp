#include <stdint.h>

#include <unordered_map>
#include <string.h>

#include <gtest/gtest.h>

#include <xfast.h>

namespace yfast {

typedef uint8_t Key;
typedef std::string Value;
typedef std::unordered_map<Key, void*> Hash;
typedef xfast<Key, Value, Hash> XFast;

TEST(XFast, InsertZero) {
    XFast xfast;
    const Key key = 0;
    const Value value = "zero";
    xfast.insert(key, value);
    XFast::Leaf* leaf = xfast.find(key);
    ASSERT_TRUE(leaf);
    EXPECT_EQ(value, leaf->value);
}

TEST(XFast, InsertZeroAndOne) {
    XFast xfast;
    const Key key0 = 0;
    const Value value0 = "zero";
    xfast.insert(key0, value0);
    const Key key1 = 1;
    const Value value1 = "one";
    xfast.insert(key1, value1);
    XFast::Leaf* leaf0 = xfast.find(key0);
    ASSERT_TRUE(leaf0);
    EXPECT_EQ(value0, leaf0->value);
    XFast::Leaf* leaf1 = xfast.find(key1);
    ASSERT_TRUE(leaf1);
    EXPECT_EQ(value1, leaf1->value);
}

TEST(XFast, PredSuccOne) {
    XFast xfast;
    const Key key0 = 0;
    const Value value0 = "zero";
    xfast.insert(key0, value0);
    const Key key2 = 2;
    const Value value2 = "two";
    xfast.insert(key2, value2);
    const Key key1 = 1;
    XFast::Leaf* leaf0 = xfast.pred(key1);
    ASSERT_TRUE(leaf0);
    EXPECT_EQ(value0, leaf0->value);
    XFast::Leaf* leaf2 = xfast.succ(key1);
    ASSERT_TRUE(leaf2);
    EXPECT_EQ(value2, leaf2->value);
}

} // namespace yfast
