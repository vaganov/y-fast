#ifndef _YFAST_INTERNAL_BST_H
#define _YFAST_INTERNAL_BST_H

namespace yfast::internal {

template <typename _Key, typename T>
struct BSTNodeBase {
    typedef _Key Key;

    Key key;
    T* parent;
    T* left;
    T* right;
    unsigned int size;
#ifdef WITH_HEIGHT
    int height;
#endif
};

}

#endif
