#ifndef _YFAST_IMPL_YFAST_H
#define _YFAST_IMPL_YFAST_H

#define INTERNAL_XLEAF 0

#include <yfast/internal/concepts.h>
#include <yfast/impl/avl.h>
#if INTERNAL_XLEAF
#include <yfast/impl/xfast.h>
#else
#include <yfast/internal/yfast.h>
#endif

namespace yfast::impl {

template <typename Leaf, unsigned int H, internal::BitExtractorGeneric<typename Leaf::Key> BitExtractor, typename Compare, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash>
class YFastTrie {
public:
    typedef typename Leaf::Key Key;
    typedef AVL<Leaf, Compare> Value;

#if INTERNAL_XLEAF
    struct XFastLeaf: public internal::XFastLeafBase<Key, XFastLeaf> {  // TODO: move out, parametrize allocator
        Value value;

        explicit XFastLeaf(const Key& key, Value&& value): internal::XFastLeafBase<Key, XFastLeaf>(key), value(std::move(value)) {}
        XFastLeaf(XFastLeaf&& other) noexcept: internal::XFastLeafBase<Key, XFastLeaf>(std::move(other)), value(std::move(other.value)) {}
    };
#else
    typedef internal::XFastLeaf<Key, Value> XFastLeaf;
#endif

private:
    static constexpr auto TREE_SPLIT_THRESHOLD = 2 * H;
    static constexpr auto TREE_MERGE_THRESHOLD = H / 4;

public:
    typedef struct {
        const YFastTrie* const trie;
        XFastLeaf* xleaf;
        Leaf* leaf;
    } Where;

private:
    Compare _cmp;
    XFastTrie<XFastLeaf, H, BitExtractor, Compare, Hash> _trie;
    // TODO: unsigned int _trie_rebuilds = 0;
    std::size_t _size;

public:
    explicit YFastTrie(BitExtractor bx = BitExtractor(), Compare cmp = Compare()): _cmp(cmp), _trie(bx, cmp), _size(0) {}

    [[nodiscard]] std::size_t size() const { return _size; }

    Where leftmost() const;
    Where rightmost() const;

    Where find(const Key& key) const;
    Where pred(const Key& key) const;
    Where succ(const Key& key) const;

