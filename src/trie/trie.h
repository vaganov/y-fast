#ifndef _TRIE_H
#define _TRIE_H

namespace yfast {

struct trie_node {
    trie_node* m_left;
    trie_node* m_right;

    trie_node();
};

template <typename _Key, typename _Value>
class trie {
  public:
    typedef _Key Key;
    typedef _Value Value;

    typedef trie_node Node;

  private:
    static const int H = 8 * sizeof(_Key);

    Node* m_root;

  public:
    trie();

    void insert(_Key key, const _Value value);
    Value* find(_Key key);
};

inline
trie_node::trie_node() : m_left(nullptr), m_right(nullptr) {
}

template <typename _Key, typename _Value>
trie<_Key, _Value>::trie() : m_root(nullptr) {
}

template <typename _Key, typename _Value>
void trie<_Key, _Value>::insert(_Key key, const _Value value) {
    if (nullptr == m_root) {
        m_root = new Node();
    }
    Node* node = m_root;

    for (int h = H - 1; h > 0; --h) {
        if (key & (1 << h)) {
            if (nullptr == node->m_right) {
                node->m_right = new Node();
            }
            node = node->m_right;
        }
        else {
            if (nullptr == node->m_left) {
                node->m_left = new Node();
            }
            node = node->m_left;
        }
    }

    if (key & 1) {
        if (nullptr == node->m_right) {
            node->m_right = reinterpret_cast<Node*>(new _Value(value));
        }
        else {
            // ?
        }
    }
    else {
        if (nullptr == node->m_left) {
            node->m_left = reinterpret_cast<Node*>(new _Value(value));
        }
        else {
            // ?
        }
    }
}

template <typename _Key, typename _Value>
typename trie<_Key, _Value>::Value* trie<_Key, _Value>::find(_Key key) {
    Node* node = m_root;
    if (nullptr == node) {
        return nullptr;
    }

    for (int h = H - 1; h > 0; --h) {
        if (key & (1 << h)) {
            if (nullptr == node->m_right) {
                return nullptr;
            }
            node = node->m_right;
        }
        else {
            if (nullptr == node->m_left) {
                return nullptr;
            }
            node = node->m_left;
        }
    }

    if (key & 1) {
        if (nullptr != node->m_right) {
            return reinterpret_cast<_Value*>(node->m_right);
        }
        else {
            return nullptr;
        }
    }
    else {
        if (nullptr != node->m_left) {
            return reinterpret_cast<_Value*>(node->m_left);
        }
        else {
            return nullptr;
        }
    }
}

} // namespace yfast

#endif
