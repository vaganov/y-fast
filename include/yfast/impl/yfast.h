#ifndef _YFAST_IMPL_YFAST_H
#define _YFAST_IMPL_YFAST_H

#include <functional>
#include <memory>

#include <yfast/impl/avl.h>
#include <yfast/internal/concepts.h>
#include <yfast/internal/bit_extractor.h>
#include <yfast/internal/yfast.h>
#include <yfast/internal/default_hash.h>

namespace yfast::impl {

template <
    typename Leaf,
    unsigned int H,
    internal::BitExtractorGeneric<typename Leaf::Key> BitExtractor = internal::BitExtractor<typename Leaf::Key>,
    internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash = internal::DefaultHash<typename BitExtractor::ShiftResult, std::uintptr_t>,
    typename Compare = std::less<typename Leaf::Key>,
    typename ArbitraryAllocator = std::allocator<typename Leaf::Key>
>
class YFastTrie {
    static_assert(H >= 8, "Key length too short");

public:
    typedef typename Leaf::Key Key;
    typedef AVL<Leaf, Compare> Value;
    typedef internal::XFastLeaf<Key, Value> XFastLeaf;

private:
    typedef typename std::allocator_traits<ArbitraryAllocator>::template rebind_alloc<XFastLeaf> Alloc;

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
    Alloc _alloc;
    Compare _cmp;
    XFastTrie<XFastLeaf, H, BitExtractor, Hash, Compare> _trie;
    unsigned int _rebuilds;
    std::size_t _size;

public:
    explicit YFastTrie(BitExtractor bx = BitExtractor(), Compare cmp = Compare(), ArbitraryAllocator alloc = ArbitraryAllocator()): _alloc(alloc), _cmp(cmp), _trie(bx, cmp), _rebuilds(0), _size(0) {}
    YFastTrie(YFastTrie&& other) noexcept: _alloc(other._alloc), _cmp(other._cmp), _trie(std::move(other._trie)), _rebuilds(other._rebuilds), _size(other._size) {
        other._size = 0;
        other._rebuilds = 0;
    }

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

    Where pred(const Key& key, bool strict = false) const {
        auto pred = _trie.pred(key, strict);
        if (pred != nullptr) {
            auto pred_max = pred->value.rightmost();
            if (_cmp(pred_max->key, key)) {
                auto succ = pred->nxt;
                if (succ != nullptr && !_cmp(key, succ->value.leftmost()->key)) {
                    auto leaf = succ->value.pred(key, strict);
                    if (leaf != nullptr) {
                        return { this, succ, leaf };
                    }
                    else {
                        return { this, pred, pred_max };
                    }
                }
                else {
                    return { this, pred, pred_max };
                }
            }
            else {
                auto leaf = pred->value.pred(key, strict);
                return { this, pred, leaf };
            }
        }
        else {
            auto succ = _trie.leftmost();
            auto leaf = succ != nullptr ? succ->value.pred(key, strict) : nullptr;
            return { this, succ, leaf };
        }
    }

    Where succ(const Key& key, bool strict = false) const {
        auto succ = _trie.succ(key, strict);
        if (succ != nullptr) {
            auto succ_min = succ->value.leftmost();
            if (_cmp(key, succ_min->key)) {
                auto pred = succ->prv;
                if (pred != nullptr && !_cmp(pred->value.rightmost()->key, key)) {
                    auto leaf = pred->value.succ(key, strict);
                    if (leaf != nullptr) {
                        return { this, pred, leaf };
                    }
                    else {
                        return { this, succ, succ_min };
                    }
                }
                else {
                    return { this, succ, succ_min };
                }
            }
            else {
                auto leaf = succ->value.succ(key, strict);
                return { this, succ, leaf };
            }
        }
        else {
            auto pred = _trie.rightmost();
            auto leaf = pred != nullptr ? pred->value.succ(key, strict) : nullptr;
            return { this, pred, leaf };
        }
    }

