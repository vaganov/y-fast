#ifndef _YFAST_INTERNAL_XFAST_H
#define _YFAST_INTERNAL_XFAST_H

#include <yfast/internal/aligned.h>

namespace yfast::internal {

template <typename _Key, typename T>
struct XFastLeafBase {
    typedef _Key Key;

    const Key key;
    T* prv;
    T* nxt;
};

template <typename Leaf>
struct XFastNode: private aligned_ptr<2, Leaf> {
    using aligned_ptr<2, Leaf>::value;

    XFastNode(): aligned_ptr<2, Leaf>() {}
    XFastNode(std::uintptr_t value): aligned_ptr<2, Leaf>(value) {}
    XFastNode(Leaf* leaf, bool left_present, bool right_present): aligned_ptr<2, Leaf>(leaf, left_present, right_present) {}

    Leaf* descendant() const { return this->get_ptr(); };
    [[nodiscard]] bool left_present() const { return this->get_bit(0); };
    [[nodiscard]] bool right_present() const { return this->get_bit(1); };

    void set_descendant(Leaf* leaf) { this->set_ptr(leaf); };
    void set_left_present() { this->set_bit(0); };
    void set_right_present() { this->set_bit(1); };
    void clear_left_present() { this->clear_bit(0); };
    void clear_right_present() { this->clear_bit(1); };
};

}

#endif
