#ifndef _YFAST_IMPL_XFAST_H
#define _YFAST_IMPL_XFAST_H

#include <yfast/internal/aligned.h>

// #define DEBUG

namespace yfast::impl {

template <typename Leaf, unsigned int H, typename BitExtractor, typename Hash>
class xfast_trie {
public:
    // typedef Leaf Leaf;
    typedef typename Leaf::Key Key;

protected:
    typedef enum {EMPTY, MISSED_LEFT, ON_TARGET, MISSED_RIGHT} Missed;

    typedef struct {
        Leaf* leaf;
        Missed missed;
        unsigned int level;
    } ApproxReport;

    struct Node: private internal::aligned_ptr<2, Leaf> {
        using internal::aligned_ptr<2, Leaf>::value;

        Node(): internal::aligned_ptr<2, Leaf>() {}
        Node(std::uintptr_t value): internal::aligned_ptr<2, Leaf>(value) {}
        Node(Leaf* leaf, bool left_present, bool right_present): internal::aligned_ptr<2, Leaf>(leaf, left_present, right_present) {}

        Leaf* descendant() const { return this->get_ptr(); };
        [[nodiscard]] bool left_present() const { return this->get_bit(0); };
        [[nodiscard]] bool right_present() const { return this->get_bit(1); };

        void set_descendant(Leaf* leaf) { this->set_ptr(leaf); };
        void set_left_present() { this->set_bit(0); };
        void set_right_present() { this->set_bit(1); };
        void clear_left_present() { this->clear_bit(0); };
        void clear_right_present() { this->clear_bit(1); };
    };

private:
    BitExtractor _bx;
    Node _root;
    Hash _hash[H];

public:
    explicit xfast_trie(BitExtractor bx = BitExtractor()): _bx(bx), _root(nullptr, false, false) {}

    Leaf* find(const Key& key) const;
    Leaf* pred(const Key& key) const;
    Leaf* succ(const Key& key) const;

