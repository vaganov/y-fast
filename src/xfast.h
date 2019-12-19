#ifndef _XFAST_H
#define _XFAST_H

#include <unordered_map>
#include <utility>

namespace yfast {

// forward decl

template <typename Key, typename Value> struct xfast_leaf;
template <typename Key, typename Value> struct xfast_node;
template <typename Key, typename Value, template<typename...> class Hash> class xfast;

// xfast_leaf

template <typename Key, typename Value>
struct xfast_leaf {
    xfast_node<Key, Value>* parent;
    xfast_leaf* prv;
    xfast_leaf* nxt;
    Key key;
    Value value;

    xfast_leaf(Key key, const Value& value);
};

// xfast_node

template <typename Key, typename Value>
struct xfast_node {
    xfast_node* parent;
    xfast_node* left;
    xfast_node* right;
    xfast_leaf<Key, Value>* descendant;

    xfast_node();
};

// xfast

template <typename Key, typename Value, template<typename...> class Hash = std::unordered_map>
class xfast {
  public:
    // public types
    typedef xfast_leaf<Key, Value> Leaf;

  private:
    // private types
    typedef xfast_node<Key, Value> Node;
    typedef Hash<Key, Node*> NodeHash;
    typedef Hash<Key, Leaf*> LeafHash;

    // private static fields
    static const int H = 8 * sizeof(Key);

    // fields
    Node* m_root;
    NodeHash m_nodes[H]; // TODO: m_nodes[0] always empty, shift or reverse index
    LeafHash m_leaves;

    // private methods
    Leaf* approx(Key key) const;

  public:
    // ctor
    xfast();
    // dtor
    ~xfast();

    // public methods
    Leaf* insert(Key key, const Value& value);
    Leaf* find(Key key) const;
    Leaf* pred(Key key) const;
    Leaf* succ(Key key) const;

};

// xfast_leaf

template <typename Key, typename Value>
xfast_leaf<Key, Value>::xfast_leaf(Key _key, const Value& _value)
    : parent(nullptr), prv(nullptr), nxt(nullptr), key(_key), value(_value) {
}

// xfast_node

template <typename Key, typename Value>
xfast_node<Key, Value>::xfast_node()
    : parent(nullptr), left(nullptr), right(nullptr), descendant(nullptr) {
}

// xfast

// private methods

template <typename Key, typename Value, template<typename...> class Hash>
typename xfast<Key, Value, Hash>::Leaf*
xfast<Key, Value, Hash>::approx(Key key) const {
    if (nullptr == m_root) {
        return nullptr;
    }

    Leaf* found = find(key);
    if (nullptr != found) {
        return found;
    }

    int l = 1;
    int r = H;
    int m;
    while (r - l > 1) { // ln H
        m = (r + l) / 2;
        if (m_nodes[m].count(key >> m) > 0) {
            r = m;
        }
        else {
            l = m;
        }
    }

    if (m_nodes[l].count(key >> l) > 0) {
        m = l;
    }
    else {
        m = r;
    }

    if (H == m) {
        Leaf* leaf = m_root->descendant;
        return leaf;
    }
    typename NodeHash::const_iterator i = m_nodes[m].find(key >> m);
    Node* node = i->second;
    Leaf* leaf = node->descendant;
    return leaf;
}

// ctor

template <typename Key, typename Value, template<typename...> class Hash>
xfast<Key, Value, Hash>::xfast() : m_root(nullptr) {
}

// dtor

template <typename Key, typename Value, template<typename...> class Hash>
xfast<Key, Value, Hash>::~xfast() {
    if (nullptr != m_root) {
        delete m_root;
    }
    for (int h = 1; h < H; ++h) {
        for (const auto& pair: m_nodes[h]) {
            Node* node = pair.second;
            delete node;
        }
        m_nodes[h].clear();
    }
    for (const auto& pair: m_leaves) {
        Leaf* leaf = pair.second;
        delete leaf;
    }
    m_leaves.clear();
}

// public methods

template <typename Key, typename Value, template<typename...> class Hash>
typename xfast<Key, Value, Hash>::Leaf*
xfast<Key, Value, Hash>::insert(Key key, const Value& value) {
    Leaf* prv = nullptr;
    Leaf* nxt = nullptr;
    Leaf* guess = approx(key);
    if (nullptr != guess) {
        if (guess->key == key) {
            return guess;
        }
        if (guess->key < key) {
            prv = guess;
            nxt = guess->nxt;
        }
        else {
            prv = guess->prv;
            nxt = guess;
        }
    }

    Leaf* leaf = new Leaf(key, value);
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
        m_root = new Node();
        m_root->descendant = leaf;
        node_existed = false;
    }

    Node* node = m_root;
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
                Node* new_node = new Node();
                new_node->parent = node;
                new_node->descendant = leaf;
                node->right = new_node;
                if (node_existed) {
                    node->descendant = nullptr;
                }
                m_nodes[h].insert(std::make_pair(key >> h, new_node));
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
                Node* new_node = new Node();
                new_node->parent = node;
                new_node->descendant = leaf;
                node->left = new_node;
                if (node_existed) {
                    node->descendant = nullptr;
                }
                m_nodes[h].insert(std::make_pair(key >> h, new_node));
                node_existed = false;
            }
            node = node->left;
        }
    }

    // These pointers are never dereferenced and only tested for null. Let this be an undocumented feature.
    if (key & 1) {
        node->right = reinterpret_cast<Node*>(leaf);
    }
    else {
        node->left = reinterpret_cast<Node*>(leaf);
    }
    if (node_existed) {
        node->descendant = nullptr;
    }

    leaf->parent = node;
    m_leaves.insert(std::make_pair(key, leaf));

    return leaf;
}

template <typename Key, typename Value, template<typename...> class Hash>
typename xfast<Key, Value, Hash>::Leaf*
xfast<Key, Value, Hash>::find(Key key) const {
    typename LeafHash::const_iterator i = m_leaves.find(key);
    if (i != m_leaves.end()) {
        Leaf* leaf = i->second;
        return leaf;
    }
    else {
        return nullptr;
    }
}

template <typename Key, typename Value, template<typename...> class Hash>
typename xfast<Key, Value, Hash>::Leaf*
xfast<Key, Value, Hash>::pred(Key key) const {
    Leaf* guess = approx(key);
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

template <typename Key, typename Value, template<typename...> class Hash>
typename xfast<Key, Value, Hash>::Leaf*
xfast<Key, Value, Hash>::succ(Key key) const {
    Leaf* guess = approx(key);
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

} // namespace yfast

#endif
