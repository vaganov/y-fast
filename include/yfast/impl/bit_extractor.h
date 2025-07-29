#ifndef _YFAST_IMPL_BIT_EXTRACTOR_H
#define _YFAST_IMPL_BIT_EXTRACTOR_H

#include <cstddef>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

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
    class ShiftResult {
        const std::vector<std::byte> _data;

    public:
        const std::span<const std::byte> view;

        ShiftResult() = default;
        explicit ShiftResult(const std::vector<std::byte>& data): _data(data), view(_data.begin(), _data.size()) {}
        explicit ShiftResult(std::vector<std::byte>&& data): _data(std::move(data)), view(_data) {}
        explicit ShiftResult(const std::span<const std::byte>& span): view(span) {}
        ShiftResult(const ShiftResult& other): _data(other._data), view(_data.empty() ? other.view : std::span(_data)) {}
        ShiftResult(ShiftResult&& other) noexcept: _data(other._data), view(_data.empty() ? other.view : std::span(_data)) {}

        bool operator == (const ShiftResult& other) const {
            return std::ranges::equal(view, other.view);
        }
    };

    static bool extract_bit(const std::byte* data, std::size_t size, unsigned int n) {
        const auto s = n / 8;
        if (size <= s) {
            return false;
        }
        const auto r = n % 8;
        const auto mask = std::byte{1} << r;
        return (data[size - s - 1] & mask) != std::byte{0};
    }

    static bool extract_bit(const std::vector<std::byte>& key, unsigned int n) {
        return extract_bit(key.data(), key.size(), n);
    }

    // [l | r] [l | r] [l | r]
    // [l] [r | l] [r | l]
    static ShiftResult shift(const std::byte* data, std::size_t size, unsigned int n) {
        const auto s = n / 8;
        if (size <= s) {
            return {};
        }

        const auto shifted_size = size - s;
        const auto r = n % 8;
        if (r == 0) {
            return ShiftResult(std::span(data, shifted_size));
        }

        const auto l = 8 - r;
        std::vector<std::byte> shifted(shifted_size);
        std::transform(data, data + shifted_size, shifted.begin(), [r] (std::byte b) { return b >> r; });
        for (int i = 1; i < shifted_size; ++i) {
            shifted[i] |= data[i - 1] << l;
        }
        return ShiftResult(std::move(shifted));
    }

    static ShiftResult shift(const std::vector<std::byte>& key, unsigned int n) {
        return shift(key.data(), key.size(), n);
    }
};

template <>
class BitExtractor<std::string> {
public:
    typedef BitExtractor<std::vector<std::byte>>::ShiftResult ShiftResult;

    static bool extract_bit(const std::string& key, unsigned int n) {
        const auto data = reinterpret_cast<const std::byte*>(key.data());
        return BitExtractor<std::vector<std::byte>>::extract_bit(data, key.size(), n);
    }

    static ShiftResult shift(const std::string& key, unsigned int n) {
        const auto data = reinterpret_cast<const std::byte*>(key.data());
        return BitExtractor<std::vector<std::byte>>::shift(data, key.size(), n);
    }
};

}

namespace std {

template <>
struct hash<yfast::impl::BitExtractor<vector<byte>>::ShiftResult> {
    size_t operator () (const yfast::impl::BitExtractor<vector<byte>>::ShiftResult& shift_result) const noexcept {
        return hash<string_view>{}({ reinterpret_cast<const char*>(shift_result.view.data()), shift_result.view.size() });
    }
};

}

#endif
