#ifndef _YFAST_BST_H
#define _YFAST_BST_H

#include <concepts>
#include <functional>

namespace yfast {

template <typename Node>
concept NodeGeneric = requires (Node node) {
    { node.key } -> std::convertible_to<typename Node::Key>;
    { node.parent } -> std::convertible_to<Node*>;
    { node.left } -> std::convertible_to<Node*>;
    { node.right } -> std::convertible_to<Node*>;
    { node.size } -> std::convertible_to<unsigned int>;
};

template <typename Eq, typename Key>
concept EqGeneric = requires (Eq eq, Key lhs, Key rhs) {
    { eq(lhs, rhs) } -> std::same_as<bool>;
};

template <typename Compare, typename Key>
concept CompareGeneric = requires (Compare cmp, Key lhs, Key rhs) {
    { cmp(lhs, rhs) } -> std::same_as<bool>;
};

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

    Node* find(const Key& key) const;

    Node* insert(Node* node);
    RemoveReport remove(Node* node);

protected:
    explicit bst(Node* root, Eq eq = Eq(), Compare cmp = Compare()): _eq(eq), _cmp(cmp), _root(root) {
        if (_root != nullptr) {
            _root->parent = nullptr;
        }
    }

    static Node* pred(const Node* node);
    static Node* succ(const Node* node);

    static void link_left(Node* parent, Node* child);
    static void link_right(Node* parent, Node* child);

    static void update_size(Node* node);
    static void inc_size_path(Node* node);
    static void dec_size_path(Node* node);

private:
    static void destroy(Node* node);

    template <typename OStream>
    void print(OStream& os, const Node* node) const;

    template <NodeGeneric Node, typename Eq, typename Compare, typename OStream>
    friend OStream& operator << (OStream& os, const bst<Node, Eq, Compare>& tree);
};

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
bst<_Node, _Eq, _Compare>::Node* bst<_Node, _Eq, _Compare>::find(const Key& key) const {
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
typename bst<_Node, _Eq, _Compare>::Node* bst<_Node, _Eq, _Compare>::insert(Node* node) {
    Node* parent = nullptr;
    auto probe = _root;
    bool left_path;

    while (probe != nullptr) {
        if (_eq(node->key, probe->key)) {
            return probe;
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
    return {succ, subtree_parent, subtree_child};
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename bst<_Node, _Eq, _Compare>::Node* bst<_Node, _Eq, _Compare>::pred(const Node* node) {  // FIXME: succ
    if (node->left != nullptr) {
        auto probe = node->left;
        while (probe->right != nullptr) {
            probe = probe->right;
        }
        return probe;
    }
    else {
        auto probe = node;
        auto parent = probe->parent;
        while (parent != nullptr && probe == parent->left) {
            probe = parent;
            parent = probe->parent;
        }
        return parent;
    }
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename bst<_Node, _Eq, _Compare>::Node* bst<_Node, _Eq, _Compare>::succ(const Node* node) {  // FIXME: pred
    if (node->right != nullptr) {
        auto probe = node->right;
        while (probe->left != nullptr) {
            probe = probe->left;
        }
        return probe;
    }
    else {
        auto probe = node;
        auto parent = probe->parent;
        while (parent != nullptr && probe == parent->right) {
            probe = parent;
            parent = probe->parent;
        }
        return parent;
    }
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
void bst<_Node, _Eq, _Compare>::link_left(Node* parent, Node* child) {
    if (parent != nullptr) {
        parent->left = child;
    }
    if (child != nullptr) {
        child->parent = parent;
    }
}

template <NodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
void bst<_Node, _Eq, _Compare>::link_right(Node* parent, Node* child) {
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
        os << *node;  // NB: requires 'operator <<' for 'Node'
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

}

#endif