    /**
     * insert a new leaf
     * @param leaf leaf to insert
     * @return Where with fields: trie -- this; xleaf -- new leaf's x-fast trie node; leaf -- NB: replaced leaf (if any)
     */
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
            xleaf = std::allocator_traits<Alloc>::allocate(_alloc, 1);
            std::allocator_traits<Alloc>::construct(_alloc, xleaf, leaf->key, Value(_cmp));
            _trie.insert(xleaf);
        }

        auto replaced = xleaf->value.insert(leaf);

        if (replaced == nullptr) {
            if (xleaf->value.size() > TREE_SPLIT_THRESHOLD) {
                _trie.remove(xleaf);
                auto split_result = xleaf->value.split();
                XFastLeaf* left = std::allocator_traits<Alloc>::allocate(_alloc, 1);
                std::allocator_traits<Alloc>::construct(_alloc, left, split_result.left.root()->key, std::move(split_result.left));
                XFastLeaf* right = std::allocator_traits<Alloc>::allocate(_alloc, 1);
                std::allocator_traits<Alloc>::construct(_alloc, right, split_result.right.root()->key, std::move(split_result.right));
                _trie.insert(left);
                _trie.insert(right);
                std::allocator_traits<Alloc>::destroy(_alloc, xleaf);
                std::allocator_traits<Alloc>::deallocate(_alloc, xleaf, 1);
                xleaf = _cmp(split_result.left_max->key, leaf->key) ? right : left;
                ++_rebuilds;
            }
            ++_size;
        }

        return { this, xleaf, replaced };
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
                    XFastLeaf* left = std::allocator_traits<Alloc>::allocate(_alloc, 1);
                    std::allocator_traits<Alloc>::construct(_alloc, left, split_result.left.root()->key, std::move(split_result.left));
                    XFastLeaf* right = std::allocator_traits<Alloc>::allocate(_alloc, 1);
                    std::allocator_traits<Alloc>::construct(_alloc, right, split_result.right.root()->key, std::move(split_result.right));
                    _trie.insert(left);
                    _trie.insert(right);
                }
                else {
                    XFastLeaf* merged_xleaf = std::allocator_traits<Alloc>::allocate(_alloc, 1);
                    std::allocator_traits<Alloc>::construct(_alloc, merged_xleaf, merged.root()->key, std::move(merged));
                    _trie.insert(merged_xleaf);
                }
                std::allocator_traits<Alloc>::destroy(_alloc, xleaf);
                std::allocator_traits<Alloc>::deallocate(_alloc, xleaf, 1);
                std::allocator_traits<Alloc>::destroy(_alloc, neighbor);
                std::allocator_traits<Alloc>::deallocate(_alloc, neighbor, 1);
                ++_rebuilds;
            }
            else if (xleaf->value.size() == 0) {
                _trie.remove(xleaf);
                std::allocator_traits<Alloc>::destroy(_alloc, xleaf);
                std::allocator_traits<Alloc>::deallocate(_alloc, xleaf, 1);
                ++_rebuilds;
            }
        }
        else {
            if (_cmp(xleaf->key, xleaf->value.leftmost()->key) || _cmp(xleaf->value.rightmost()->key, xleaf->key)) {
                _trie.remove(xleaf);
                XFastLeaf* reinserted_xleaf = std::allocator_traits<Alloc>::allocate(_alloc, 1);
                std::allocator_traits<Alloc>::construct(_alloc, reinserted_xleaf, xleaf->value.root()->key, std::move(xleaf->value));
                _trie.insert(reinserted_xleaf);
                std::allocator_traits<Alloc>::destroy(_alloc, xleaf);
                std::allocator_traits<Alloc>::deallocate(_alloc, xleaf, 1);
            }
        }

        --_size;
    }

    void clear() {
        for (auto xleaf = _trie.leftmost(); xleaf != nullptr; xleaf = xleaf->nxt) {
            std::allocator_traits<Alloc>::destroy(_alloc, xleaf);
            std::allocator_traits<Alloc>::deallocate(_alloc, xleaf, 1);
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
