#ifndef _YFAST_IMPL_XFAST_H
#define _YFAST_IMPL_XFAST_H

#include <functional>
#include <unordered_map>

#include <yfast/internal/concepts.h>
#include <yfast/impl/bit_extractor.h>

namespace yfast::impl {

using internal::LeafGeneric;
using internal::MapGeneric;
using internal::BitExtractorGeneric;

template <unsigned int H, LeafGeneric _Leaf, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor = BitExtractor<typename _Leaf::Key>, MapGeneric<typename _BitExtractor::ShiftResult, void*> _Hash = std::unordered_map<typename _BitExtractor::ShiftResult, void*>, typename _Eq = std::equal_to<typename _Leaf::Key>, typename _Compare = std::less<typename _Leaf::Key>>
class xfast_trie {
public:
    typedef _Leaf Leaf;
    typedef typename Leaf::Key Key;

protected:
    struct Node {
        Node* left;
        Node* right;
        Leaf* descendant;
    };

private:
    _BitExtractor _bx;
    _Eq _eq;
    _Compare _cmp;
    Node* _root;
    _Hash _hash[H];

public:
    explicit xfast_trie(_BitExtractor bx = _BitExtractor(), _Eq eq = _Eq(), _Compare cmp = _Compare()) : _bx(bx), _eq(eq), _cmp(cmp), _root(nullptr) {}
    ~xfast_trie() { destroy(_root); }  // TODO: _hash[0]

    Leaf* find(const Key& key) const;
    Leaf* pred(const Key& key) const;
    Leaf* succ(const Key& key) const;

    Leaf* insert(Leaf* leaf);
    Leaf* remove(Leaf* leaf);

private:
    [[nodiscard]] Leaf* approx(const Key& key) const;

