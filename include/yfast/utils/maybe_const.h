#ifndef _YFAST_UTILS_MAYBE_CONST_H
#define _YFAST_UTILS_MAYBE_CONST_H

namespace yfast::utils {

template <typename T, bool Const>
struct MaybeConst;

template <typename T>
struct MaybeConst<T, true> {
    typedef const T Type;
};

template <typename T>
struct MaybeConst<T, false> {
    typedef T Type;
};

}

#endif
