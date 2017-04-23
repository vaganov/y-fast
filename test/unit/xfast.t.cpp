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

TEST(XFast, Ctor) {
    XFast xfast;
}

}
