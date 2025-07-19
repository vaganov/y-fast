#ifndef _YFAST_IMPL_BST_H
#define _YFAST_IMPL_BST_H

#include <functional>

namespace yfast::impl {

template <typename Node, typename Compare = std::less<typename Node::Key>>
class BST {
public:
    typedef typename Node::Key Key;

    typedef struct {
        Node* substitution;
        Node* subtree_parent;  // height changed
        Node* subtree_child;  // height unchanged
        bool is_left_child;  // NB: only matters if subtree_parent != nullptr
    } RemoveReport;

protected:
    Compare _cmp;
    Node* _root;

public:
    explicit BST(Compare cmp = Compare()): BST(nullptr, cmp) {}

    BST(const BST& other) = delete;

    BST(BST&& other) noexcept: _cmp(other._cmp), _root(other._root) {
        other._root = nullptr;
    }

    ~BST() { destroy(_root); }

    [[nodiscard]] unsigned int size() const { return _root != nullptr ? _root->size : 0; }
    const Node* root() const { return _root; }
    const Node* leftmost() const { return _root != nullptr ? _leftmost(_root) : nullptr; }
    const Node* rightmost() const { return _root != nullptr ? _rightmost(_root) : nullptr; }

    Node* find(const Key& key) const;
    Node* pred(const Key& key) const;
    Node* succ(const Key& key) const;

    Node* insert(Node* node);
    RemoveReport remove(Node* node);

protected:
    explicit BST(Node* root, Compare cmp = Compare()): _cmp(cmp), _root(root) {
        if (_root != nullptr) {
            _root->parent = nullptr;
        }
    }

    static Node* _leftmost(Node* node);
    static Node* _rightmost(Node* node);
    static Node* pred(const Node* node);  // TODO: rename _pred
    static Node* succ(const Node* node);  // TODO: rename _succ

    static void link_left(Node* parent, Node* child);
    static void link_right(Node* parent, Node* child);

    static void update_size(Node* node);
    static void update_size_path(Node* node);
    static void inc_size_path(Node* node);
    static void dec_size_path(Node* node);

// private:
    static void destroy(Node* node);

    template <typename OStream>
    void print(OStream& os, const Node* node) const;

