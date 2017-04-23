#ifndef _TRIE_H
#define _TRIE_H

namespace yfast {

struct trie_node {
    trie_node* left;
    trie_node* right;

    trie_node();
};

template <typename _Key, typename _Value, typename _Node = trie_node>
class trie {
  public:
    typedef _Key Key;
    typedef _Value Value;

  private:
    static const int H = 8 * sizeof(_Key);

    _Node* m_root;

  public:
    trie();

    void insert(_Key key, const _Value value);
    Value* find(_Key key);
};

inline
trie_node::trie_node() : left(nullptr), right(nullptr) {
}

template <typename _Key, typename _Value, typename _Node>
trie<_Key, _Value, _Node>::trie() : m_root(nullptr) {
}

template <typename _Key, typename _Value, typename _Node>
void trie<_Key, _Value, _Node>::insert(_Key key, const _Value value) {
    if (nullptr == m_root) {
        m_root = new _Node();
    }
    _Node* node = m_root;

    for (int h = H - 1; h > 0; --h) {
        if (key & (1 << h)) {
            if (nullptr == node->right) {
                node->right = new _Node();
            }
            node = node->right;
        }
        else {
            if (nullptr == node->left) {
                node->left = new _Node();
            }
            node = node->left;
        }
    }

    if (key & 1) {
        if (nullptr == node->right) {
            node->right = reinterpret_cast<_Node*>(new _Value(value));
        }
        else {
            // ?
        }
    }
    else {
        if (nullptr == node->left) {
            node->left = reinterpret_cast<_Node*>(new _Value(value));
        }
        else {
            // ?
        }
    }
}

template <typename _Key, typename _Value, typename _Node>
typename trie<_Key, _Value, _Node>::Value*
trie<_Key, _Value, _Node>::find(_Key key) {
    _Node* node = m_root;
    if (nullptr == node) {
        return nullptr;
    }

    for (int h = H - 1; h > 0; --h) {
        if (key & (1 << h)) {
            if (nullptr == node->right) {
                return nullptr;
            }
            node = node->right;
        }
        else {
            if (nullptr == node->left) {
                return nullptr;
            }
            node = node->left;
        }
    }

    if (key & 1) {
        if (nullptr != node->right) {
            return reinterpret_cast<_Value*>(node->right);
        }
        else {
            return nullptr;
        }
    }
    else {
        if (nullptr != node->left) {
            return reinterpret_cast<_Value*>(node->left);
        }
        else {
            return nullptr;
        }
    }
}

} // namespace yfast

#endif
