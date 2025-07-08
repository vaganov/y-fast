#ifndef _YFAST_IMPL_BST_H
#define _YFAST_IMPL_BST_H

#include <algorithm>
#include <functional>

#include <yfast/internal/concepts.h>

// #define DEBUG

namespace yfast::impl {

using internal::NodeGeneric;
using internal::EqGeneric;
using internal::CompareGeneric;

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq = std::equal_to<typename _Node::Key>, CompareGeneric<typename _Node::Key> _Compare = std::less<typename _Node::Key>>
class bst {
public:
    typedef _Node Node;
    typedef typename Node::Key Key;
    typedef _Eq Eq;
    typedef _Compare Compare;

    typedef struct {
        Node* substitution;
        Node* subtree_parent;  // height changed
        Node* subtree_child;  // height unchanged
    } RemoveReport;

protected:
    Eq _eq;
    Compare _cmp;
    Node* _root;

public:
    explicit bst(Eq eq = Eq(), Compare cmp = Compare()): bst(nullptr, eq, cmp) {}
    ~bst() { destroy(_root); }

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
    explicit bst(Node* root, Eq eq = Eq(), Compare cmp = Compare()): _eq(eq), _cmp(cmp), _root(root) {
        if (_root != nullptr) {
            _root->parent = nullptr;
        }
    }