    Where insert(Leaf* leaf);
    void remove(Leaf* leaf, XFastLeaf* hint = nullptr);

private:
    static XFastLeaf* pick_neighbor(XFastLeaf* xleaf);
};

template <typename Leaf, unsigned int H, internal::BitExtractorGeneric<typename Leaf::Key> BitExtractor, typename Compare, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash>
typename YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::Where YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::leftmost() const {
    auto xleaf = _trie.leftmost();
    auto leaf = xleaf != nullptr ? xleaf->value.leftmost() : nullptr;
    return { this, xleaf, leaf };
}

template <typename Leaf, unsigned int H, internal::BitExtractorGeneric<typename Leaf::Key> BitExtractor, typename Compare, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash>
typename YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::Where YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::rightmost() const {
    auto xleaf = _trie.rightmost();
    auto leaf = xleaf != nullptr ? xleaf->value.rightmost() : nullptr;
    return { this, xleaf, leaf };
}

template <typename Leaf, unsigned int H, internal::BitExtractorGeneric<typename Leaf::Key> BitExtractor, typename Compare, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash>
typename YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::Where YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::find(const Key& key) const {
    auto pred = _trie.pred(key);
    if (pred != nullptr) {
        auto leaf = pred->value.find(key);
        if (leaf != nullptr) {
            return { this, pred, leaf };
        }
    }
    auto succ = (pred != nullptr) ? pred->nxt : _trie.succ(key);
    if (succ != nullptr) {
        auto leaf = succ->value.find(key);
        if (leaf != nullptr) {
            return { this, succ, leaf };
        }
    }
#ifdef DEBUG
    int cnt = 0;
    for (auto p = _trie.leftmost(); p != nullptr; p = p->nxt) {
        if (p->value.find(key) != nullptr) {
            asm("nop");
        }
        ++cnt;
    }
#endif
    return { this, nullptr, nullptr };
}

template <typename Leaf, unsigned int H, internal::BitExtractorGeneric<typename Leaf::Key> BitExtractor, typename Compare, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash>
typename YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::Where YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::pred(const Key& key) const {
    auto pred = _trie.pred(key);
    auto leaf = pred->value.pred(key);
    return { this, pred, leaf };  // FIXME
}

template <typename Leaf, unsigned int H, internal::BitExtractorGeneric<typename Leaf::Key> BitExtractor, typename Compare, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash>
typename YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::Where YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::succ(const Key& key) const {
    auto succ = _trie.succ(key);
    auto leaf = succ->value.pred(key);
    return { this, succ, leaf };  // FIXME
}

template <typename Leaf, unsigned int H, internal::BitExtractorGeneric<typename Leaf::Key> BitExtractor, typename Compare, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash>
typename YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::Where YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::insert(Leaf* leaf) {
    auto pred = _trie.pred(leaf->key);
    auto succ = (pred != nullptr) ? pred->nxt : _trie.succ(leaf->key);
    XFastLeaf* xleaf;

    if (pred != nullptr && succ != nullptr) {
        if (_cmp(leaf->key, succ->value.leftmost()->key)) {
            xleaf = pred;
        }
        else {
            xleaf = succ;
        }
    }
    else if (pred != nullptr) {
        xleaf = pred;
    }
    else if (succ != nullptr) {
        xleaf = succ;
    }
    else {
        xleaf = new XFastLeaf(leaf->key, AVL<Leaf, Compare>(_cmp));
        _trie.insert(xleaf);
    }

    xleaf->value.insert(leaf);
    if (xleaf->value.size() > TREE_SPLIT_THRESHOLD) {
        _trie.remove(xleaf);
        auto split_result = xleaf->value.split();
        auto left = new XFastLeaf(split_result.left.root()->key, std::move(split_result.left));
        auto right = new XFastLeaf(split_result.right.root()->key, std::move(split_result.right));
        _trie.insert(left);
        _trie.insert(right);
        delete xleaf;
        xleaf = _cmp(split_result.left_max->key, leaf->key) ? right : left;
    }

    ++_size;
#ifdef DEBUG
    int cnt = 0;
    std::size_t size = 0;
    for (auto p = _trie.leftmost(); p != nullptr; p = p->nxt) {
        ++cnt;
        size += p->value.size();
    }
    if (size != _size) {
        asm("nop");
    }
#endif

    return { this, xleaf, leaf };  // TODO: return replaced leaf
}

template <typename Leaf, unsigned int H, internal::BitExtractorGeneric<typename Leaf::Key> BitExtractor, typename Compare, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash>
void YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::remove(Leaf* leaf, XFastLeaf* hint) {
    auto xleaf = hint;
    if (xleaf == nullptr || xleaf->value.find(leaf->key) != leaf) {
        auto where = find(leaf->key);
        xleaf = where.xleaf;
    }
    if (xleaf == nullptr) {
        return;
    }
    xleaf->value.remove(leaf);
    if (xleaf->value.size() < TREE_MERGE_THRESHOLD) {
        auto neighbor = pick_neighbor(xleaf);
        if (neighbor != nullptr) {
            _trie.remove(xleaf);
            _trie.remove(neighbor);
#if 1
#ifdef DEBUG
            auto size = xleaf->value.size() + neighbor->value.size();
#endif
            auto merged = Value::merge(std::move(xleaf->value), std::move(neighbor->value));  // TODO: split if necessary
#ifdef DEBUG
            if (merged.size() != size) {
                asm("nop");
            }
#endif
#else
            Value merged;
            while (xleaf->value.size() > 0) {
                auto node = const_cast<Leaf*>(xleaf->value.leftmost());
                xleaf->value.remove(node);
                merged.insert(node);
            }
            while (neighbor->value.size() > 0) {
                auto node = const_cast<Leaf*>(neighbor->value.leftmost());
                neighbor->value.remove(node);
                merged.insert(node);
            }
#endif
            _trie.insert(new XFastLeaf(merged.root()->key, std::move(merged)));
            delete xleaf;
            delete neighbor;
        }
        else if (xleaf->value.size() == 0) {
            _trie.remove(xleaf);
            delete xleaf;
        }
    }
    else {
        if (_cmp(xleaf->key, xleaf->value.leftmost()->key) || _cmp(xleaf->value.rightmost()->key, xleaf->key)) {
            _trie.remove(xleaf);
            _trie.insert(new XFastLeaf(xleaf->value.root()->key, std::move(xleaf->value)));
            delete xleaf;
        }
    }

    --_size;
#ifdef DEBUG
    int cnt = 0;
    std::size_t size = 0;
    for (auto p = _trie.leftmost(); p != nullptr; p = p->nxt) {
        ++cnt;
        size += p->value.size();
    }
    if (size != _size) {
        asm("nop");
    }
#endif
}

template <typename Leaf, unsigned int H, internal::BitExtractorGeneric<typename Leaf::Key> BitExtractor, typename Compare, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash>
typename YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::XFastLeaf* YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::pick_neighbor(XFastLeaf* xleaf) {
    auto pred = xleaf->prv;
    auto succ = xleaf->nxt;
    if (pred != nullptr && succ != nullptr) {
        auto pred_merge_size = pred->value.size() + xleaf->value.size();
        auto succ_merge_size = succ->value.size() + xleaf->value.size();
        if (pred_merge_size <= TREE_SPLIT_THRESHOLD && succ_merge_size <= TREE_SPLIT_THRESHOLD) {
            return pred_merge_size < succ_merge_size ? pred : succ;
        }
        if (pred_merge_size <= TREE_SPLIT_THRESHOLD) {
            return pred;
        }
        if (succ_merge_size <= TREE_SPLIT_THRESHOLD) {
            return succ;
        }
        return pred_merge_size > succ_merge_size ? pred : succ;
    }
    if (pred != nullptr) {
        return pred;
    }
    if (succ != nullptr) {
        return succ;
    }
    return nullptr;
}

}

#endif