    static void destroy(Node* node);
};

template <unsigned int H, LeafGeneric _Leaf, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor, MapGeneric<typename _BitExtractor::ShiftResult, void*> _Hash, typename _Eq, typename _Compare>
typename xfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::Leaf* xfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::find(const Key& key) const {
    if (_hash[0].contains(_bx.shift(key, 0))) {
        return static_cast<Leaf*>(_hash[0].at(_bx.shift(key, 0)));
    }
    else {
        return nullptr;
    }
}

template <unsigned int H, LeafGeneric _Leaf, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor, MapGeneric<typename _BitExtractor::ShiftResult, void*> _Hash, typename _Eq, typename _Compare>
typename xfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::Leaf* xfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::pred(const Key& key) const {
    auto guess = approx(key);
    if (guess == nullptr) {
        return nullptr;
    }
    return _cmp(key, guess->key) ? guess->prv : guess;
}

template <unsigned int H, LeafGeneric _Leaf, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor, MapGeneric<typename _BitExtractor::ShiftResult, void*> _Hash, typename _Eq, typename _Compare>
typename xfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::Leaf* xfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::succ(const Key& key) const {
    auto guess = approx(key);
    if (guess == nullptr) {
        return nullptr;
    }
    return _cmp(guess->key, key) ? guess->nxt : guess;
}

template <unsigned int H, LeafGeneric _Leaf, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor, MapGeneric<typename _BitExtractor::ShiftResult, void*> _Hash, typename _Eq, typename _Compare>
typename xfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::Leaf* xfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::insert(Leaf* leaf) {
    Leaf* prv = nullptr;
    Leaf* nxt = nullptr;
    auto guess = approx(leaf->key);
    if (guess != nullptr) {
        if (_eq(guess->key, leaf->key)) {
            return guess;
        }
        if (_cmp(guess->key, leaf->key)) {
            prv = guess;
            nxt = guess->nxt;
        }
        else {
            prv = guess->prv;
            nxt = guess;
        }
    }

    leaf->prv = prv;
    leaf->nxt = nxt;
    if (prv != nullptr) {
        prv->nxt = leaf;
    }
    if (nxt != nullptr) {
        nxt->prv = leaf;
    }

    if (_root != nullptr) {
        // ...
    }
    else {
        _root = new Node(nullptr, nullptr, leaf);
    }

    auto node = _root;
    for (auto h = H - 1; h > 0; --h) {
        if (_bx.extract_bit(leaf->key, h)) {
            if (node->right != nullptr) {
                if (node->descendant != nullptr) {
                    if (_cmp(leaf->key, node->descendant->key)) {
                        node->descendant = leaf;
                    }
                }
            }
            else {
                auto new_node = new Node(nullptr, nullptr, leaf);
                node->right = new_node;
                _hash[h][_bx.shift(leaf->key, h)] = static_cast<void*>(new_node);
            }
            node = node->right;
        }
        else {
            if (node->left != nullptr) {
                if (node->descendant != nullptr) {
                    if (_cmp(node->descendant->key, leaf->key)) {
                        node->descendant = leaf;
                    }
                }
            }
            else {
                auto new_node = new Node(nullptr, nullptr, leaf);
                node->left = new_node;
                _hash[h][_bx.shift(leaf->key, h)] = static_cast<void*>(new_node);
            }
            node = node->left;
        }
    }

    _hash[0][_bx.shift(leaf->key, 0)] = static_cast<void*>(leaf);

    return leaf;
}

template <unsigned int H, LeafGeneric _Leaf, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor, MapGeneric<typename _BitExtractor::ShiftResult, void*> _Hash, typename _Eq, typename _Compare>
typename xfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::Leaf* xfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::remove(Leaf* leaf) {
    auto prv = leaf->prv;
    auto nxt = leaf->nxt;

    if (prv != nullptr) {
        prv->nxt = nxt;
    }
    if (nxt != nullptr) {
        nxt->prv = prv;
    }
    _hash[0].erase(_bx.shift(leaf->key, 0));

    auto subtree_removed = true;
    for (auto h = 1; h < H; ++h) {
        auto key_prefix = _bx.shift(leaf->key, h);
        void* p = _hash[h].at(key_prefix);
        auto node = static_cast<Node*>(p);
        if (_bx.extract_bit(leaf->key, h - 1)) {  // NB
            node->right = nullptr;
            if (node->left != nullptr) {
                subtree_removed = false;
                node->descendant = nxt;
                break;
            }
        }
        else {
            node->left = nullptr;
            if (node->right != nullptr) {
                subtree_removed = false;
                node->descendant = prv;
                break;
            }
        }
        _hash[h].erase(key_prefix);
        delete node;
    }

    if (subtree_removed) {
        if (_bx.extract_bit(leaf->key, H - 1)) {
            _root->right = nullptr;
            if (_root->left != nullptr) {
                _root->descendant = nxt;
            }
            else {
                delete _root;
                _root = nullptr;
            }
        }
        else {
            _root->left = nullptr;
            if (_root->right != nullptr) {
                _root->descendant = prv;
            }
            else {
                delete _root;
                _root = nullptr;
            }
        }
    }

    return leaf;
}

template <unsigned int H, LeafGeneric _Leaf, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor, MapGeneric<typename _BitExtractor::ShiftResult, void*> _Hash, typename _Eq, typename _Compare>
typename xfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::Leaf* xfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::approx(const Key& key) const {
    if (_root == nullptr) {
        return nullptr;
    }

    int l = 0;
    int r = H;
    int m;
    while (r - l > 1) { // ln H
        m = (r + l) / 2;
        if (_hash[m].contains(_bx.shift(key, m))) {
            r = m;
        }
        else {
            l = m;
        }
    }

    if (_hash[l].contains(_bx.shift(key, l))) {
        m = l;
    }
    else {
        m = r;
    }

    if (m == 0) {
        void* p = _hash[0].at(_bx.shift(key, 0));
        return static_cast<Leaf*>(p);
    }
    if (m == H) {
        return _root->descendant;
    }
    void* p = _hash[m].at(_bx.shift(key, m));
    Node* node = static_cast<Node*>(p);
    return node->descendant;
}

template <unsigned int H, LeafGeneric _Leaf, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor, MapGeneric<typename _BitExtractor::ShiftResult, void*> _Hash, typename _Eq, typename _Compare>
void xfast_trie<H, _Leaf, _BitExtractor, _Hash, _Eq, _Compare>::destroy(Node* node) {
    if (node != nullptr) {
        destroy(node->left);
        destroy(node->right);
        delete node;
    }
}

} // namespace yfast

#endif