    bst(bst&& other) noexcept: _eq(other._eq), _cmp(other._cmp), _root(other._root) {
        other._root = nullptr;
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

#ifdef DEBUG
    static void check_sanity(const Node* node);
#endif

private:
    static void destroy(Node* node);

    template <typename OStream>
    void print(OStream& os, const Node* node) const;

    template <NodeGeneric Node, typename Eq, typename Compare, typename OStream>
    friend OStream& operator << (OStream& os, const bst<Node, Eq, Compare>& tree);
};

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
const typename bst<_Node, _Eq, _Compare>::Key& bst<_Node, _Eq, _Compare>::sample() const {
    if (_root != nullptr) {
        return _root->key;
    }
    static const auto default_key = Key();
    return default_key;
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
const typename bst<_Node, _Eq, _Compare>::Key& bst<_Node, _Eq, _Compare>::min() const {
    if (_root != nullptr) {
        return leftmost(_root)->key;
    }
    static const auto default_key = Key();
    return default_key;
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
const typename bst<_Node, _Eq, _Compare>::Key& bst<_Node, _Eq, _Compare>::max() const {
    if (_root != nullptr) {
        return rightmost(_root)->key;
    }
    static const auto default_key = Key();
    return default_key;
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename bst<_Node, _Eq, _Compare>::Node* bst<_Node, _Eq, _Compare>::find(const Key& key) const {
    auto probe = _root;
    while (probe != nullptr) {
        if (_eq(probe->key, key)) {
            return probe;
        }
        if (_cmp(probe->key, key)) {
            probe = probe->left;
        }
        else {
            probe = probe->right;
        }
    }
    return nullptr;
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename bst<_Node, _Eq, _Compare>::Node* bst<_Node, _Eq, _Compare>::pred(const Key& key) const {
    auto probe = _root;
    Node* parent = nullptr;
    while (probe != nullptr) {
        parent = probe;
        probe = _cmp(key, parent->key) ? parent->right : parent->left;
    }
    return parent;
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename bst<_Node, _Eq, _Compare>::Node* bst<_Node, _Eq, _Compare>::succ(const Key& key) const {
    auto probe = _root;
    Node* parent = nullptr;
    while (probe != nullptr) {
        parent = probe;
        probe = _cmp(parent->key, key) ? parent->left : parent->right;
    }
    return parent;
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename bst<_Node, _Eq, _Compare>::Node* bst<_Node, _Eq, _Compare>::insert(Node* node) {
#ifdef DEBUG
    print(std::cerr, _root);
    std::cerr << std::endl;
#endif
    Node* parent = nullptr;
    auto probe = _root;
    bool left_path;

    while (probe != nullptr) {
        if (_eq(node->key, probe->key)) {
            return probe;  // TODO: replace?
        }
        parent = probe;
        if (_cmp(node->key, parent->key)) {
            left_path = true;
            probe = parent->left;
        }
        else {
            left_path = false;
            probe = parent->right;
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

#ifdef DEBUG
    check_sanity(_root);
#endif

    inc_size_path(parent);
#ifdef WITH_HEIGHT
    update_height_path(parent);
#endif

    return node;
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename bst<_Node, _Eq, _Compare>::RemoveReport bst<_Node, _Eq, _Compare>::remove(Node* node) {
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
    auto succ = bst::succ(node);
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

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
bst<_Node, _Eq, _Compare>::Node* bst<_Node, _Eq, _Compare>::rightmost(Node* node) {
    auto probe = node;
    while (probe->right != nullptr) {
        probe = probe->right;
    }
    return probe;
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
bst<_Node, _Eq, _Compare>::Node* bst<_Node, _Eq, _Compare>::leftmost(Node* node) {
    auto probe = node;
    while (probe->left != nullptr) {
        probe = probe->left;
    }
    return probe;
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename bst<_Node, _Eq, _Compare>::Node* bst<_Node, _Eq, _Compare>::pred(const Node* node) {
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

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename bst<_Node, _Eq, _Compare>::Node* bst<_Node, _Eq, _Compare>::succ(const Node* node) {
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

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
void bst<_Node, _Eq, _Compare>::link_left(Node* parent, Node* child) {
#ifdef DEBUG
    if (parent != nullptr && child != nullptr) {
        if (child->key > parent->key) {
            asm("nop");
        }
    }
#endif
    if (parent != nullptr) {
        parent->left = child;
    }
    if (child != nullptr) {
        child->parent = parent;
    }
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
void bst<_Node, _Eq, _Compare>::link_right(Node* parent, Node* child) {
#ifdef DEBUG
    if (parent != nullptr && child != nullptr) {
        if (child->key < parent->key) {
            asm("nop");
        }
    }
#endif
    if (parent != nullptr) {
        parent->right = child;
    }
    if (child != nullptr) {
        child->parent = parent;
    }
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
void bst<_Node, _Eq, _Compare>::update_size(Node* node) {
    if (node != nullptr) {
        auto left_size = node->left != nullptr ? node->left->size : 0;
        auto right_size = node->right != nullptr ? node->right->size : 0;
        node->size = 1 + left_size + right_size;
    }
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
void bst<_Node, _Eq, _Compare>::update_size_path(Node* node) {
    for (auto ancestor = node; ancestor != nullptr; ancestor = ancestor->parent) {
        update_size(ancestor);
    }
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
void bst<_Node, _Eq, _Compare>::inc_size_path(Node* node) {
    for (auto ancestor = node; ancestor != nullptr; ancestor = ancestor->parent) {
        ++(ancestor->size);
    }
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
void bst<_Node, _Eq, _Compare>::dec_size_path(Node* node) {
    for (auto ancestor = node; ancestor != nullptr; ancestor = ancestor->parent) {
        --(ancestor->size);
    }
}

#ifdef WITH_HEIGHT
template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
void bst<_Node, _Eq, _Compare>::update_height(Node* node) {
    if (node != nullptr) {
        auto left_height = node->left != nullptr ? node->left->height : 0;
        auto right_height = node->right != nullptr ? node->right->height : 0;
        node->height = 1 + std::max(left_height, right_height);
    }
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
void bst<_Node, _Eq, _Compare>::update_height_path(Node* node) {
    for (auto ancestor = node; ancestor != nullptr; ancestor = ancestor->parent) {
        update_height(ancestor);
    }
}
#endif

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
void bst<_Node, _Eq, _Compare>::destroy(Node* node) {
    if (node != nullptr) {
        destroy(node->left);
        destroy(node->right);
        delete node;
    }
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
template <typename OStream>
void bst<_Node, _Eq, _Compare>::print(OStream& os, const Node* node) const {
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

template<NodeGeneric _Node, typename _Eq, typename _Compare, typename OStream>
OStream& operator << (OStream& os, const bst<_Node, _Eq, _Compare>& tree) {
    tree.print(os, tree._root);
    return os << " [size=" << tree.size() << "]";
}

#ifdef DEBUG
template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
void bst<_Node, _Eq, _Compare>::check_sanity(const Node* node) {
    if (node != nullptr) {
        if (node->left != nullptr && node->key < node->left->key) {
            asm("nop");
        }
        if (node->right != nullptr && node->key > node->right->key) {
            asm("nop");
        }
        check_sanity(node->left);
        check_sanity(node->right);
    }
}
#endif

}

#endif
