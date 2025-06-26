#ifndef _XFAST_H
#define _XFAST_H

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace std {

template <>
struct hash<vector<byte>> {
    size_t operator()(const vector<byte>& array) const noexcept {
        return hash<string_view>{}({reinterpret_cast<const char*>(array.data()), array.size()});
    }
};

}

namespace yfast {

// xfast_leaf

template <typename _Key, typename _Value>
struct xfast_leaf {
    xfast_leaf* prv;
    xfast_leaf* nxt;
    _Key key;
    _Value value;

    xfast_leaf(_Key key, const _Value& value);
};

// xfast_node

template <typename _Key, typename _Value>
struct xfast_node {
    xfast_node* left;
    xfast_node* right;
    xfast_leaf<_Key, _Value>* descendant;

    xfast_node();
};

// xfast

template <typename _Key, typename _Value, typename _Hash>
class xfast {
  private:
    // private types
    typedef xfast_leaf<_Key, _Value> _Leaf;

  public:
    // public typess
    typedef _Key Key;
    typedef _Value Value;
    typedef _Leaf Leaf;

  private:
    // private types
    typedef xfast_node<_Key, _Value> _Node;

    // private static fields
    static const int H = 8 * sizeof(_Key);

    // fields
    _Node* m_root;
    _Hash m_hash[H];

    // private methods
    _Leaf* approx(_Key key) const;

  public:
    // ctor
    xfast();

