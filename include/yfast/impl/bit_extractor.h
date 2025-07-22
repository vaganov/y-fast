#ifndef _YFAST_IMPL_BIT_EXTRACTOR_H
#define _YFAST_IMPL_BIT_EXTRACTOR_H

#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>

#include <yfast/internal/hash.h>

namespace yfast::impl {

template <typename Key, typename = void>
class BitExtractor;

template <typename Key>
class BitExtractor<Key, std::enable_if_t<std::is_integral_v<Key>>> {
public:
    typedef Key ShiftResult;

    static bool extract_bit(Key key, unsigned int n) { return key & (1 << n); }
    static Key shift(Key key, unsigned int n) { return key >> n; }
};

template <>
class BitExtractor<std::vector<std::byte>> {
public:
    typedef std::vector<std::byte> ShiftResult;

    static bool extract_bit(const std::byte* data, std::size_t size, unsigned int n);
    static bool extract_bit(const std::vector<std::byte>& key, unsigned int n);
    static std::vector<std::byte> shift(const std::byte* data, std::size_t size, unsigned int n);
    static std::vector<std::byte> shift(const std::vector<std::byte>& key, unsigned int n);
};

inline bool BitExtractor<std::vector<std::byte>>::extract_bit(const std::byte* data, std::size_t size, unsigned int n) {
    const auto s = n / 8;
    if (size <= s) {
        return false;
    }
    const auto r = n % 8;
    const auto mask = std::byte{1} << r;
    return (data[size - s - 1] & mask) != std::byte{0};
}

inline bool BitExtractor<std::vector<std::byte>>::extract_bit(const std::vector<std::byte>& key, unsigned int n) {
    return extract_bit(key.data(), key.size(), n);
}

// [l | r] [l | r] [l | r]
// [l] [r | l] [r | l]
inline std::vector<std::byte> BitExtractor<std::vector<std::byte>>::shift(const std::byte* data, std::size_t size, unsigned int n) {
    const auto s = n / 8;
    if (size <= s) {
        return {};
    }

    const auto shifted_size = size - s;
    std::vector<std::byte> shifted(shifted_size);
    const auto r = n % 8;
    if (r == 0) {
        std::ranges::copy(data, data + shifted_size, shifted.begin());
        return shifted;
    }

    const auto l = 8 - r;
    std::transform(data, data + shifted_size, shifted.begin(), [r] (std::byte b) { return b >> r; });
    for (int i = 1; i < shifted_size; ++i) {
        shifted[i] |= data[i - 1] << l;
    }
    return shifted;
}

inline std::vector<std::byte> BitExtractor<std::vector<std::byte>>::shift(const std::vector<std::byte>& key, unsigned int n) {
    return shift(key.data(), key.size(), n);
}

template <>
class BitExtractor<std::string> {
public:
    typedef std::vector<std::byte> ShiftResult;

    static bool extract_bit(const std::string& key, unsigned int n);
    static std::vector<std::byte> shift(const std::string& key, unsigned int n);
};

inline bool BitExtractor<std::string>::extract_bit(const std::string& key, unsigned int n) {
    auto data = reinterpret_cast<const std::byte*>(key.data());
    return BitExtractor<std::vector<std::byte>>::extract_bit(data, key.size(), n);
}

inline std::vector<std::byte> BitExtractor<std::string>::shift(const std::string& key, unsigned int n) {
    auto data = reinterpret_cast<const std::byte*>(key.data());
    return BitExtractor<std::vector<std::byte>>::shift(data, key.size(), n);
}

}

#endif