    void insert(Leaf* leaf);
    void remove(Leaf* leaf);

private:
    ApproxReport approx(const Key& key) const;
};

template <typename Leaf, unsigned int H, typename BitExtractor, typename Hash>
Leaf* xfast_trie<Leaf, H, BitExtractor, Hash>::find(const Key& key) const {
    auto key_ = _bx.shift(key, 0);
    if (_hash[0].contains(key_)) {
        return reinterpret_cast<Leaf*>(_hash[0].at(key_));
    }
    return nullptr;
}

template <typename Leaf, unsigned int H, typename BitExtractor, typename Hash>
Leaf* xfast_trie<Leaf, H, BitExtractor, Hash>::pred(const Key& key) const {
    auto [guess, missed, level] = approx(key);
    switch (missed) {
        case EMPTY:
            return nullptr;
        case MISSED_LEFT:
            return guess;
        case ON_TARGET:
            return guess;
        case MISSED_RIGHT:
            return guess->prv;
    }
}

template <typename Leaf, unsigned int H, typename BitExtractor, typename Hash>
Leaf* xfast_trie<Leaf, H, BitExtractor, Hash>::succ(const Key& key) const {
    auto [guess, missed, level] = approx(key);
    switch (missed) {
        case EMPTY:
            return nullptr;
        case MISSED_LEFT:
            return guess->nxt;
        case ON_TARGET:
            return guess;
        case MISSED_RIGHT:
            return guess;
    }
}

template <typename Leaf, unsigned int H, typename BitExtractor, typename Hash>
void xfast_trie<Leaf, H, BitExtractor, Hash>::insert(Leaf* leaf) {
    Leaf* prv;
    Leaf* nxt;
    auto [guess, missed, level] = approx(leaf->key);
    switch (missed) {
        case EMPTY:
            _root = Node(leaf, false, false);
            prv = nullptr;
            nxt = nullptr;
            break;
        case MISSED_LEFT:
            prv = guess;
            nxt = guess->nxt;
            break;
        case ON_TARGET:
            return;
        case MISSED_RIGHT:
            prv = guess->prv;
            nxt = guess;
            break;
    }

    leaf->prv = prv;
    leaf->nxt = nxt;
    if (prv != nullptr) {
#ifdef DEBUG
        if (prv->key >= leaf->key) {
            asm("nop");
        }
#endif
        prv->nxt = leaf;
    }
    if (nxt != nullptr) {
#ifdef DEBUG
        if (nxt->key <= leaf->key) {
            asm("nop");
        }
#endif
        nxt->prv = leaf;
    }

    bool next_bit;

    if (level == H) {
        next_bit = _bx.extract_bit(leaf->key, H - 1);
        if (next_bit) {
            _root.set_right_present();
        }
        else {
            _root.set_left_present();
        }
    }

    next_bit = _bx.extract_bit(leaf->key, level - 1);
    // next_bit = _bx.extract_bit(leaf->key, level);

    if ((!_root.left_present() && !next_bit) || (!_root.right_present() && next_bit)) {
        _root.set_descendant(leaf);
    }

    for (unsigned int h = H - 1; h >= level; --h) {
    // for (unsigned int h = H - 1; h > level; --h) {
        auto key_prefix = _bx.shift(leaf->key, h);
        std::uintptr_t value = _hash[h].at(key_prefix);
        Node node = value;
#if 0
        if ((!node.left_present() && !next_bit) || (!node.right_present() && next_bit)) {
            node.set_descendant(leaf);
            _hash[h][key_prefix] = node.value;
        }
#else
        if (!node.left_present()) {
            if (leaf->key < node.descendant()->key) {  // FIXME
                node.set_descendant(leaf);
                _hash[h][key_prefix] = node.value;
            }
        }
        if (!node.right_present()) {
            if (leaf->key > node.descendant()->key) {  // FIXME
                node.set_descendant(leaf);
                _hash[h][key_prefix] = node.value;
            }
        }
#endif
    }

    // for (unsigned int h = H - 1; h > 0; --h) {
    // for (unsigned int h = level - 1; h > 0; --h) {
    for (unsigned int h = std::min(level, H - 1); h > 0; --h) {
        next_bit = _bx.extract_bit(leaf->key, h - 1);
        auto key_prefix = _bx.shift(leaf->key, h);
        if (_hash[h].contains(key_prefix)) {
            std::uintptr_t value = _hash[h].at(key_prefix);
            Node node = value;
            if (next_bit) {
                node.set_right_present();
                if (!node.left_present()) {
                    if (leaf->key < node.descendant()->key) {  // FIXME
                        node.set_descendant(leaf);
                    }
                }
            }
            else {
                node.set_left_present();
                if (!node.right_present()) {
                    if (leaf->key > node.descendant()->key) {  // FIXME
                        node.set_descendant(leaf);
                    }
                }
            }
            _hash[h][key_prefix] = node.value;
        }
        else {
            Node node(leaf, !next_bit, next_bit);
            _hash[h][key_prefix] = node.value;
        }
    }

    _hash[0][_bx.shift(leaf->key, 0)] = reinterpret_cast<std::uintptr_t>(leaf);

#ifdef DEBUG
    for (unsigned int h = 0; h < H - 1; ++h) {
        if (_hash[h].size() > 2 * _hash[h + 1].size()) {
            asm("nop");
        }
    }
    for (unsigned int h = 1; h < H; ++h) {
        for (const auto& [key, value] : _hash[h]) {
            Node node = value;
            if ((!node.left_present() || !node.right_present()) && !_hash[0].contains(node.descendant()->key)) {
                asm("nop");
            }
        }
    }
#endif
}

template <typename Leaf, unsigned int H, typename BitExtractor, typename Hash>
void xfast_trie<Leaf, H, BitExtractor, Hash>::remove(Leaf* leaf) {
    auto prv = leaf->prv;
    auto nxt = leaf->nxt;
    if (prv != nullptr) {
        prv->nxt = nxt;
    }
    if (nxt != nullptr) {
        nxt->prv = prv;
    }

#ifdef DEBUG
    if (!_hash[0].contains(_bx.shift(leaf->key, 0))) {
        asm("nop");
    }
#endif
    _hash[0].erase(_bx.shift(leaf->key, 0));

    bool subtree_removed = true;
#ifdef DEBUG
    unsigned int level;
#endif
    // for (unsigned int h = 1; subtree_removed && (h < H); ++h) {
    for (unsigned int h = 1; h < H; ++h) {
        auto key_prefix = _bx.shift(leaf->key, h);
        std::uintptr_t value = _hash[h].at(key_prefix);
        Node node = value;
        if (subtree_removed) {
            if (_bx.extract_bit(leaf->key, h - 1)) {
                node.clear_right_present();
                if (node.left_present()) {
#ifdef DEBUG
                    if (_bx.shift(prv->key, h) != key_prefix) {
                        asm("nop");
                    }
#endif
                    node.set_descendant(prv);
                    _hash[h][key_prefix] = node.value;
                    subtree_removed = false;
#ifdef DEBUG
                    level = h;
#endif
                }
                else {
                    _hash[h].erase(key_prefix);
#ifdef DEBUG
                    if (_hash[h - 1].size() > 2 * _hash[h].size()) {
                        asm("nop");
                    }
#endif
                }
            }
            else {
                node.clear_left_present();
                if (node.right_present()) {
#ifdef DEBUG
                    if (_bx.shift(nxt->key, h) != key_prefix) {
                        asm("nop");
                    }
#endif
                    node.set_descendant(nxt);
                    _hash[h][key_prefix] = node.value;
                    subtree_removed = false;
#ifdef DEBUG
                    level = h;
#endif
                }
                else {
                    _hash[h].erase(key_prefix);
#ifdef DEBUG
                    if (_hash[h - 1].size() > 2 * _hash[h].size()) {
                        asm("nop");
                    }
#endif
                }
            }
        }
        else {
            if (node.descendant() == leaf) {
                if (!node.left_present()) {
#ifdef DEBUG
                    if (_bx.shift(nxt->key, h) != key_prefix) {
                        asm("nop");
                    }
#endif
                    node.set_descendant(nxt);
                }
                if (!node.right_present()) {
#ifdef DEBUG
                    if (_bx.shift(prv->key, h) != key_prefix) {
                        asm("nop");
                    }
#endif
                    node.set_descendant(prv);
                }
                _hash[h][key_prefix] = node.value;
            }
        }
    }

    if (subtree_removed) {
        if (_bx.extract_bit(leaf->key, H - 1)) {
            _root.clear_right_present();
            if (_root.left_present()) {
                _root.set_descendant(prv);
            }
            else {
                _root.set_descendant(nullptr);
            }
        }
        else {
            _root.clear_left_present();
            if (_root.right_present()) {
                _root.set_descendant(nxt);
            }
            else {
                _root.set_descendant(nullptr);
            }
        }
    }

#ifdef DEBUG
    for (unsigned int h = 0; h < H - 1; ++h) {
        if (_hash[h].size() > 2 * _hash[h + 1].size()) {
            asm("nop");
        }
    }
    for (unsigned int h = 1; h < H; ++h) {
        for (const auto& [key, value] : _hash[h]) {
            Node node = value;
            if ((!node.left_present() || !node.right_present()) && !_hash[0].contains(node.descendant()->key)) {
                asm("nop");
            }
        }
    }
#endif
}

template <typename Leaf, unsigned int H, typename BitExtractor, typename Hash>
typename xfast_trie<Leaf, H, BitExtractor, Hash>::ApproxReport xfast_trie<Leaf, H, BitExtractor, Hash>::approx(const Key& key) const {
    if (_root.descendant() == nullptr) {
        return {nullptr, EMPTY, H };
    }

    unsigned int l = 0;
    unsigned int r = H;
    unsigned int m;
    while (r - l > 1) { // ln H
        m = (r + l) / 2;
        if (_hash[m].contains(_bx.shift(key, m))) {
            r = m;
        }
        else {
            l = m;
        }
    }
    if (_hash[l].contains(_bx.shift(key, l))) {
        m = l;
    }
    else {
        m = r;
    }

    if (m == 0) {
        std::uintptr_t value = _hash[0].at(_bx.shift(key, 0));
        return {reinterpret_cast<Leaf*>(value), ON_TARGET, 0 };
    }

    Node node;
    if (m < H) {
        std::uintptr_t value = _hash[m].at(_bx.shift(key, m));
        node.value = value;
    }
    else {
        node = _root;
    }
#ifdef DEBUG
    if (!_hash[0].contains(_bx.shift(node.descendant()->key, 0))) {
        asm("nop");
    }
#endif
    if (node.left_present()) {
        return { node.descendant(), MISSED_LEFT, m };
    }
    else {
        return { node.descendant(), MISSED_RIGHT, m };
    }
}

}

#endif
