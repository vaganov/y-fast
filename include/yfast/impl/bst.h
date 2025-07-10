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
#ifdef WITH_HEIGHT
    [[nodiscard]] unsigned int height() const { return _root != nullptr ? _root->height : 0; }
#endif
    const Key& sample() const;
    const Key& min() const;
    const Key& max() const;

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

    static Node* rightmost(Node* node);
    static Node* leftmost(Node* node);
    static Node* pred(const Node* node);
    static Node* succ(const Node* node);

    static void link_left(Node* parent, Node* child);
    static void link_right(Node* parent, Node* child);

    static void update_size(Node* node);
    static void update_size_path(Node* node);
    static void inc_size_path(Node* node);
    static void dec_size_path(Node* node);

#ifdef WITH_HEIGHT
    static void update_height(Node* node);
    static void update_height_path(Node* node);
#endif

private:
    static void destroy(Node* node);
};

template <typename Node, typename Compare>
const typename BST<Node, Compare>::Key& BST<Node, Compare>::sample() const {
    if (_root != nullptr) {
        return _root->key;
    }
    static const auto default_key = Key();
    return default_key;
}

template <typename Node, typename Compare>
const typename BST<Node, Compare>::Key& BST<Node, Compare>::min() const {
    if (_root != nullptr) {
        return leftmost(_root)->key;
    }
    static const auto default_key = Key();
    return default_key;
}

template <typename Node, typename Compare>
const typename BST<Node, Compare>::Key& BST<Node, Compare>::max() const {
    if (_root != nullptr) {
        return rightmost(_root)->key;
    }
    static const auto default_key = Key();
    return default_key;
}

template <typename Node, typename Compare>
Node* BST<Node, Compare>::find(const Key& key) const {
    auto probe = _root;
    while (probe != nullptr) {
        if (_cmp(probe->key, key)) {
            probe = probe->left;
        }
        else if (_cmp(key, probe->key)) {
            probe = probe->right;
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
#ifdef WITH_HEIGHT
    node->height = 1;
#endif

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
#ifdef WITH_HEIGHT
    update_height_path(parent);
#endif

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
#ifdef WITH_HEIGHT
        update_height_path(parent);
#endif
        return {nullptr, parent, nullptr};
    }

    if (node->left == nullptr) {  // => node->right != nullptr
        if (parent != nullptr) {
            if (left_path) {  // NB: assigned if 'parent' non-null
                parent->left = node->right;
            }
            else {
                parent->right = node->right;
            }
        }
        else {
            _root = node->right;
        }
        dec_size_path(parent);
#ifdef WITH_HEIGHT
        update_height_path(parent);
#endif
        return {node->right, parent, node->right};
    }

    if (node->right == nullptr) {  // => node->left != nullptr
        if (parent != nullptr) {
            if (left_path) {  // NB: assigned if 'parent' non-null
                parent->left = node->left;
            }
            else {
                parent->right = node->left;
            }
        }
        else {
            _root = node->left;
        }
        dec_size_path(parent);
#ifdef WITH_HEIGHT
        update_height_path(parent);
#endif
        return {node->left, parent, node->left};
    }

    Node* subtree_parent;
    Node* subtree_child;
    auto succ = BST::succ(node);
    if (succ == node->right) {
        link_left(succ, node->left);
        update_size(succ);
        subtree_parent = succ;
        subtree_child = succ->right;
    }
    else {
        link_left(succ->parent, succ->right);
        link_left(succ, node->left);
        link_right(succ, node->right);
        succ->size = node->size;
        subtree_parent = succ->parent;
        subtree_child = succ->right;
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
#ifdef WITH_HEIGHT
    update_height_path(subtree_parent);
#endif
    return {succ, subtree_parent, subtree_child};
}

template <typename Node, typename Compare>
Node* BST<Node, Compare>::rightmost(Node* node) {
    auto probe = node;
    while (probe->right != nullptr) {
        probe = probe->right;
    }
    return probe;
}

template <typename Node, typename Compare>
Node* BST<Node, Compare>::leftmost(Node* node) {
    auto probe = node;
    while (probe->left != nullptr) {
        probe = probe->left;
    }
    return probe;
}

template <typename Node, typename Compare>
Node* BST<Node, Compare>::pred(const Node* node) {
    if (node->left != nullptr) {
        return rightmost(node->left);
    }
    auto probe = node->parent;
    if (probe == nullptr) {
        return nullptr;
    }
    while (probe->parent != nullptr && probe == probe->parent->right) {
        probe = probe->parent;
    }
    return probe;
}

template <typename Node, typename Compare>
Node* BST<Node, Compare>::succ(const Node* node) {
    if (node->right != nullptr) {
        return leftmost(node->right);
    }
    auto probe = node->parent;
    if (probe == nullptr) {
        return nullptr;
    }
    while (probe->parent != nullptr && probe == probe->parent->left) {
        probe = probe->parent;
    }
    return probe;
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

#ifdef WITH_HEIGHT
template <typename Node, typename Compare>
void BST<Node, Compare>::update_height(Node* node) {
    if (node != nullptr) {
        auto left_height = node->left != nullptr ? node->left->height : 0;
        auto right_height = node->right != nullptr ? node->right->height : 0;
        node->height = 1 + std::max(left_height, right_height);
    }
}

template <typename Node, typename Compare>
void BST<Node, Compare>::update_height_path(Node* node) {
    for (auto ancestor = node; ancestor != nullptr; ancestor = ancestor->parent) {
        update_height(ancestor);
    }
}
#endif

template <typename Node, typename Compare>
void BST<Node, Compare>::destroy(Node* node) {
    if (node != nullptr) {
        destroy(node->left);
        destroy(node->right);
        delete node;
    }
}

}

#endif
