#ifndef _YFAST_ITERATOR_H
#define _YFAST_ITERATOR_H

namespace yfast {

template <typename ForwardIterator>
auto make_reverse_iterator(const ForwardIterator& i) {
    return i.make_reverse();
}

}

#endif
