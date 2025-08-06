#ifndef _YFAST_FASTMAP_H
#define _YFAST_FASTMAP_H

#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <utility>

#include <yfast/impl/yfast.h>
#include <yfast/internal/concepts.h>
#include <yfast/internal/default_hash.h>
#include <yfast/internal/fastmap.h>
#include <yfast/impl/bit_extractor.h>
#include <yfast/utils/maybe_const.h>

namespace yfast {

template <
    typename Key,
    typename Value,
    unsigned int H,
    internal::BitExtractorGeneric<Key> BitExtractor = impl::BitExtractor<Key>,
    internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash = internal::DefaultHash<typename BitExtractor::ShiftResult, std::uintptr_t>,
    typename Compare = std::less<Key>,
    typename ArbitraryAllocator = std::allocator<Key>
>
class fastmap {
public:
    typedef Key key_type;
    typedef Value value_type;

    static constexpr unsigned int KeyLength = H;

private:
    typedef internal::YFastLeaf<Key, Value> YFastLeaf;

    typedef typename std::allocator_traits<ArbitraryAllocator>::template rebind_alloc<YFastLeaf> Alloc;

    typedef impl::YFastTrie<YFastLeaf, H, BitExtractor, Hash, Compare, ArbitraryAllocator> YFastTrie;

    template <bool Const>
    class IteratorBase: private YFastTrie::Where {
        friend class fastmap;

        using YFastTrie::Where::leaf;
        using YFastTrie::Where::xleaf;
        using YFastTrie::Where::trie;

        unsigned int _last_rebuild = 0;

    public:
        typedef const typename YFastLeaf::Key key_type;
        typedef typename utils::MaybeConst<typename YFastLeaf::DerefType, Const>::Type value_type;

        typedef std::bidirectional_iterator_tag iterator_category;  // TODO: random access
        typedef std::ptrdiff_t difference_type;
        typedef value_type* pointer;
        typedef value_type& reference;

    protected:
        IteratorBase() = default;
        explicit IteratorBase(const typename YFastTrie::Where& where): YFastTrie::Where(where), _last_rebuild(trie->rebuilds()) {}

        template <bool Const_>
        bool same_as(const IteratorBase<Const_>& other) const {
            return trie == other.trie && leaf == other.leaf;
        }

        void forward() {
            auto succ = YFastTrie::Value::succ(leaf);
            if (succ != nullptr) {
                leaf = succ;
            }
            else {
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
        }

        void backward() {
            auto pred = YFastTrie::Value::pred(leaf);
            if (pred != nullptr) {
                leaf = pred;
            }
            else {
                if (trie->rebuilds() > _last_rebuild) {
                    auto where = trie->find(leaf->key);
                    xleaf = where.xleaf;
                    _last_rebuild = trie->rebuilds();
                }
                xleaf = xleaf->prv;
                if (xleaf != nullptr) {
                    leaf = xleaf->value.rightmost();
                }
                else {
                    leaf = nullptr;
                }
            }
        }

    public:
        key_type& key() const {
            if (leaf == nullptr) {
                throw std::out_of_range("yfast::fastmap::iterator");
            }
            return leaf->key;
        }

        value_type& value() const {
            if (leaf == nullptr) {
                throw std::out_of_range("yfast::fastmap::iterator");
            }
            return leaf->deref();
        }

        value_type& operator * () const {
            if (leaf == nullptr) {
                throw std::out_of_range("yfast::fastmap::iterator");
            }
            return leaf->deref();
        }

        value_type* operator -> () const {
            if (leaf == nullptr) {
                throw std::out_of_range("yfast::fastmap::iterator");
            }
            return &leaf->deref();
        }
    };

    template <bool Const>
    class ForwardIteratorBase: public IteratorBase<Const> {
        friend class fastmap;

        using IteratorBase<Const>::leaf;
        using IteratorBase<Const>::trie;
        using IteratorBase<Const>::same_as;
        using IteratorBase<Const>::forward;
        using IteratorBase<Const>::backward;

    protected:
        ForwardIteratorBase() = default;
        explicit ForwardIteratorBase(const typename YFastTrie::Where& where): IteratorBase<Const>(where) {}

        void inc() {
            if (leaf != nullptr) {
                forward();
            }
        }

