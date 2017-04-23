#ifndef _XFAST_H
#define _XFAST_H

namespace yfast {

// xfast_leaf

template <typename _Value>
struct xfast_leaf {
    xfast_leaf* prv;
    xfast_leaf* nxt;
    _Value value;

    xfast_leaf(const _Value& value);
};

// xfast_node

template <typename _Value>
struct xfast_node {
    xfast_node* left;
    xfast_node* right;
    xfast_leaf<_Value>* descendant;

    xfast_node();
};

// xfast

template <typename _Key, typename _Value, typename _Hash>
class xfast {
  public:
    typedef _Key Key;
    typedef _Value Value;

  private:
    // private types
    typedef xfast_node<_Value> _Node;
    typedef xfast_leaf<_Value> _Leaf;

    // private static fields
    static const int H = 8 * sizeof(_Key);

    // fields
    _Node* m_root;
    _Hash m_hash[H];

    // private methods
    _Leaf* pred(_Key key);
    _Leaf* succ(_Key key);

  public:
    // ctor
    xfast();

    // public methods
    void insert(_Key key, const _Value& value);
};

// xfast_leaf

template <typename _Value>
xfast_leaf<_Value>::xfast_leaf(const _Value& _value)
    : prv(nullptr), nxt(nullptr), value(_value) {
}

// xfast_node

template <typename _Value>
xfast_node<_Value>::xfast_node()
    : left(nullptr), right(nullptr), descendant(nullptr) {
}

// xfast

// private methods

template <typename _Key, typename _Value, typename _Hash>
typename xfast<_Key, _Value, _Hash>::_Leaf*
xfast<_Key, _Value, _Hash>::pred(_Key key) {
    int l = 0;
    int r = H - 1;
    int m;
    while (r - l > 1) {
        m = (r + l) / 2;
        if (m_hash[m].count(key) > 0) { // XXX
            r = m;
        }
        else {
            l = m;
        }
    }
    if (m_hash[l].count(key) > 0) { // XXX
        m = l;
    }
    else {
        m = r;
    }
    typename _Hash::const_iterator i = m_hash[m].find(key); // XXX
    void* p = i->second;
    _Node* node = static_cast<_Node*>(p);
    _Leaf* leaf = node->descendant;
    return leaf; // XXX
}

template <typename _Key, typename _Value, typename _Hash>
typename xfast<_Key, _Value, _Hash>::_Leaf*
xfast<_Key, _Value, _Hash>::succ(_Key key) {
}

// ctor

template <typename _Key, typename _Value, typename _Hash>
xfast<_Key, _Value, _Hash>::xfast() : m_root(nullptr) {
}

// public methods

template <typename _Key, typename _Value, typename _Hash>
void xfast<_Key, _Value, _Hash>::insert(_Key key, const _Value& value) {
    _Leaf* prv = pred(key);
    _Leaf* nxt = succ(key);
    _Leaf* leaf = new _Leaf(value);
    prv->nxt = leaf;
    leaf->prv = prv;
    leaf->nxt = nxt;
    nxt->prv = leaf;

    _Node* node = m_root;
    for (int h = H - 1; h > 0; --h) {
        if (key & (1 << h)) {
            if (nullptr == node->left) {
                _Node* new_node = new _Node();
                node->left = new_node;
                void* p = new_node;
                m_hash[h].insert(std::make_pair(key, p)); // XXX
            }
            node = node->left;
        }
        else {
            if (nullptr == node->right) {
                _Node* new_node = new _Node();
                node->right = new_node;
                void* p = new_node;
                m_hash[h].insert(std::make_pair(key, p)); // XXX
            }
            node = node->right;
        }
    }

    node->descendant = leaf;

    void* p = leaf;
    m_hash[0].insert(std::make_pair(key, p)); // XXX
}

} // namespace yfast

#endif
