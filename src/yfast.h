#ifndef _YFAST_YFAST_H
#define _YFAST_YFAST_H

#include <functional>
#include <unordered_map>

#include <avl.h>
#include <xfast.h>

namespace yfast {

template <unsigned int H, SelfBalancedNodeGeneric _Leaf, typename _Hash = std::unordered_map<typename _Leaf::Key, void*>, typename _BitExtractor = BitExtractor<typename _Leaf::Key>, typename _Eq = std::equal_to<typename _Leaf::Key>, typename _Compare = std::less<typename _Leaf::Key>>
class yfast_trie {
public:
    typedef _Leaf Leaf;
    typedef typename Leaf::Key Key;

protected:
    struct XTrieLeaf {
        typedef typename Leaf::Key Key;
        typedef avl<Leaf, _Eq, _Compare> Value;

        Key key;
        Value value;
        XTrieLeaf* prv;
        XTrieLeaf* nxt;
    };

public:
    typedef typename XTrieLeaf::Value bst;

private:
    _BitExtractor _bx;
    _Eq _eq;
    _Compare _cmp;
    xfast_trie<H, XTrieLeaf, _Hash, _BitExtractor, _Eq, _Compare> _trie;

public:
    explicit yfast_trie(_BitExtractor bx = _BitExtractor(), _Eq eq = _Eq(), _Compare cmp = _Compare()): _eq(eq), _cmp(cmp), _trie(bx, eq, cmp) {}

    Leaf* find(const Key& key) const;

    Leaf* insert(Leaf* leaf);
};

template <unsigned int H, SelfBalancedNodeGeneric _Leaf, typename _Hash, typename _BitExtractor, typename _Eq, typename _Compare>
yfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::Leaf* yfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::find(const Key& key) const {
    auto pred = _trie.pred(key);
    if (pred != nullptr) {
        auto leaf = pred->value.find(key);
        if (leaf != nullptr) {
            return leaf;
        }
    }
    auto succ = (pred != nullptr) ? pred->nxt : _trie.succ(key);
    if (succ != nullptr) {
        return succ->value.find(key);
    }
    return nullptr;
}

template <unsigned int H, SelfBalancedNodeGeneric _Leaf, typename _Hash, typename _BitExtractor, typename _Eq, typename _Compare>
typename yfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::Leaf* yfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::insert(Leaf* leaf) {
    auto pred = _trie.pred(leaf->key);
    auto succ = (pred != nullptr) ? pred->nxt : _trie.succ(leaf->key);
    XTrieLeaf* xleaf;

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
        xleaf = _trie.insert(new XTrieLeaf(leaf->key, avl<_Leaf, _Eq, _Compare>(_eq, _cmp)));
    }

    xleaf->value.insert(leaf);
    if (xleaf->value.size() > 2 * H) {
        _trie.remove(xleaf);
        auto split_result = xleaf->value.split();
        _trie.insert(new XTrieLeaf(split_result.left.sample(), split_result.left));
        _trie.insert(new XTrieLeaf(split_result.right.sample(), split_result.right));
        delete xleaf;
    }
    return leaf;
}

}

#endif
