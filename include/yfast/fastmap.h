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

        unsigned int _last_rebuild = 0;

        using YFastTrie::Where::leaf;
        using YFastTrie::Where::xleaf;
        using YFastTrie::Where::trie;

    public:
        typedef const typename YFastLeaf::Key key_type;
        typedef typename utils::MaybeConst<typename YFastLeaf::DerefType, Const>::Type value_type;

    protected:
        IteratorBase() = default;
        explicit IteratorBase(const typename YFastTrie::Where& where): YFastTrie::Where(where), _last_rebuild(trie->rebuilds()) {}

        void inc() {
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

        void dec() {
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
        template <bool Const_>
        bool operator == (const IteratorBase<Const_>& other) const {
            return leaf == other.leaf;
        }

        template <bool Const_>
        bool operator != (const IteratorBase<Const_>& other) const {
            return leaf != other.leaf;
        }

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
    protected:
        ForwardIteratorBase() = default;
        explicit ForwardIteratorBase(const typename YFastTrie::Where& where): IteratorBase<Const>(where) {}
    public:
        ForwardIteratorBase& operator ++ () {
            this->inc();
            return *this;
        }

        ForwardIteratorBase operator ++ (int) {
            auto i = *this;
            this->inc();
            return i;
        }

        ForwardIteratorBase& operator -- () {
            this->dec();
            return *this;
        }

        ForwardIteratorBase operator -- (int) {
            auto i = *this;
            this->dec();
            return i;
        }
    };

    template <bool Const>
    class ReverseIteratorBase: public IteratorBase<Const> {
    protected:
        ReverseIteratorBase() = default;
        explicit ReverseIteratorBase(const typename YFastTrie::Where& where): IteratorBase<Const>(where) {}
    public:
        ReverseIteratorBase& operator ++ () {
            this->dec();
            return *this;
        }

        ReverseIteratorBase operator ++ (int) {
            auto i = *this;
            this->dec();
            return i;
        }

        ReverseIteratorBase& operator -- () {
            this->inc();
            return *this;
        }

        ReverseIteratorBase operator -- (int) {
            auto i = *this;
            this->inc();
            return i;
        }
    };

public:
    // class iterator;
    // class const_iterator;
    // class reverse_iterator;
    // class const_reverse_iterator;

    class iterator: public ForwardIteratorBase<false> {
        friend class fastmap;

        explicit iterator(const typename YFastTrie::Where& where): ForwardIteratorBase<false>(where) {}

    public:
        iterator() = default;
        iterator(const iterator& other) = default;
        // iterator(const reverse_iterator& other): ForwardIteratorBase<false>(other) {}
    };

    class const_iterator: public ForwardIteratorBase<true> {
        friend class fastmap;

        explicit const_iterator(const typename YFastTrie::Where& where): ForwardIteratorBase<true>(where) {}

    public:
        const_iterator() = default;
        const_iterator(const const_iterator& other) = default;
        const_iterator(const iterator& other): ForwardIteratorBase<true>(other) {}
        // const_iterator(const reverse_iterator& other): ForwardIteratorBase<true>(other) {}
        // const_iterator(const const_reverse_iterator& other): ForwardIteratorBase<true>(other) {}
    };

    class reverse_iterator: public ReverseIteratorBase<false> {
        friend class fastmap;

        explicit reverse_iterator(const typename YFastTrie::Where& where): ReverseIteratorBase<false>(where) {}

    public:
        reverse_iterator() = default;
        reverse_iterator(const reverse_iterator& other) = default;
        // reverse_iterator(const iterator& other): ReverseIteratorBase<false>(other) {}
    };

    class const_reverse_iterator: public ReverseIteratorBase<true> {
        friend class fastmap;

        explicit const_reverse_iterator(const typename YFastTrie::Where& where): ReverseIteratorBase<true>(where) {}

    public:
        const_reverse_iterator() = default;
        const_reverse_iterator(const const_reverse_iterator& other) = default;
        // const_reverse_iterator(const iterator& other): ReverseIteratorBase<true>(other) {}
        const_reverse_iterator(const reverse_iterator& other): ReverseIteratorBase<true>(other) {}
        // const_reverse_iterator(const const_iterator& other): ReverseIteratorBase<true>(other) {}
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
        return iterator();
    }

    const_iterator begin() const {
        return const_iterator(_trie.leftmost());
    }

    const_iterator end() const {
        return const_iterator();
    }

    const_iterator cbegin() const {
        return const_iterator(_trie.leftmost());
    }

    const_iterator cend() const {
        return const_iterator();
    }

    reverse_iterator rbegin() {
        return reverse_iterator(_trie.rightmost());
    }

    reverse_iterator rend() {
        return reverse_iterator();
    }

    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(_trie.rightmost());
    }

    const_reverse_iterator rend() const {
        return const_reverse_iterator();
    }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(_trie.rightmost());
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator();
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
        if (i == end()) {
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
