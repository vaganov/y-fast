#ifndef _YFAST_INTERNAL_CONCEPTS_H
#define _YFAST_INTERNAL_CONCEPTS_H

#include <concepts>

// #define WITH_HEIGHT

namespace yfast::internal {

template <typename Node>
concept NodeGeneric = requires (Node node) {
    { node.key } -> std::convertible_to<typename Node::Key>;
    { node.parent } -> std::convertible_to<Node*>;
    { node.left } -> std::convertible_to<Node*>;
    { node.right } -> std::convertible_to<Node*>;
    { node.size } -> std::convertible_to<unsigned int>;
#ifdef WITH_HEIGHT
    { node.height } -> std::convertible_to<unsigned int>;
#endif
};

template <typename Node>
concept SelfBalancedNodeGeneric = NodeGeneric<Node> && requires (Node node) {
    { node.balance_factor } -> std::convertible_to<int>;
};

template <typename Leaf>
concept LeafGeneric = requires (Leaf leaf) {
    { leaf.key } -> std::convertible_to<typename Leaf::Key>;
    { leaf.prv } -> std::convertible_to<Leaf*>;
    { leaf.nxt } -> std::convertible_to<Leaf*>;
};

template <typename Eq, typename Key>
concept EqGeneric = requires (Eq eq, Key lhs, Key rhs) {
    { eq(lhs, rhs) } -> std::same_as<bool>;
};

template <typename Compare, typename Key>
concept CompareGeneric = requires (Compare cmp, Key lhs, Key rhs) {
    { cmp(lhs, rhs) } -> std::same_as<bool>;
};

template <typename Map, typename Key, typename Value>
concept MapGeneric = requires (Map map, Key key) {
    { map[key] } -> std::convertible_to<Value>;  // TODO: Value&
    { map.at(key) } -> std::convertible_to<Value>;  // TODO: Value&
    { map.contains(key) } -> std::convertible_to<bool>;
    { map.erase(key) };
};

template <typename BitExtractor, typename Key>
concept BitExtractorGeneric = requires (BitExtractor bx, Key key, unsigned int n) {
    { bx.extract_bit(key, n) } -> std::convertible_to<bool>;
    { bx.shift(key, n) } -> std::convertible_to<typename BitExtractor::ShiftResult>;
};

}

#endif
