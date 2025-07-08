#ifndef _YFAST_IMPL_YFAST_H
#define _YFAST_IMPL_YFAST_H

#include <functional>
#include <unordered_map>

#include <boost/intrusive/avltree.hpp>

#include <yfast/impl/avl.h>
#include <yfast/impl/xfast.h>
#include <yfast/impl/bit_extractor.h>

#define MY_AVL

namespace yfast::impl {

template <unsigned int H, SelfBalancedNodeGeneric _Leaf, typename _BitExtractor = BitExtractor<typename _Leaf::Key>, typename _Hash = std::unordered_map<typename _BitExtractor::ShiftResult, void*>, typename _Eq = std::equal_to<typename _Leaf::Key>, typename _Compare = std::less<typename _Leaf::Key>>
class yfast_trie {
public:
    typedef _Leaf Leaf;
    typedef typename Leaf::Key Key;

// protected:
    struct XTrieLeaf {
        typedef typename Leaf::Key Key;
#ifdef MY_AVL
        typedef avl<Leaf, _Eq, _Compare> Value;
#else
        struct LeafCompare {
            bool operator() (const Leaf& lhs, const Leaf& rhs) const {
                return lhs.key < rhs.key;  // FIXME
            }
        };
        typedef boost::intrusive::avltree<Leaf, boost::intrusive::compare<LeafCompare>> Value;
#endif

        Key key;
        Value value;
        XTrieLeaf* prv;
        XTrieLeaf* nxt;
    };

    static constexpr auto TREE_SPLIT_THRESHOLD = 2 * H;
    static constexpr auto TREE_MERGE_THRESHOLD = H / 4;

public:
    typedef struct {
        typename XTrieLeaf::Value* tree;
        Leaf* leaf;
    } FindReport;

private:
    _BitExtractor _bx;
    _Eq _eq;
    _Compare _cmp;
    // xfast_trie<H, XTrieLeaf, _BitExtractor, _Hash, _Eq, _Compare> _trie;
    xfast_trie<XTrieLeaf, H, _BitExtractor, _Hash> _trie;

public:
    // explicit yfast_trie(_BitExtractor bx = _BitExtractor(), _Eq eq = _Eq(), _Compare cmp = _Compare()): _eq(eq), _cmp(cmp), _trie(bx, eq, cmp) {}
    explicit yfast_trie(_BitExtractor bx = _BitExtractor(), _Eq eq = _Eq(), _Compare cmp = _Compare()): _eq(eq), _cmp(cmp), _trie(bx) {}

    FindReport find(const Key& key) const;