    template <typename _Node, typename _Compare, typename OStream>
    friend OStream& operator << (OStream& os, const BST<_Node, _Compare>& tree);
};

template <typename Node, typename Compare>
Node* BST<Node, Compare>::find(const Key& key) const {
    auto probe = _root;
    while (probe != nullptr) {
        if (_cmp(probe->key, key)) {
            probe = probe->right;
        }
        else if (_cmp(key, probe->key)) {
            probe = probe->left;
        }
        else {
            return probe;
        }
    }
    return nullptr;
}

template <typename Node, typename Compare>
Node* BST<Node, Compare>::pred(const Key& key) const {
    auto probe = _root;
    Node* parent = nullptr;
    while (probe != nullptr) {
        parent = probe;
        probe = _cmp(key, parent->key) ? parent->right : parent->left;
    }
    return parent;
}

template <typename Node, typename Compare>
Node* BST<Node, Compare>::succ(const Key& key) const {
    auto probe = _root;
    Node* parent = nullptr;
    while (probe != nullptr) {
        parent = probe;
        probe = _cmp(parent->key, key) ? parent->left : parent->right;
    }
    return parent;
}

template <typename Node, typename Compare>
Node* BST<Node, Compare>::insert(Node* node) {
    Node* parent = nullptr;
    auto probe = _root;
    bool left_path;

    while (probe != nullptr) {
        if (_cmp(node->key, probe->key)) {
            left_path = true;
            parent = probe;
            probe = parent->left;
        }
        else if (_cmp(probe->key, node->key)) {
            left_path = false;
            parent = probe;
            probe = parent->right;
        }
        else {
            return probe;  // TODO: replace?
        }
    }

    node->parent = parent;
    node->left = nullptr;
    node->right = nullptr;
    node->size = 1;

    if (parent != nullptr) {
        if (left_path) {  // NB: assigned if 'parent' non-null
            parent->left = node;
        }
        else {
            parent->right = node;
        }
    }
    else {
        _root = node;
    }

    inc_size_path(parent);

    return node;
}

template <typename Node, typename Compare>
typename BST<Node, Compare>::RemoveReport BST<Node, Compare>::remove(Node* node) {
    auto parent = node->parent;
    bool left_path;
    if (parent != nullptr) {
        if (node == parent->left) {
            left_path = true;
        }
        else {
            left_path = false;
        }
    }

    if (node->left == nullptr && node->right == nullptr) {
        if (parent != nullptr) {
            if (left_path) {  // NB: assigned if 'parent' non-null
                parent->left = nullptr;
            }
            else {
                parent->right = nullptr;
            }
        }
        else {
            _root = nullptr;
        }
        dec_size_path(parent);
        return { nullptr, parent, nullptr, left_path };
    }

    if (node->left == nullptr) {  // => node->right != nullptr
        if (parent != nullptr) {
            if (left_path) {  // NB: assigned if 'parent' non-null
                // parent->left = node->right;
                link_left(parent, node->right);
            }
            else {
                // parent->right = node->right;
                link_right(parent, node->right);
            }
        }
        else {
            _root = node->right;
            _root->parent = nullptr;
        }
        dec_size_path(parent);
        return { node->right, parent, node->right, left_path };
    }

    if (node->right == nullptr) {  // => node->left != nullptr
        if (parent != nullptr) {
            if (left_path) {  // NB: assigned if 'parent' non-null
                // parent->left = node->left;
                link_left(parent, node->left);
            }
            else {
                // parent->right = node->left;
                link_right(parent, node->left);
            }
        }
        else {
            _root = node->left;
            _root->parent = nullptr;
        }
        dec_size_path(parent);
        return { node->left, parent, node->left, left_path };
    }

    Node* subtree_parent;
    Node* subtree_child;
    bool is_left_child;
    auto succ = BST::succ(node);
    if (succ == node->right) {
        link_left(succ, node->left);
        succ->size = node->size;
        subtree_parent = succ;
        subtree_child = succ->right;
        is_left_child = false;
    }
    else {
        subtree_parent = succ->parent;
        subtree_child = succ->right;
        link_left(succ->parent, succ->right);
        link_left(succ, node->left);
        link_right(succ, node->right);
        succ->size = node->size;
        is_left_child = true;
    }
    if (parent != nullptr) {
        if (left_path) {  // NB: assigned if 'parent' non-null
            link_left(parent, succ);
        }
        else {
            link_right(parent, succ);
        }
    }
    else {
        succ->parent = nullptr;
        _root = succ;
    }
    dec_size_path(subtree_parent);
    succ->balance_factor = node->balance_factor;  // FIXME
    return { succ, subtree_parent, subtree_child, is_left_child };
}

template <typename Node, typename Compare>
Node* BST<Node, Compare>::_rightmost(Node* node) {
    auto probe = node;
    while (probe->right != nullptr) {
        probe = probe->right;
    }
    return probe;
}

template <typename Node, typename Compare>
Node* BST<Node, Compare>::_leftmost(Node* node) {
    auto probe = node;
    while (probe->left != nullptr) {
        probe = probe->left;
    }
    return probe;
}

template <typename Node, typename Compare>
Node* BST<Node, Compare>::pred(const Node* node) {
    if (node->left != nullptr) {
        return _rightmost(node->left);
    }
    auto probe = node->parent;
    if (probe == nullptr) {
        return nullptr;
    }
    // while (probe->parent != nullptr && probe == probe->parent->right) {
    while (probe->parent != nullptr && probe == probe->parent->left) {
        probe = probe->parent;
    }
    return probe->parent;
}

template <typename Node, typename Compare>
Node* BST<Node, Compare>::succ(const Node* node) {
    if (node->right != nullptr) {
        return _leftmost(node->right);
    }
    auto probe = node->parent;
    if (probe == nullptr) {
        return nullptr;
    }
    // while (probe->parent != nullptr && (probe == probe->parent->left)) {
    while (probe->parent != nullptr && probe == probe->parent->right) {
        probe = probe->parent;
    }
    return probe->parent;
}

template <typename Node, typename Compare>
void BST<Node, Compare>::link_left(Node* parent, Node* child) {
    if (parent != nullptr) {
        parent->left = child;
    }
    if (child != nullptr) {
        child->parent = parent;
    }
}

template <typename Node, typename Compare>
void BST<Node, Compare>::link_right(Node* parent, Node* child) {
    if (parent != nullptr) {
        parent->right = child;
    }
    if (child != nullptr) {
        child->parent = parent;
    }
}

template <typename Node, typename Compare>
void BST<Node, Compare>::update_size(Node* node) {
    if (node != nullptr) {
        auto left_size = node->left != nullptr ? node->left->size : 0;
        auto right_size = node->right != nullptr ? node->right->size : 0;
        node->size = 1 + left_size + right_size;
    }
}

template <typename Node, typename Compare>
void BST<Node, Compare>::update_size_path(Node* node) {
    for (auto ancestor = node; ancestor != nullptr; ancestor = ancestor->parent) {
        update_size(ancestor);
    }
}

template <typename Node, typename Compare>
void BST<Node, Compare>::inc_size_path(Node* node) {
    for (auto ancestor = node; ancestor != nullptr; ancestor = ancestor->parent) {
        ++(ancestor->size);
    }
}

template <typename Node, typename Compare>
void BST<Node, Compare>::dec_size_path(Node* node) {
    for (auto ancestor = node; ancestor != nullptr; ancestor = ancestor->parent) {
        --(ancestor->size);
    }
}

template <typename Node, typename Compare>
void BST<Node, Compare>::destroy(Node* node) {
    if (node != nullptr) {
        destroy(node->left);
        destroy(node->right);
        delete node;
    }
}

template <typename Node, typename Compare>
template <typename OStream>
void BST<Node, Compare>::print(OStream& os, const Node* node) const {
    if (node != nullptr) {
        // os << *node;  // NB: requires 'operator <<' for 'Node'
        os << node->key;
        os << " (";
        print(os, node->left);
        os << ", ";
        print(os, node->right);
        os << ")";
    }
    else {
        os << "NULL";
    }
}

template<typename _Node, typename _Eq, typename _Compare, typename OStream>
OStream& operator << (OStream& os, const BST<_Node, _Compare>& tree) {
    tree.print(os, tree._root);
    return os << " [size=" << tree.size() << "]";
}

}

#endif