        void dec() {
            if (leaf != nullptr) {
                backward();
            }
            else {
                *this = ForwardIteratorBase(trie->rightmost());
            }
        }

    public:
        template <bool Const_>
        bool operator == (const ForwardIteratorBase<Const_>& other) const {
            return same_as(other);
        }

        template <bool Const_>
        bool operator != (const ForwardIteratorBase<Const_>& other) const {
            return !same_as(other);
        }
    };

    template <bool Const>
    class ReverseIteratorBase: public IteratorBase<Const> {
        friend class fastmap;

        using IteratorBase<Const>::leaf;
        using IteratorBase<Const>::trie;
        using IteratorBase<Const>::same_as;
        using IteratorBase<Const>::forward;
        using IteratorBase<Const>::backward;

    protected:
        ReverseIteratorBase() = default;
        explicit ReverseIteratorBase(const typename YFastTrie::Where& where): IteratorBase<Const>(where) {}

        void inc() {
            if (leaf != nullptr) {
                backward();
            }
        }

        void dec() {
            if (leaf != nullptr) {
                forward();
            }
            else {
                *this = ReverseIteratorBase(trie->leftmost());
            }
        }

    public:
        template <bool Const_>
        bool operator == (const ReverseIteratorBase<Const_>& other) const {
            return same_as(other);
        }

        template <bool Const_>
        bool operator != (const ReverseIteratorBase<Const_>& other) const {
            return !same_as(other);
        }
    };

public:
    class reverse_iterator;
    class const_reverse_iterator;

    class iterator: public ForwardIteratorBase<false> {
        friend class fastmap;

        using ForwardIteratorBase<false>::inc;
        using ForwardIteratorBase<false>::dec;

        explicit iterator(const typename YFastTrie::Where& where): ForwardIteratorBase<false>(where) {}

    public:
        iterator() = default;
        iterator(const iterator& other) = default;

        iterator& operator ++ () {
            inc();
            return *this;
        }

        iterator operator ++ (int) {
            auto i = *this;
            inc();
            return i;
        }

        iterator& operator -- () {
            dec();
            return *this;
        }

        iterator operator -- (int) {
            auto i = *this;
            dec();
            return i;
        }

        reverse_iterator make_reverse() const {
            auto i = *this;
            const typename YFastTrie::Where& where = --i;
            return reverse_iterator(where);
        }
    };

    class const_iterator: public ForwardIteratorBase<true> {
        friend class fastmap;

        using ForwardIteratorBase<true>::inc;
        using ForwardIteratorBase<true>::dec;

        explicit const_iterator(const typename YFastTrie::Where& where): ForwardIteratorBase<true>(where) {}

    public:
        const_iterator() = default;
        const_iterator(const const_iterator& other) = default;
        const_iterator(const iterator& other): ForwardIteratorBase<true>(other) {}

        const_iterator& operator ++ () {
            inc();
            return *this;
        }

        const_iterator operator ++ (int) {
            auto i = *this;
            inc();
            return i;
        }

        const_iterator& operator -- () {
            dec();
            return *this;
        }

        const_iterator operator -- (int) {
            auto i = *this;
            dec();
            return i;
        }

        const_reverse_iterator make_reverse() const {
            auto i = *this;
            const typename YFastTrie::Where& where = --i;
            return const_reverse_iterator(where);
        }
    };

    class reverse_iterator: public ReverseIteratorBase<false> {
        friend class fastmap;

        using ReverseIteratorBase<false>::inc;
        using ReverseIteratorBase<false>::dec;

        explicit reverse_iterator(const typename YFastTrie::Where& where): ReverseIteratorBase<false>(where) {}

    public:
        reverse_iterator() = default;
        reverse_iterator(const reverse_iterator& other) = default;

        reverse_iterator& operator ++ () {
            inc();
            return *this;
        }

        reverse_iterator operator ++ (int) {
            auto i = *this;
            inc();
            return i;
        }

        reverse_iterator& operator -- () {
            dec();
            return *this;
        }

        reverse_iterator operator -- (int) {
            auto i = *this;
            dec();
            return i;
        }
    };

    class const_reverse_iterator: public ReverseIteratorBase<true> {
        friend class fastmap;

