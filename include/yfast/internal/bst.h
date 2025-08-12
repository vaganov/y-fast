#ifndef _YFAST_INTERNAL_BST_H
#define _YFAST_INTERNAL_BST_H

#include <cstddef>

#include <yfast/utils/aligned.h>

namespace yfast::internal {

template <typename _Key, typename T>
struct BSTNodeBase {
    typedef _Key Key;
    typedef utils::aligned_ptr<1, T> Child;

    const Key key;
    T* parent;
    std::uintptr_t _left;
    std::uintptr_t _right;
    std::size_t size;

    T* left() const { return Child(_left).get_ptr(); }
    T* right() const { return Child(_right).get_ptr(); }

    void set_left(T* node) {
        Child child = _left;
        child.set_ptr(node);
        _left = child.value;
    }
    void set_right(T* node) {
        Child child = _right;
        child.set_ptr(node);
        _right = child.value;
    }
};

}

#endif
