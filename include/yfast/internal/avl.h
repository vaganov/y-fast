#ifndef _YFAST_INTERNAL_AVL_H
#define _YFAST_INTERNAL_AVL_H

#include <yfast/internal/bst.h>

namespace yfast::internal {

template <typename Key, typename T>
struct AVLNodeBase: public BSTNodeBase<Key, T> {
    int balance_factor;

    explicit AVLNodeBase(const Key& key): BSTNodeBase<Key, T>(key) {}
};

}

#endif
