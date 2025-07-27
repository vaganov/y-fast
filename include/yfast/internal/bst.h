#ifndef _YFAST_INTERNAL_BST_H
#define _YFAST_INTERNAL_BST_H

#include <cstddef>

namespace yfast::internal {

template <typename _Key, typename T>
struct BSTNodeBase {
    typedef _Key Key;

    const Key key;
    T* parent;
    T* left;
    T* right;
    std::size_t size;
};

}

#endif