        using ReverseIteratorBase<true>::inc;
        using ReverseIteratorBase<true>::dec;

        explicit const_reverse_iterator(const typename YFastTrie::Where& where): ReverseIteratorBase<true>(where) {}

    public:
        const_reverse_iterator() = default;
        const_reverse_iterator(const const_reverse_iterator& other) = default;
        const_reverse_iterator(const reverse_iterator& other): ReverseIteratorBase<true>(other) {}

        const_reverse_iterator& operator ++ () {
            inc();
            return *this;
        }

        const_reverse_iterator operator ++ (int) {
            auto i = *this;
            inc();
            return i;
        }

        const_reverse_iterator& operator -- () {
            dec();
            return *this;
        }

        const_reverse_iterator operator -- (int) {
            auto i = *this;
            dec();
            return i;
        }
    };

private:
    Alloc _alloc;
    YFastTrie _trie;

public:
    explicit fastmap(BitExtractor bx = BitExtractor(), Compare cmp = Compare(), ArbitraryAllocator alloc = ArbitraryAllocator()): _alloc(alloc), _trie(bx, cmp, alloc) {}
    fastmap(std::initializer_list<std::pair<Key, Value>> array, BitExtractor bx = BitExtractor(), Compare cmp = Compare(), ArbitraryAllocator alloc = ArbitraryAllocator()): _alloc(alloc), _trie(bx, cmp, alloc) {
        for (auto& pair: array) {
            insert(pair.first, pair.second);
        }
    }
    fastmap(const fastmap& other) = delete;
    fastmap(fastmap&& other) noexcept = default;

    ~fastmap() { clear(); }

    /**
     * @return the number of entries in the container
     */
    [[nodiscard]] std::size_t size() const { return _trie.size(); }

    /**
     * @return \a true if the container is empty otherwise \a false
     */
    [[nodiscard]] bool empty() const { return _trie.size() == 0; }

    iterator begin() {
        return iterator(_trie.leftmost());
    }

    iterator end() {
        return iterator(_trie.nowhere);
    }

    const_iterator begin() const {
        return const_iterator(_trie.leftmost());
    }

    const_iterator end() const {
        return const_iterator(_trie.nowhere);
    }

    const_iterator cbegin() const {
        return const_iterator(_trie.leftmost());
    }

    const_iterator cend() const {
        return const_iterator(_trie.nowhere);
    }

    reverse_iterator rbegin() {
        return reverse_iterator(_trie.rightmost());
    }

    reverse_iterator rend() {
        return reverse_iterator(_trie.nowhere);
    }

    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(_trie.rightmost());
    }

    const_reverse_iterator rend() const {
        return const_reverse_iterator(_trie.nowhere);
    }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(_trie.rightmost());
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator(_trie.nowhere);
    }

    /**
     * find an entry by key
     * @param key key to find
     * @return iterator pointing to the entry with the key equal to \a key if any or \a end() otherwise
     */
    iterator find(const Key& key) {
        auto where = _trie.find(key);
        return iterator(where);
    }

    /**
     * find an entry by key
     * @param key key to find
     * @return const iterator pointing to the entry with the key equal to \a key if any or \a end() otherwise
     */
    const_iterator find(const Key& key) const {
        auto where = _trie.find(key);
        return const_iterator(where);
    }

    /**
     * find a predecessor entry for a key
     * @param key key
     * @param strict whether an entry with the key strictly less than \a key should be returned
     * @return iterator pointing to the entry with the key either not greater or strictly less than \a key
     */
    iterator pred(const Key& key, bool strict = false) {
        auto where = _trie.pred(key, strict);
        return iterator(where);
    }

    /**
     * find a predecessor entry for a key
     * @param key key
     * @param strict whether an entry with the key strictly less than \a key should be returned
     * @return const iterator pointing to the entry with the key either not greater or strictly less than \a key
     */
    const_iterator pred(const Key& key, bool strict = false) const {
        auto where = _trie.pred(key, strict);
        return const_iterator(where);
    }

    /**
     * find a successor entry for a key
     * @param key key
     * @param strict whether an entry with the key strictly greater than \a key should be returned
     * @return iterator pointing to the entry with the key either not less or strictly greater than \a key
     */
    iterator succ(const Key& key, bool strict = false) {
        auto where = _trie.succ(key, strict);
        return iterator(where);
    }

