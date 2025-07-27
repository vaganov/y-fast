#ifndef _YFAST_INTERNAL_HASH_H
#define _YFAST_INTERNAL_HASH_H

#include <cstddef>
#include <string_view>
#include <vector>

namespace std {

template <>
struct hash<vector<byte>> {
    size_t operator()(const vector<byte>& array) const noexcept {
        return hash<string_view>{}({reinterpret_cast<const char*>(array.data()), array.size()});
    }
};

}

#endif
