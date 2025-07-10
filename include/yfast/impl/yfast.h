#ifndef _YFAST_IMPL_YFAST_H
#define _YFAST_IMPL_YFAST_H

#include <yfast/internal/concepts.h>
#include <yfast/impl/avl.h>
#include <yfast/impl/xfast.h>

namespace yfast::impl {

template <typename Leaf, unsigned int H, internal::BitExtractorGeneric<typename Leaf::Key> BitExtractor, typename Compare, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash>
class YFastTrie {
public:
    typedef typename Leaf::Key Key;
    typedef AVL<Leaf, Compare> Value;

protected:
    struct XFastLeaf: public internal::XFastLeafBase<Key, XFastLeaf> {  // TODO: move out, parametrize allocator
        Value value;

        explicit XFastLeaf(const Key& key, Value&& value): internal::XFastLeafBase<Key, XFastLeaf>(key), value(std::move(value)) {}
    };

    static constexpr auto TREE_SPLIT_THRESHOLD = 2 * H;
    static constexpr auto TREE_MERGE_THRESHOLD = H / 4;

public:
    typedef struct {
        Value* tree;
        Leaf* leaf;
    } FindReport;

private:
    Compare _cmp;
    XFastTrie<XFastLeaf, H, BitExtractor, Compare, Hash> _trie;

public:
    explicit YFastTrie(BitExtractor bx = BitExtractor(), Compare cmp = Compare()): _cmp(cmp), _trie(bx, cmp) {}

    FindReport find(const Key& key) const;

    Leaf* insert(Leaf* leaf);  // TODO: Value*
};

template <typename Leaf, unsigned int H, internal::BitExtractorGeneric<typename Leaf::Key> BitExtractor, typename Compare, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash>
typename YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::FindReport YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::find(const Key& key) const {
    auto pred = _trie.pred(key);
    if (pred != nullptr) {
        auto leaf = pred->value.find(key);
        if (leaf != nullptr) {
            return {&pred->value, leaf};
        }
    }
    auto succ = (pred != nullptr) ? pred->nxt : _trie.succ(key);
    if (succ != nullptr) {
        auto leaf = succ->value.find(key);
        if (leaf != nullptr) {
            return {&pred->value, leaf};
        }
    }
    return {nullptr, nullptr};
}

template <typename Leaf, unsigned int H, internal::BitExtractorGeneric<typename Leaf::Key> BitExtractor, typename Compare, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash>
Leaf* YFastTrie<Leaf, H, BitExtractor, Compare, Hash>::insert(Leaf* leaf) {
    auto pred = _trie.pred(leaf->key);
    auto succ = (pred != nullptr) ? pred->nxt : _trie.succ(leaf->key);
    XFastLeaf* xleaf;

    if (pred != nullptr && succ != nullptr) {
        if (_cmp(leaf->key, succ->value.min())) {
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
        _trie.insert(new XFastLeaf(split_result.left.sample(), std::move(split_result.left)));
        _trie.insert(new XFastLeaf(split_result.right.sample(), std::move(split_result.right)));
        delete xleaf;
    }
    return leaf;
}

}

#endif
