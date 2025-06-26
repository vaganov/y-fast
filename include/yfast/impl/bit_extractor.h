#ifndef _YFAST_IMPL_BIT_EXTRACTOR_H
#define _YFAST_IMPL_BIT_EXTRACTOR_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace yfast::impl {

template <typename Key>
class BitExtractor;

template <>
class BitExtractor<std::uint64_t> {
public:
    static bool extract_bit(std::uint64_t key, unsigned int n) { return key & (1 << n); }
    static std::uint64_t shift(std::uint64_t key, unsigned int n) { return key >> n; }
};

template <>
class BitExtractor<std::vector<std::byte>> {
public:
    static bool extract_bit(const std::vector<std::byte>& key, unsigned int n);
    static std::vector<std::byte> shift(const std::vector<std::byte>& key, unsigned int n);
};

// TODO: BitExtractor<std::string>?

inline bool BitExtractor<std::vector<std::byte>>::extract_bit(const std::vector<std::byte>& key, unsigned int n) {
    const auto s = n / 8;
    if (key.size() <= s) {
        return false;
    }
    const auto r = n % 8;
    const auto mask = std::byte{1} << r;
    // return (key[s] & mask) != std::byte{0};
    return (key[key.size() - s - 1] & mask) != std::byte{0};
}

// [l | r] [l | r] [l | r]
// [l] [r | l] [r | l]
inline std::vector<std::byte> BitExtractor<std::vector<std::byte>>::shift(const std::vector<std::byte>& key, unsigned int n) {
    const auto s = n / 8;
    if (key.size() <= s) {
        return {};
    }

    const auto shifted_size = key.size() - s;
    std::vector<std::byte> shifted(shifted_size);
    const auto r = n % 8;
    if (r == 0) {
        std::ranges::copy(key.begin() + s, key.end(), shifted.begin());
        return shifted;
    }

    const auto l = 8 - r;
    // shifted[0] = key[s] >> r;
    // for (int i = 1; i < shifted_size; ++i) {
    //     shifted[i] = (key[i + s - 1] << l) | (key[i + s] >> r);
    // }
    std::transform(key.begin() + s, key.end(), shifted.begin(), [r] (std::byte b) { return b >> r; });
    for (int i = 1; i < shifted_size; ++i) {
        shifted[i] |= key[i + s - 1] << l;
    }
    return shifted;
}

}

#endif