    Leaf* insert(Leaf* leaf);
    Leaf* remove(Leaf* leaf);

private:
    static XTrieLeaf* pick_neighbor(XTrieLeaf* xleaf);
};

template <unsigned int H, SelfBalancedNodeGeneric _Leaf, typename _BitExtractor, typename _Hash, typename _Eq, typename _Compare>
typename yfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::FindReport yfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::find(const Key& key) const {
    auto pred = _trie.pred(key);
    if (pred != nullptr) {
#ifdef MY_AVL
        auto leaf = pred->value.find(key);
#else
        auto leaf = &(*pred->value.find(key));
#endif
        if (leaf != nullptr) {
            return {&pred->value, leaf};
        }
    }
    auto succ = (pred != nullptr) ? pred->nxt : _trie.succ(key);
    if (succ != nullptr) {
#ifdef MY_AVL
        auto leaf = succ->value.find(key);
#else
        auto leaf = &(*succ->value.find(key));
#endif
        if (leaf != nullptr) {
            return {&pred->value, leaf};
        }
    }
    return {nullptr, nullptr};
}

template <unsigned int H, SelfBalancedNodeGeneric _Leaf, typename _BitExtractor, typename _Hash, typename _Eq, typename _Compare>
typename yfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::Leaf* yfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::insert(Leaf* leaf) {
#ifdef DEBUG
    std::cerr << "yfast::insert key=" << leaf->key << std::endl;
    // _trie.print(std::cerr);
#endif
    auto pred = _trie.pred(leaf->key);
    auto succ = (pred != nullptr) ? pred->nxt : _trie.succ(leaf->key);
    // auto succ = _trie.succ(leaf->key);  // FIXME
#ifdef DEBUG
    std::cerr << "pred=" << (pred != nullptr ? std::to_string(pred->key) : "NULL") << " succ=" << (succ != nullptr ? std::to_string(succ->key) : "NULL") << std::endl;
#endif
    XTrieLeaf* xleaf;

    if (pred != nullptr && succ != nullptr) {
#ifdef MY_AVL
#ifdef DEBUG
        std::cerr << "before min: succ=" << succ->value << std::endl;
#endif
        if (_cmp(leaf->key, succ->value.min())) {
#else
#ifdef DEBUG
        std::cerr << "succ->value.size() = " << succ->value.size() << std::endl;
        std::cerr << "succ->value = ";
        for (const auto& node: succ->value) {
            std::cerr << node.key << " ";
        }
        std::cerr << std::endl;
#endif
        if (_cmp(leaf->key, succ->value.begin()->key)) {
#endif
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
#ifdef MY_AVL
        // xleaf = _trie.insert(new XTrieLeaf(leaf->key, avl<_Leaf, _Eq, _Compare>(_eq, _cmp)));
        xleaf = new XTrieLeaf(leaf->key, avl<_Leaf, _Eq, _Compare>(_eq, _cmp));
        _trie.insert(xleaf);
#else
        // xleaf = _trie.insert(new XTrieLeaf(leaf->key, typename XTrieLeaf::Value()));
        xleaf = new XTrieLeaf(leaf->key, typename XTrieLeaf::Value());
        _trie.insert(xleaf);
#endif
    }

#ifdef MY_AVL
    xleaf->value.insert(leaf);
#else
    xleaf->value.insert_equal(*leaf);
#endif
    if (xleaf->value.size() > TREE_SPLIT_THRESHOLD) {
        _trie.remove(xleaf);
#ifdef MY_AVL
        auto split_result = xleaf->value.split();
        // _trie.insert(new XTrieLeaf(split_result.left.sample(), split_result.left));
        // _trie.insert(new XTrieLeaf(split_result.right.sample(), split_result.right));
        _trie.insert(new XTrieLeaf(split_result.left.sample(), std::move(split_result.left)));
        _trie.insert(new XTrieLeaf(split_result.right.sample(), std::move(split_result.right)));
#else
        // auto begin = xleaf->value.begin();
        // auto root = xleaf->value.root();
        // auto end = xleaf->value.end();
        // auto left = typename XTrieLeaf::Value(true, begin, root);
        // auto right = typename XTrieLeaf::Value(true, root, end);
#ifdef DEBUG
        std::cerr << "tree about to split: ";
        for (const auto& leaf: xleaf->value) {
            std::cerr << leaf.key << " ";
        }
        std::cerr << std::endl;
#endif
        const auto size = xleaf->value.size();
        typename XTrieLeaf::Value left;
        for (auto i = 0; i < size / 2; ++i) {
            // left.push_back(*xleaf->value.unlink_leftmost_without_rebalance());
            auto leftmost = xleaf->value.unlink_leftmost_without_rebalance();
#ifdef DEBUG
            std::cerr << "unlink leftmost=" << leftmost->key << std::endl;
#endif
            left.push_back(*leftmost);
        }
        typename XTrieLeaf::Value right;
        for (auto i = size / 2; i < size; ++i) {
            auto leftmost = xleaf->value.unlink_leftmost_without_rebalance();
#ifdef DEBUG
            std::cerr << "unlink leftmost=" << leftmost->key << std::endl;
#endif
            right.push_back(*leftmost);
        }
#ifdef DEBUG
        std::cerr << "split left=" << left.root()->key << " right=" << right.root()->key << std::endl;
        std::cerr << "left: ";
        for (const auto& leaf: left) {
            std::cerr << leaf.key << " ";
        }
        std::cerr << std::endl;
        std::cerr << "right: ";
        for (const auto& leaf: right) {
            std::cerr << leaf.key << " ";
        }
        std::cerr << std::endl;
#endif
        _trie.insert(new XTrieLeaf(left.root()->key, std::move(left)));
        _trie.insert(new XTrieLeaf(right.root()->key, std::move(right)));
#endif
        delete xleaf;
    }
    return leaf;
}

template <unsigned int H, SelfBalancedNodeGeneric _Leaf, typename _BitExtractor, typename _Hash, typename _Eq, typename _Compare>
typename yfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::Leaf* yfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::remove(Leaf* leaf) {
    XTrieLeaf* xleaf;  // TODO
    xleaf->value.remove(leaf);
    if (xleaf->value.size() < TREE_MERGE_THRESHOLD) {
        auto neighbor = pick_neighbor(xleaf);
        if (neighbor != nullptr) {
            _trie.remove(xleaf);
            _trie.remove(neighbor);
            auto merged = XTrieLeaf::Value::merge(xleaf->value, neighbor->value);  // FIXME
            _trie.insert(new XTrieLeaf(merged.sample(), merged));
            delete xleaf;
            delete neighbor;
        }
    }
    return leaf;
}

template <unsigned int H, SelfBalancedNodeGeneric _Leaf, typename _BitExtractor, typename _Hash, typename _Eq, typename _Compare>
typename yfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::XTrieLeaf* yfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::pick_neighbor(XTrieLeaf* xleaf) {
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
