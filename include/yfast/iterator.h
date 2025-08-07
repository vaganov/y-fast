#ifndef _YFAST_ITERATOR_H
#define _YFAST_ITERATOR_H

namespace yfast {

/**
 * make reverse iterator
 * @param i forward iterator
 * @return reverse iterator pointing to the same entry as \a --i (or \a rend() if \a i == \a begin())
 */
template <typename ForwardIterator>
auto make_reverse_iterator(const ForwardIterator& i) {
    return i.make_reverse();
}

}

#endif
