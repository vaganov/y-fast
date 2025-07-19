#ifndef _YFAST_INTERNAL_YFAST_H
#define _YFAST_INTERNAL_YFAST_H

#include <utility>

#include <yfast/impl/xfast.h>

namespace yfast::internal {

template <typename Key, typename Value>
struct XFastLeaf: public XFastLeafBase<Key, XFastLeaf<Key, Value>> {
    Value value;

    explicit XFastLeaf(const Key& key, Value&& value): XFastLeafBase<Key, XFastLeaf>(key), value(std::move(value)) {}
    XFastLeaf(XFastLeaf&& other) noexcept: XFastLeafBase<Key, XFastLeaf>(other), value(std::move(other.value)) {  // TODO: remove
    }
};

}

#endif
