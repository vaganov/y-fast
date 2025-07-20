#ifndef _YFAST_FASTMAP_H
#define _YFAST_FASTMAP_H

#include <functional>
#include <stdexcept>
#include <unordered_map>

#include <yfast/impl/yfast.h>
#include <yfast/internal/concepts.h>
#include <yfast/internal/fastmap.h>
#include <yfast/impl/bit_extractor.h>

#define DEBUG

namespace yfast {

template <typename Key, typename Value, unsigned int H, internal::BitExtractorGeneric<Key> BitExtractor = impl::BitExtractor<Key>, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash = std::unordered_map<typename BitExtractor::ShiftResult, std::uintptr_t>, typename Compare = std::less<Key>>
class fastmap {
    static_assert(H >= 8, "Key length too short");

public:
    typedef Key key_type;
    typedef Value value_type;

    static constexpr unsigned int KeyLength = H;

private:
    typedef internal::YFastLeaf<Key, Value> YFastLeaf;

    typedef impl::YFastTrie<YFastLeaf, KeyLength, BitExtractor, Compare, Hash> YFastTrie;

public:
    class iterator: private YFastTrie::Where {
        friend class fastmap;

        using YFastTrie::Where::leaf;
        using YFastTrie::Where::xleaf;
        using YFastTrie::Where::trie;
        unsigned int _last_rebuild = 0;

        explicit iterator(const YFastTrie* trie): YFastTrie::Where(trie), _last_rebuild(trie->rebuilds()) {}
        explicit iterator(const typename YFastTrie::Where& where): YFastTrie::Where(where), _last_rebuild(trie->rebuilds()) {}

    public:
        iterator() = default;
        iterator(const iterator& other) = default;

        bool operator != (const iterator& other) const {
            return leaf != other.leaf;
        }

        typename YFastLeaf::DerefType& operator * () {
            if (leaf != nullptr) {
                return leaf->deref();
            }
            throw std::out_of_range("fastmap::iterator");
        }

        typename YFastLeaf::DerefType* operator -> () {
            if (leaf != nullptr) {
                return &leaf->deref();
            }
            throw std::out_of_range("fastmap::iterator");
        }

        const Key& key() const {
            if (leaf != nullptr) {
                return leaf->key;
            }
            throw std::out_of_range("fastmap::iterator");
        }

        iterator& operator ++ () {
            auto succ = YFastTrie::Value::succ(leaf);
            if (succ == nullptr) {
                if (trie->rebuilds() > _last_rebuild) {
                    auto where = trie->find(leaf->key);
                    xleaf = where.xleaf;
                    _last_rebuild = trie->rebuilds();
                }
                xleaf = xleaf->nxt;
                if (xleaf != nullptr) {
                    leaf = xleaf->value.leftmost();
                }
                else {
                    leaf = nullptr;
                }
            }
            else {
                leaf = succ;
            }
            return  *this;
        }
    };

private:
    YFastTrie _trie;

    const iterator _null_iterator = iterator(&_trie);

public:
    explicit fastmap(BitExtractor bx = BitExtractor(), Compare cmp = Compare()): _trie(bx, cmp) {}

    [[nodiscard]] std::size_t size() const { return _trie.size(); }

    iterator begin() const {
        return iterator(_trie.leftmost());
    }

    iterator end() const {
        return _null_iterator;
    }

    iterator find(const Key& key) const;

    template <typename ... Args>
    iterator insert(const Key& key, Args ... args);
    void erase(iterator& i);
};

template <typename Key, typename Value, unsigned int H, internal::BitExtractorGeneric<Key> BitExtractor, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash, typename Compare>
typename fastmap<Key, Value, H, BitExtractor, Hash, Compare>::iterator fastmap<Key, Value, H, BitExtractor, Hash, Compare>::find(const Key& key) const {
    auto where = _trie.find(key);
    return iterator(where);
}

template <typename Key, typename Value, unsigned int H, internal::BitExtractorGeneric<Key> BitExtractor, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash, typename Compare>
template <typename ... Args>
typename fastmap<Key, Value, H, BitExtractor, Hash, Compare>::iterator fastmap<Key, Value, H, BitExtractor, Hash, Compare>::insert(const Key& key, Args ... args) {
    // _trie.insert(new YFastLeaf(key, std::forward<Args>(args) ...));
    auto leaf = new YFastLeaf(key, args ...);
    auto where = _trie.insert(leaf);
    if (where.leaf != leaf) {
        delete where.leaf;
        where.leaf = leaf;
    }
    return iterator(where);
}

template <typename Key, typename Value, unsigned int H, internal::BitExtractorGeneric<Key> BitExtractor, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash, typename Compare>
void fastmap<Key, Value, H, BitExtractor, Hash, Compare>::erase(iterator& i) {
    _trie.remove(i.leaf, i.xleaf);
    delete i.leaf;
}

}

#endif
