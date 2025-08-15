#ifndef _YFAST_INTERNAL_CONCEPTS_H
#define _YFAST_INTERNAL_CONCEPTS_H

#include <concepts>

namespace yfast::internal {

template <typename Map, typename Key, typename Value>
concept MapGeneric = requires (Map map, Key key) {
    { map[key] } -> std::same_as<Value&>;
    { map.at(key) } -> std::convertible_to<Value>;
    { map.contains(key) } -> std::convertible_to<bool>;
    { map.size() } -> std::convertible_to<std::size_t>;
    { map.erase(key) };
    { map.clear() };
};

template <typename BitExtractor, typename Key>
concept BitExtractorGeneric = requires (BitExtractor bx, Key key, unsigned int n) {
    { bx.extract_bit(key, n) } -> std::convertible_to<bool>;
    { bx.shift(key, n) } -> std::convertible_to<typename BitExtractor::ShiftResult>;
};

}

#endif