    /**
     * find a successor entry for a key
     * @param key key
     * @param strict whether an entry with the key strictly greater than \a key should be returned
     * @return const iterator pointing to the entry with the key either not less or strictly greater than \a key
     */
    const_iterator succ(const Key& key, bool strict = false) const {
        auto where = _trie.succ(key, strict);
        return const_iterator(where);
    }

    iterator lower_bound(const Key& key) {
        return succ(key);
    }

    const_iterator lower_bound(const Key& key) const {
        return succ(key);
    }

    iterator upper_bound(const Key& key) {
        return succ(key, true);
    }

    const_iterator upper_bound(const Key& key) const {
        return succ(key, true);
    }

    /**
     * get value by key
     * @param key key
     * @return reference to the value indexed by \a key; if not present, a default-constructed value is inserted
     */
    typename YFastLeaf::DerefType& operator [] (const Key& key) {
        auto i = find(key);
        if (i == end()) {
            YFastLeaf* leaf = std::allocator_traits<Alloc>::allocate(_alloc, 1);
            std::allocator_traits<Alloc>::construct(_alloc, leaf, key);
            auto where = _trie.insert(leaf);
            where.leaf = leaf;
            i = iterator(where);
        }
        return *i;
    }

    /**
     * get value by key
     * @param key key
     * @return const reference to the value indexed by \a key; if not present, \a std::out_of_range is thrown
     */
    const typename YFastLeaf::DerefType& at(const Key& key) const {
        auto i = find(key);
        if (i == end()) {
            throw std::out_of_range("yfast::fastmap::at");
        }
        return *i;
    }

    /**
     * insert an entry, possibly replacing the existing one
     * @param key key to insert
     * @param args const reference or move-reference to the value to insert
     * @return iterator pointing to the inserted entry
     */
    template <typename ... Args>
    iterator insert(const Key& key, Args ... args) {
        YFastLeaf* leaf = std::allocator_traits<Alloc>::allocate(_alloc, 1);
        std::allocator_traits<Alloc>::construct(_alloc, leaf, key, args ...);
        auto where = _trie.insert(leaf);
        if (where.leaf != nullptr) {
            std::allocator_traits<Alloc>::destroy(_alloc, where.leaf);
            std::allocator_traits<Alloc>::deallocate(_alloc, where.leaf, 1);
        }
        where.leaf = leaf;
        return iterator(where);
    }

    /**
     * erase entry by iterator
     * @param i iterator (may be a const iterator and/or a reverse iterator)
     * @return iterator following the removed entry with respect to the iterator direction
     */
    template <typename Iterator>
    Iterator erase(const Iterator& i) {
        if (i.trie != &_trie) {
            throw std::invalid_argument("yfast::fastmap::erase");
        }
        if (i.leaf == nullptr) {
            return i;
        }
        auto j = i;
        ++j;
        _trie.remove(i.leaf, i.xleaf);
        std::allocator_traits<Alloc>::destroy(_alloc, i.leaf);
        std::allocator_traits<Alloc>::deallocate(_alloc, i.leaf, 1);
        return j;
    }

    /**
     * find and erase entry by key
     * @param key key to find and erase
     * @return \a true if the entry has been found and erased or \a false otherwise
     */
    bool erase(const Key& key) {
        auto i = find(key);
        if (i == end()) {
            return false;
        }
        _trie.remove(i);
        return true;
    }

    /**
     * erase all entries
     */
    void clear() {
        typename YFastTrie::XFastLeaf* xleaf = _trie.leftmost().xleaf;
        while (xleaf != nullptr) {
            destroy_subtree(xleaf->value.root());
            xleaf = xleaf->nxt;
        }
        _trie.clear();
    }

private:
    void destroy_subtree(YFastLeaf* leaf) {
        if (leaf != nullptr) {
            destroy_subtree(leaf->left());
            destroy_subtree(leaf->right());
            std::allocator_traits<Alloc>::destroy(_alloc, leaf);
            std::allocator_traits<Alloc>::deallocate(_alloc, leaf, 1);
        }
    }
};

}

#endif
