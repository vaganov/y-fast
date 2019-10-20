#ifndef _XFAST_H
#define _XFAST_H

#include <utility>

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

} // namespace yfast

#endif
