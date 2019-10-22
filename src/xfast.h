#ifndef _XFAST_H
#define _XFAST_H

#include <utility>

namespace yfast {

// xfast_leaf

template <typename Key, typename Value>
struct xfast_leaf {
    xfast_leaf* prv;
    xfast_leaf* nxt;
    Key key;
    Value value;

    xfast_leaf(Key key, const Value& value);
};

// xfast_node

template <typename Key, typename Value>
struct xfast_node {
    xfast_node* left;
    xfast_node* right;
    xfast_leaf<Key, Value>* descendant;

    xfast_node();
};

// xfast

template <typename Key, typename Value, typename Hash>
class xfast {
  public:
    // public types
    typedef xfast_leaf<Key, Value> Leaf;

  private:
    // private types
    typedef xfast_node<Key, Value> Node;

    // private static fields
    static const int H = 8 * sizeof(Key);

    // fields
    Node* m_root;
    Hash m_hash[H];

    // private methods
    Leaf* approx(Key key) const;

  public:
    // ctor
    xfast();

    // public methods
    void insert(Key key, const Value& value);
    Leaf* find(Key key) const;
    Leaf* pred(Key key) const;
    Leaf* succ(Key key) const;

};

// xfast_leaf

template <typename Key, typename Value>
xfast_leaf<Key, Value>::xfast_leaf(Key _key, const Value& _value)
    : prv(nullptr), nxt(nullptr), key(_key), value(_value) {
}

// xfast_node

template <typename Key, typename Value>
xfast_node<Key, Value>::xfast_node()
    : left(nullptr), right(nullptr), descendant(nullptr) {
}

// xfast

// private methods

template <typename Key, typename Value, typename Hash>
typename xfast<Key, Value, Hash>::Leaf*
xfast<Key, Value, Hash>::approx(Key key) const {
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
        Leaf* leaf = m_root->descendant;
        return leaf;
    }
    if (0 == m) {
        typename Hash::const_iterator i = m_hash[0].find(key);
        void* p = i->second;
        Leaf* leaf = static_cast<Leaf*>(p);
        return leaf;
    }
    typename Hash::const_iterator i = m_hash[m].find(key >> m);
    void* p = i->second;
    Node* node = static_cast<Node*>(p);
    Leaf* leaf = node->descendant;
    return leaf;
}

// ctor

template <typename Key, typename Value, typename Hash>
xfast<Key, Value, Hash>::xfast() : m_root(nullptr) {
}

// public methods

template <typename Key, typename Value, typename Hash>
void xfast<Key, Value, Hash>::insert(Key key, const Value& value) {
    Leaf* prv = nullptr;
    Leaf* nxt = nullptr;
    Leaf* guess = approx(key);
    if (nullptr != guess) {
        const Key guess_key = guess->key;
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
                Node* new_node = new Node();
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
        node->right = reinterpret_cast<Node*>(leaf);
        if (node_existed) {
            node->descendant = nullptr;
        }
    }
    else {
        node->left = reinterpret_cast<Node*>(leaf);
        if (node_existed) {
            node->descendant = nullptr;
        }
    }

    void* p = leaf;
    m_hash[0].insert(std::make_pair(key, p));
}

template <typename Key, typename Value, typename Hash>
typename xfast<Key, Value, Hash>::Leaf*
xfast<Key, Value, Hash>::find(Key key) const {
    typename Hash::const_iterator i = m_hash[0].find(key);
    if (i != m_hash[0].end()) {
        void* p = i->second;
        Leaf* leaf = static_cast<Leaf*>(p);
        return leaf;
    }
    else {
        return nullptr;
    }
}

template <typename Key, typename Value, typename Hash>
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

template <typename Key, typename Value, typename Hash>
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
