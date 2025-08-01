#ifndef _YFAST_IMPL_YFAST_H
#define _YFAST_IMPL_YFAST_H

#include <yfast/internal/concepts.h>
#include <yfast/impl/avl.h>
#include <yfast/internal/yfast.h>

namespace yfast::impl {

template <typename Leaf, unsigned int H, internal::BitExtractorGeneric<typename Leaf::Key> BitExtractor, typename Compare, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash>
class YFastTrie {
public:
    typedef typename Leaf::Key Key;
    typedef AVL<Leaf, Compare> Value;
    typedef internal::XFastLeaf<Key, Value> XFastLeaf;

private:
    static constexpr auto TREE_SPLIT_THRESHOLD = 2 * H;
    static constexpr auto TREE_MERGE_THRESHOLD = H / 4;

public:
    struct Where {
        const YFastTrie* trie;
        XFastLeaf* xleaf;
        Leaf* leaf;
    };

    const Where nowhere = { this, nullptr, nullptr };

private:
    Compare _cmp;
    XFastTrie<XFastLeaf, H, BitExtractor, Compare, Hash> _trie;
    unsigned int _rebuilds;
    std::size_t _size;

public:
    explicit YFastTrie(BitExtractor bx = BitExtractor(), Compare cmp = Compare()): _cmp(cmp), _trie(bx, cmp), _rebuilds(0), _size(0) {}

    [[nodiscard]] std::size_t size() const { return _size; }
    [[nodiscard]] unsigned int rebuilds() const { return _rebuilds; }

    Where leftmost() const {
        auto xleaf = _trie.leftmost();
        auto leaf = xleaf != nullptr ? xleaf->value.leftmost() : nullptr;
        return { this, xleaf, leaf };
    }

    Where rightmost() const {
        auto xleaf = _trie.rightmost();
        auto leaf = xleaf != nullptr ? xleaf->value.rightmost() : nullptr;
        return { this, xleaf, leaf };
    }

    Where find(const Key& key) const {
        auto pred = _trie.pred(key);
        if (pred != nullptr) {
            auto leaf = pred->value.find(key);
            if (leaf != nullptr) {
                return { this, pred, leaf };
            }
        }
        auto succ = (pred != nullptr) ? pred->nxt : _trie.leftmost();
        if (succ != nullptr) {
            auto leaf = succ->value.find(key);
            if (leaf != nullptr) {
                return { this, succ, leaf };
            }
        }
        return { this, nullptr, nullptr };
    }

    Where pred(const Key& key) const {
        auto pred = _trie.pred(key);
        if (pred != nullptr) {
            auto pred_max = pred->value.rightmost();
            if (_cmp(pred_max->key, key)) {
                auto succ = pred->nxt;
                if (succ != nullptr && !_cmp(key, succ->value.leftmost()->key)) {
                    auto leaf = succ->value.pred(key);
                    return { this, succ, leaf };
                }
                else {
                    return { this, pred, pred_max };
                }
            }
            else {
                auto leaf = pred->value.pred(key);
                return { this, pred, leaf };
            }
        }
        else {
            auto succ = _trie.leftmost();
            auto leaf = succ != nullptr ? succ->value.pred(key) : nullptr;
            return { this, succ, leaf };
        }
    }

    Where succ(const Key& key) const {
        auto succ = _trie.succ(key);
        if (succ != nullptr) {
            auto succ_min = succ->value.leftmost();
            if (_cmp(key, succ_min->key)) {
                auto pred = succ->prv;
                if (pred != nullptr && !_cmp(pred->value.rightmost()->key, key)) {
                    auto leaf = pred->value.succ(key);
                    return { this, pred, leaf };
                }
                else {
                    return { this, succ, succ_min };
                }
            }
            else {
                auto leaf = succ->value.succ(key);
                return { this, succ, leaf };
            }
        }
        else {
            auto pred = _trie.rightmost();
            auto leaf = pred != nullptr ? pred->value.succ(key) : nullptr;
            return { this, pred, leaf };
        }
    }

    Where insert(Leaf* leaf) {
        auto pred = _trie.pred(leaf->key);
        auto succ = (pred != nullptr) ? pred->nxt : _trie.leftmost();
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
            ++_rebuilds;
        }

        ++_size;

        return { this, xleaf, leaf };  // TODO: return replaced leaf
    }

    void remove(Leaf* leaf, XFastLeaf* hint = nullptr) {
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
                auto merged = Value::merge(std::move(xleaf->value), std::move(neighbor->value));
                if (merged.size() > TREE_SPLIT_THRESHOLD) {
                    auto split_result = merged.split();
                    auto left = new XFastLeaf(split_result.left.root()->key, std::move(split_result.left));
                    auto right = new XFastLeaf(split_result.right.root()->key, std::move(split_result.right));
                    _trie.insert(left);
                    _trie.insert(right);
                }
                else {
                    _trie.insert(new XFastLeaf(merged.root()->key, std::move(merged)));
                }
                delete xleaf;
                delete neighbor;
                ++_rebuilds;
            }
            else if (xleaf->value.size() == 0) {
                _trie.remove(xleaf);
                delete xleaf;
                ++_rebuilds;
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
    }

    void clear() {
        for (auto xleaf = _trie.leftmost(); xleaf != nullptr; xleaf = xleaf->nxt) {
            delete xleaf;
        }
        _trie.clear();
        _size = 0;
        _rebuilds = 0;
    }

private:
    static XFastLeaf* pick_neighbor(XFastLeaf* xleaf) {
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
};

}

#endif
