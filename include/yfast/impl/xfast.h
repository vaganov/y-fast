#ifndef _YFAST_IMPL_XFAST_H
#define _YFAST_IMPL_XFAST_H

#include <cstdint>

#include <yfast/internal/concepts.h>
#include <yfast/internal/xfast.h>

namespace yfast::impl {

template <typename Leaf, unsigned int H, internal::BitExtractorGeneric<typename Leaf::Key> BitExtractor, typename Compare, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash>
class XFastTrie {
public:
    typedef typename Leaf::Key Key;

protected:
    typedef enum {EMPTY, MISSED_LEFT, ON_TARGET, MISSED_RIGHT} Missed;

    typedef struct {
        Leaf* leaf;
        Missed missed;
        unsigned int level;
    } ApproxReport;

    typedef internal::XFastNode<Leaf> Node;

private:
    BitExtractor _bx;
    Compare _cmp;
    Node _root;
    Leaf* _leftmost;
    Leaf* _rightmost;
    Hash _hash[H];

public:
    explicit XFastTrie(BitExtractor bx = BitExtractor(), Compare cmp = Compare()): _bx(bx), _cmp(cmp), _root(nullptr, false, false), _leftmost(nullptr), _rightmost(nullptr) {}
    XFastTrie(const XFastTrie& other) = delete;
    XFastTrie(XFastTrie&& other) noexcept: _bx(other._bx), _cmp(other._cmp), _root(other._root), _leftmost(other._leftmost), _rightmost(other._rightmost), _hash(std::move(other._hash)) {
        other._root = Node(nullptr, false, false);
        other._leftmost = nullptr;
        other._rightmost = nullptr;
    }

    Leaf* leftmost() const { return _leftmost; }
    Leaf* rightmost() const { return _rightmost; }

    Leaf* find(const Key& key) const {
        auto key_ = _bx.shift(key, 0);
        if (_hash[0].contains(key_)) {
            return reinterpret_cast<Leaf*>(_hash[0].at(key_));
        }
        return nullptr;
    }

    Leaf* pred(const Key& key) const {
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

    Leaf* succ(const Key& key) const {
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

    void insert(Leaf* leaf) {
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
            prv->nxt = leaf;
        }
        if (nxt != nullptr) {
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

        if ((!_root.left_present() && !next_bit) || (!_root.right_present() && next_bit)) {
            _root.set_descendant(leaf);
        }

        for (unsigned int h = H - 1; h >= level; --h) {
            auto key_prefix = _bx.shift(leaf->key, h);
            std::uintptr_t value = _hash[h].at(key_prefix);
            Node node = value;
            if (!node.left_present()) {
                if (_cmp(leaf->key, node.descendant()->key)) {
                    node.set_descendant(leaf);
                    _hash[h][key_prefix] = node.value;
                }
            }
            if (!node.right_present()) {
                if (_cmp(node.descendant()->key, leaf->key)) {
                    node.set_descendant(leaf);
                    _hash[h][key_prefix] = node.value;
                }
            }
        }

        for (unsigned int h = std::min(level, H - 1); h > 0; --h) {
            next_bit = _bx.extract_bit(leaf->key, h - 1);
            auto key_prefix = _bx.shift(leaf->key, h);
            if (_hash[h].contains(key_prefix)) {
                std::uintptr_t value = _hash[h].at(key_prefix);
                Node node = value;
                if (next_bit) {
                    node.set_right_present();
                    if (!node.left_present()) {
                        if (_cmp(leaf->key, node.descendant()->key)) {
                            node.set_descendant(leaf);
                        }
                    }
                }
                else {
                    node.set_left_present();
                    if (!node.right_present()) {
                        if (_cmp(node.descendant()->key, leaf->key)) {
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

        if (_leftmost == nullptr || _cmp(leaf->key, _leftmost->key)) {
            _leftmost = leaf;
        }
        if (_rightmost == nullptr || _cmp(_rightmost->key, leaf->key)) {
            _rightmost = leaf;
        }
    }

    void remove(Leaf* leaf) {
        auto prv = leaf->prv;
        auto nxt = leaf->nxt;
        if (prv != nullptr) {
            prv->nxt = nxt;
        }
        if (nxt != nullptr) {
            nxt->prv = prv;
        }

        _hash[0].erase(_bx.shift(leaf->key, 0));

        bool subtree_removed = true;
        for (unsigned int h = 1; h < H; ++h) {
            auto key_prefix = _bx.shift(leaf->key, h);
            std::uintptr_t value = _hash[h].at(key_prefix);
            Node node = value;
            if (subtree_removed) {
                if (_bx.extract_bit(leaf->key, h - 1)) {
                    node.clear_right_present();
                    if (node.left_present()) {
                        node.set_descendant(prv);
                        _hash[h][key_prefix] = node.value;
                        subtree_removed = false;
                    }
                    else {
                        _hash[h].erase(key_prefix);
                    }
                }
                else {
                    node.clear_left_present();
                    if (node.right_present()) {
                        node.set_descendant(nxt);
                        _hash[h][key_prefix] = node.value;
                        subtree_removed = false;
                    }
                    else {
                        _hash[h].erase(key_prefix);
                    }
                }
            }
            else {
                if (node.descendant() == leaf) {
                    if (!node.left_present()) {
                        node.set_descendant(nxt);
                    }
                    if (!node.right_present()) {
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
        else {
            if (_root.descendant() == leaf) {
                if (!_root.left_present()) {
                    _root.set_descendant(nxt);
                }
                if (!_root.right_present()) {
                    _root.set_descendant(prv);
                }
            }
        }

        if (_leftmost == leaf) {
            _leftmost = nxt;
        }
        if (_rightmost == leaf) {
            _rightmost = prv;
        }
    }

    void clear() {
        for (unsigned int h = 0; h < H; ++h) {
            _hash[h].clear();
        }
        _root = Node(nullptr, false, false);
        _leftmost = nullptr;
        _rightmost = nullptr;
    }

private:
    ApproxReport approx(const Key& key) const {
        if (!_root.left_present() && !_root.right_present()) {
            return {nullptr, EMPTY, H };
        }

        unsigned int l = 0;
        unsigned int r = H;
        unsigned int m;
        while (r - l > 1) {  // ln H
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
        if (node.left_present()) {
            return { node.descendant(), MISSED_LEFT, m };
        }
        else {
            return { node.descendant(), MISSED_RIGHT, m };
        }
    }
};

}

#endif