    // public methods
    void insert(_Key key, const _Value& value);
    Leaf* find(_Key key) const;
    Leaf* pred(_Key key) const;
    Leaf* succ(_Key key) const;

};

// xfast_leaf

template <typename _Key, typename _Value>
xfast_leaf<_Key, _Value>::xfast_leaf(_Key _key, const _Value& _value)
    : prv(nullptr), nxt(nullptr), key(_key), value(_value) {
}

// xfast_node

template <typename _Key, typename _Value>
xfast_node<_Key, _Value>::xfast_node()
    : left(nullptr), right(nullptr), descendant(nullptr) {
}

// xfast

// private methods

template <typename _Key, typename _Value, typename _Hash>
typename xfast<_Key, _Value, _Hash>::_Leaf*
xfast<_Key, _Value, _Hash>::approx(_Key key) const {
    if (nullptr == m_root) {
        return nullptr;
    }

    int l = 0;
    int r = H;
    int m;
    while (r - l > 1) { // ln H
        m = (r + l) / 2;
        if (m_hash[m].count(key >> m) > 0) {
            r = m;
        }
        else {
            l = m;
        }
    }

    if (m_hash[l].count(key >> l) > 0) {
        m = l;
    }
    else {
        m = r;
    }

    if (H == m) {
        _Leaf* leaf = m_root->descendant;
        return leaf;
    }
    if (0 == m) {
        typename _Hash::const_iterator i = m_hash[0].find(key);
        void* p = i->second;
        _Leaf* leaf = static_cast<_Leaf*>(p);
        return leaf;
    }
    typename _Hash::const_iterator i = m_hash[m].find(key >> m);
    void* p = i->second;
    _Node* node = static_cast<_Node*>(p);
    _Leaf* leaf = node->descendant;
    return leaf;
}

// ctor

template <typename _Key, typename _Value, typename _Hash>
xfast<_Key, _Value, _Hash>::xfast() : m_root(nullptr) {
}

// public methods

template <typename _Key, typename _Value, typename _Hash>
void xfast<_Key, _Value, _Hash>::insert(_Key key, const _Value& value) {
    _Leaf* prv = nullptr;
    _Leaf* nxt = nullptr;
    _Leaf* guess = approx(key);
    if (nullptr != guess) {
        const _Key guess_key = guess->key;
        if (guess_key == key) {
            return;
        }
        if (guess_key < key) {
            prv = guess;
            nxt = guess->nxt;
        }
        else {
            prv = guess->prv;
            nxt = guess;
        }
    }

    _Leaf* leaf = new _Leaf(key, value);
    if (nullptr != prv) {
        prv->nxt = leaf;
    }
    leaf->prv = prv;
    leaf->nxt = nxt;
    if (nullptr != nxt) {
        nxt->prv = leaf;
    }

    bool node_existed;

    if (nullptr != m_root) {
        node_existed = true;
    }
    else {
        m_root = new _Node();
        m_root->descendant = leaf;
        node_existed = false;
    }

    _Node* node = m_root;
    for (int h = H - 1; h > 0; --h) {
        if (key & (1 << h)) {
            if (nullptr != node->right) {
                if (nullptr != node->descendant) {
                    if (key < node->descendant->key) {
                        node->descendant = leaf;
                    }
                }
                node_existed = true;
            }
            else {
                _Node* new_node = new _Node();
                new_node->descendant = leaf;
                node->right = new_node;
                if (node_existed) {
                    node->descendant = nullptr;
                }
                void* p = new_node;
                m_hash[h].insert(std::make_pair(key >> h, p));
                node_existed = false;
            }
            node = node->right;
        }
        else {
            if (nullptr != node->left) {
                if (nullptr != node->descendant) {
                    if (key > node->descendant->key) {
                        node->descendant = leaf;
                    }
                }
                node_existed = true;
            }
            else {
                _Node* new_node = new _Node();
                new_node->descendant = leaf;
                node->left = new_node;
                if (node_existed) {
                    node->descendant = nullptr;
                }
                void* p = new_node;
                m_hash[h].insert(std::make_pair(key >> h, p));
                node_existed = false;
            }
            node = node->left;
        }
    }

    if (key & 1) {
        node->right = reinterpret_cast<_Node*>(leaf);
        if (node_existed) {
            node->descendant = nullptr;
        }
    }
    else {
        node->left = reinterpret_cast<_Node*>(leaf);
        if (node_existed) {
            node->descendant = nullptr;
        }
    }

    void* p = leaf;
    m_hash[0].insert(std::make_pair(key, p));
}

template <typename _Key, typename _Value, typename _Hash>
typename xfast<_Key, _Value, _Hash>::Leaf*
xfast<_Key, _Value, _Hash>::find(_Key key) const {
    typename _Hash::const_iterator i = m_hash[0].find(key);
    if (i != m_hash[0].end()) {
        void* p = i->second;
        _Leaf* leaf = static_cast<_Leaf*>(p);
        return leaf;
    }
    else {
        return nullptr;
    }
}

template <typename _Key, typename _Value, typename _Hash>
typename xfast<_Key, _Value, _Hash>::Leaf*
xfast<_Key, _Value, _Hash>::pred(_Key key) const {
    _Leaf* guess = approx(key);
    if (nullptr == guess) {
        return nullptr;
    }
    if (guess->key > key) {
        return guess->prv;
    }
    else {
        return guess;
    }
}

template <typename _Key, typename _Value, typename _Hash>
typename xfast<_Key, _Value, _Hash>::Leaf*
xfast<_Key, _Value, _Hash>::succ(_Key key) const {
    _Leaf* guess = approx(key);
    if (nullptr == guess) {
        return nullptr;
    }
    if (guess->key < key) {
        return guess->nxt;
    }
    else {
        return guess;
    }
}

template <typename Leaf>
concept LeafGeneric = requires (Leaf leaf) {
    { leaf.key } -> std::convertible_to<typename Leaf::Key>;
    { leaf.prv } -> std::convertible_to<Leaf*>;
    { leaf.nxt } -> std::convertible_to<Leaf*>;
};

template <typename Map, typename Key, typename Value>
concept MapGeneric = requires (Map map, Key key) {
    { map[key] } -> std::convertible_to<Value>;  // TODO: Value&
    { map.at(key) } -> std::convertible_to<Value>;  // TODO: Value&
    { map.contains(key) } -> std::convertible_to<bool>;
    { map.erase(key) };
};

template <typename BitExtractor, typename Key>
concept BitExtractorGeneric = requires (BitExtractor bx, Key key, unsigned int n) {
    { bx.extract_bit(key, n) } -> std::convertible_to<bool>;
    { bx.shift(key, n) } -> std::convertible_to<Key>;
};

template <typename Key>
class BitExtractor;

template <>
class BitExtractor<std::uint64_t> {
public:
    static bool extract_bit(std::uint64_t key, unsigned int n) { return key & (1 << n); }
    static std::uint64_t shift(std::uint64_t key, unsigned int n) { return key >> n; }
};

template <>
class BitExtractor<std::vector<std::byte>> {
public:
    static bool extract_bit(const std::vector<std::byte>& key, unsigned int n);
    static std::vector<std::byte> shift(const std::vector<std::byte>& key, unsigned int n);
};

// TODO: BitExtractor<std::string>?

inline bool BitExtractor<std::vector<std::byte>>::extract_bit(const std::vector<std::byte>& key, unsigned int n) {
    const auto s = n / 8;
    if (key.size() <= s) {
        return false;
    }
    const auto r = n % 8;
    const auto mask = std::byte{1} << r;
    // return (key[s] & mask) != std::byte{0};
    return (key[key.size() - s - 1] & mask) != std::byte{0};
}

// [l | r] [l | r] [l | r]
// [l] [r | l] [r | l]
inline std::vector<std::byte> BitExtractor<std::vector<std::byte>>::shift(const std::vector<std::byte>& key, unsigned int n) {
    const auto s = n / 8;
    if (key.size() <= s) {
        return {};
    }

    const auto shifted_size = key.size() - s;
    std::vector<std::byte> shifted(shifted_size);
    const auto r = n % 8;
    if (r == 0) {
        std::ranges::copy(key.begin() + s, key.end(), shifted.begin());
        return shifted;
    }

    const auto l = 8 - r;
    // shifted[0] = key[s] >> r;
    // for (int i = 1; i < shifted_size; ++i) {
    //     shifted[i] = (key[i + s - 1] << l) | (key[i + s] >> r);
    // }
    std::transform(key.begin() + s, key.end(), shifted.begin(), [r] (std::byte b) { return b >> r; });
    for (int i = 1; i < shifted_size; ++i) {
        shifted[i] |= key[i + s - 1] << l;
    }
    return shifted;
}

template <unsigned int H, LeafGeneric _Leaf, MapGeneric<typename _Leaf::Key, void*> _Hash = std::unordered_map<typename _Leaf::Key, void*>, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor = BitExtractor<typename _Leaf::Key>, typename _Eq = std::equal_to<typename _Leaf::Key>, typename _Compare = std::less<typename _Leaf::Key>>
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

template <unsigned int H, LeafGeneric _Leaf, MapGeneric<typename _Leaf::Key, void*> _Hash, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor, typename _Eq, typename _Compare>
typename xfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::Leaf* xfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::find(const Key& key) const {
    if (_hash[0].contains(key)) {
        return static_cast<Leaf*>(_hash[0].at(key));
    }
    else {
        return nullptr;
    }
}

template <unsigned int H, LeafGeneric _Leaf, MapGeneric<typename _Leaf::Key, void*> _Hash, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor, typename _Eq, typename _Compare>
typename xfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::Leaf* xfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::pred(const Key& key) const {
    auto guess = approx(key);
    if (guess == nullptr) {
        return nullptr;
    }
    return _cmp(key, guess->key) ? guess->prv : guess;
}

template <unsigned int H, LeafGeneric _Leaf, MapGeneric<typename _Leaf::Key, void*> _Hash, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor, typename _Eq, typename _Compare>
typename xfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::Leaf* xfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::succ(const Key& key) const {
    auto guess = approx(key);
    if (guess == nullptr) {
        return nullptr;
    }
    return _cmp(guess->key, key) ? guess->nxt : guess;
}

template <unsigned int H, LeafGeneric _Leaf, MapGeneric<typename _Leaf::Key, void*> _Hash, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor, typename _Eq, typename _Compare>
typename xfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::Leaf* xfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::insert(Leaf* leaf) {
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

    _hash[0][leaf->key] = static_cast<void*>(leaf);

    return leaf;
}

template <unsigned int H, LeafGeneric _Leaf, MapGeneric<typename _Leaf::Key, void*> _Hash, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor, typename _Eq, typename _Compare>
typename xfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::Leaf* xfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::remove(Leaf* leaf) {
    auto prv = leaf->prv;
    auto nxt = leaf->nxt;

    if (prv != nullptr) {
        prv->nxt = nxt;
    }
    if (nxt != nullptr) {
        nxt->prv = prv;
    }
    _hash[0].erase(leaf->key);

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

template <unsigned int H, LeafGeneric _Leaf, MapGeneric<typename _Leaf::Key, void*> _Hash, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor, typename _Eq, typename _Compare>
typename xfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::Leaf* xfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::approx(const Key& key) const {
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
        void* p = _hash[0].at(key);
        return static_cast<Leaf*>(p);
    }
    if (m == H) {
        return _root->descendant;
    }
    void* p = _hash[m].at(_bx.shift(key, m));
    Node* node = static_cast<Node*>(p);
    return node->descendant;
}

template <unsigned int H, LeafGeneric _Leaf, MapGeneric<typename _Leaf::Key, void*> _Hash, BitExtractorGeneric<typename _Leaf::Key> _BitExtractor, typename _Eq, typename _Compare>
void xfast_trie<H, _Leaf, _Hash, _BitExtractor, _Eq, _Compare>::destroy(Node* node) {
    if (node != nullptr) {
        destroy(node->left);
        destroy(node->right);
        delete node;
    }
}

} // namespace yfast

#endif
