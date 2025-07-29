#ifndef _YFAST_INTERNAL_AVL_H
#define _YFAST_INTERNAL_AVL_H

#include <yfast/internal/bst.h>

namespace yfast::internal {

template <typename Key, typename T>
struct AVLNodeBase: public BSTNodeBase<Key, T> {
    using typename BSTNodeBase<Key, T>::Child;

    using BSTNodeBase<Key, T>::_left;
    using BSTNodeBase<Key, T>::_right;

    explicit AVLNodeBase(const Key& key): BSTNodeBase<Key, T>(key) {}

    [[nodiscard]] bool is_left_heavy() const { return Child(_left).get_bit(0); }
    [[nodiscard]] bool is_right_heavy() const { return Child(_right).get_bit(0); }
    [[nodiscard]] bool is_balanced() const { return !is_left_heavy() && !is_right_heavy(); }

    void set_left_heavy() {
        Child left_child = _left;
        left_child.set_bit(0);
        _left = left_child.value;

        Child right_child = _right;
        right_child.clear_bit(0);
        _right = right_child.value;
    }
    void set_right_heavy() {
        Child left_child = _left;
        left_child.clear_bit(0);
        _left = left_child.value;

        Child right_child = _right;
        right_child.set_bit(0);
        _right = right_child.value;
    }
    void set_balanced() {
        Child left_child = _left;
        left_child.clear_bit(0);
        _left = left_child.value;

        Child right_child = _right;
        right_child.clear_bit(0);
        _right = right_child.value;
    }
};

}

#endif
