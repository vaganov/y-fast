#ifndef _YFAST_INTERNAL_FASTMAP_H
#define _YFAST_INTERNAL_FASTMAP_H

#include <utility>

#include <yfast/internal/avl.h>

namespace yfast::internal {

template <typename Key, typename Value>
struct YFastLeaf;

template <typename Key, typename Value>
struct YFastLeaf: public AVLNodeBase<Key, YFastLeaf<Key, Value>> {
    typedef Value DerefType;

    Value value;

    explicit YFastLeaf(const Key& key): AVLNodeBase<Key, YFastLeaf>(key), value() {}
    explicit YFastLeaf(const Key& key, const Value& value): AVLNodeBase<Key, YFastLeaf>(key), value(value) {}
    explicit YFastLeaf(const Key& key, Value&& value): AVLNodeBase<Key, YFastLeaf>(key), value(std::move(value)) {}

    DerefType& deref() { return value; }
};

template <typename Key>
struct YFastLeaf<Key, void>: public AVLNodeBase<Key, YFastLeaf<Key, void>> {
    typedef const Key DerefType;

    using AVLNodeBase<Key, YFastLeaf>::key;

    explicit YFastLeaf(const Key& key): AVLNodeBase<Key, YFastLeaf>(key) {}

    DerefType& deref() { return key; }
};

}

#endif
