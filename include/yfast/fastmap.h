#ifndef _YFAST_FASTMAP_H
#define _YFAST_FASTMAP_H

#include <functional>
#include <stdexcept>

#include <yfast/impl/yfast.h>
#include <yfast/internal/concepts.h>
#include <yfast/internal/default_hash.h>
#include <yfast/internal/fastmap.h>
#include <yfast/impl/bit_extractor.h>
#include <yfast/utils/maybe_const.h>

namespace yfast {

template <typename Key, typename Value, unsigned int H, internal::BitExtractorGeneric<Key> BitExtractor = impl::BitExtractor<Key>, internal::MapGeneric<typename BitExtractor::ShiftResult, std::uintptr_t> Hash = internal::DefaultHash<typename BitExtractor::ShiftResult, std::uintptr_t>, typename Compare = std::less<Key>>
class fastmap {
    static_assert(H >= 8, "Key length too short");

public:
    typedef Key key_type;
    typedef Value value_type;

    static constexpr unsigned int KeyLength = H;

private:
    typedef internal::YFastLeaf<Key, Value> YFastLeaf;

    typedef impl::YFastTrie<YFastLeaf, H, BitExtractor, Compare, Hash> YFastTrie;

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
    YFastTrie _trie;

public:
    explicit fastmap(BitExtractor bx = BitExtractor(), Compare cmp = Compare()): _trie(bx, cmp) {}

    [[nodiscard]] std::size_t size() const { return _trie.size(); }

    iterator begin() {
        return iterator(_trie.leftmost());
    }

    iterator end() {
        return iterator({ &_trie, nullptr, nullptr });
    }

    const_iterator begin() const {
        return const_iterator(_trie.leftmost());
    }

    const_iterator end() const {
        return const_iterator({ &_trie, nullptr, nullptr });
    }

    const_iterator cbegin() const {
        return const_iterator(_trie.leftmost());
    }

    const_iterator cend() const {
        return const_iterator({ &_trie, nullptr, nullptr });
    }

    reverse_iterator rbegin() {
        return reverse_iterator(_trie.rightmost());
    }

    reverse_iterator rend() {
        return reverse_iterator({ &_trie, nullptr, nullptr });
    }

    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(_trie.rightmost());
    }

    const_reverse_iterator rend() const {
        return const_reverse_iterator({ &_trie, nullptr, nullptr });
    }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(_trie.rightmost());
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator({ &_trie, nullptr, nullptr });
    }

    iterator find(const Key& key) {
        auto where = _trie.find(key);
        return iterator(where);
    }

    const_iterator find(const Key& key) const {
        auto where = _trie.find(key);
        return const_iterator(where);
    }

    typename YFastLeaf::DerefType& operator [] (const Key& key) {
        auto i = find(key);
        if (i == end()) {
            auto leaf = new YFastLeaf(key);
            auto where = _trie.insert(leaf);
            i = iterator(where);
        }
        return *i;
    }

    const typename YFastLeaf::DerefType& at(const Key& key) const {
        auto i = find(key);
        if (i == end()) {
            throw std::out_of_range("yfast::fastmap::at");
        }
        return *i;
    }

    template <typename ... Args>
    iterator insert(const Key& key, Args ... args) {
        auto leaf = new YFastLeaf(key, args ...);
        auto where = _trie.insert(leaf);
        if (where.leaf != leaf) {
            delete where.leaf;
            where.leaf = leaf;
        }
        return iterator(where);
    }

    template <typename Iterator>
    Iterator erase(const Iterator& i) {
        if (i.leaf == nullptr) {
            return i;
        }
        auto j = i;
        ++j;
        _trie.remove(i.leaf, i.xleaf);
        delete i.leaf;
        return j;
    }

    void clear() {
        _trie.clear();
    }
};

}

#endif
