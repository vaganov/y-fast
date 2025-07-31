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
    Node* root() const { return _root; }
    Node* leftmost() const { return _root != nullptr ? _leftmost(_root) : nullptr; }
    Node* rightmost() const { return _root != nullptr ? _rightmost(_root) : nullptr; }

    Node* find(const Key& key) const {
        auto probe = _root;
        while (probe != nullptr) {
            if (_cmp(probe->key, key)) {
                probe = probe->right();
            }
            else if (_cmp(key, probe->key)) {
                probe = probe->left();
            }
            else {
                return probe;
            }
        }
        return nullptr;
    }

    Node* pred(const Key& key) const {
        auto node = seek(key);
        if (node == nullptr) {
            return nullptr;
        }
        if (_cmp(key, node->key)) {
            return pred(node);
        }
        return node;
    }

    Node* succ(const Key& key) const {
        auto node = seek(key);
        if (node == nullptr) {
            return nullptr;
        }
        if (_cmp(node->key, key)) {
            return succ(node);
        }
        return node;
    }

    Node* insert(Node* node) {
        Node* parent = nullptr;
        auto probe = _root;
        bool left_path;

        while (probe != nullptr) {
            if (_cmp(node->key, probe->key)) {
                left_path = true;
                parent = probe;
                probe = parent->left();
            }
            else if (_cmp(probe->key, node->key)) {
                left_path = false;
                parent = probe;
                probe = parent->right();
            }
            else {
                return probe;  // TODO: replace?
            }
        }

        node->parent = parent;
        node->set_left(nullptr);
        node->set_right(nullptr);
        node->size = 1;

        if (parent != nullptr) {
            if (left_path) {  // NB: assigned if 'parent' non-null
                parent->set_left(node);
            }
            else {
                parent->set_right(node);
            }
        }
        else {
            _root = node;
        }

        inc_size_path(parent);

        return node;
    }

    RemoveReport remove(Node* node) {
        auto parent = node->parent;
        bool left_path;
        if (parent != nullptr) {
            if (node == parent->left()) {
                left_path = true;
            }
            else {
                left_path = false;
            }
        }

        if (node->left() == nullptr && node->right() == nullptr) {
            if (parent != nullptr) {
                if (left_path) {  // NB: assigned if 'parent' non-null
                    parent->set_left(nullptr);
                }
                else {
                    parent->set_right(nullptr);
                }
            }
            else {
                _root = nullptr;
            }
            dec_size_path(parent);
            return { nullptr, parent, nullptr, left_path };
        }

        if (node->left() == nullptr) {  // => node->right != nullptr
            if (parent != nullptr) {
                if (left_path) {  // NB: assigned if 'parent' non-null
                    link_left(parent, node->right());
                }
                else {
                    link_right(parent, node->right());
                }
            }
            else {
                _root = node->right();
                _root->parent = nullptr;
            }
            dec_size_path(parent);
            return { node->right(), parent, node->right(), left_path };
        }

        if (node->right() == nullptr) {  // => node->left != nullptr
            if (parent != nullptr) {
                if (left_path) {  // NB: assigned if 'parent' non-null
                    link_left(parent, node->left());
                }
                else {
                    link_right(parent, node->left());
                }
            }
            else {
                _root = node->left();
                _root->parent = nullptr;
            }
            dec_size_path(parent);
            return { node->left(), parent, node->left(), left_path };
        }

        Node* subtree_parent;
        Node* subtree_child;
        bool is_left_child;
        auto succ = BST::succ(node);
        if (succ == node->right()) {
            link_left(succ, node->left());
            succ->size = node->size;
            subtree_parent = succ;
            subtree_child = succ->right();
            is_left_child = false;
        }
        else {
            subtree_parent = succ->parent;
            subtree_child = succ->right();
            link_left(succ->parent, succ->right());
            link_left(succ, node->left());
            link_right(succ, node->right());
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
        return { succ, subtree_parent, subtree_child, is_left_child };
    }

    static Node* pred(Node* node) {
        if (node->left() != nullptr) {
            return _rightmost(node->left());
        }
        for (auto probe = node; probe->parent != nullptr; probe = probe->parent) {
            if (probe == probe->parent->right()) {
                return probe->parent;
            }
        }
        return nullptr;
    }

    static Node* succ(Node* node) {
        if (node->right() != nullptr) {
            return _leftmost(node->right());
        }
        for (auto probe = node; probe->parent != nullptr; probe = probe->parent) {
            if (probe == probe->parent->left()) {
                return probe->parent;
            }
        }
        return nullptr;
    }

protected:
    explicit BST(Node* root, Compare cmp = Compare()): _cmp(cmp), _root(root) {
        if (_root != nullptr) {
            _root->parent = nullptr;
        }
    }

    Node* seek(const Key& key) const {
        auto probe = _root;
        Node* parent = nullptr;
        while (probe != nullptr) {
            if (_cmp(probe->key, key)) {
                parent = probe;
                probe = parent->right();
            }
            else if (_cmp(key, probe->key)) {
                parent = probe;
                probe = parent->left();
            }
            else {
                return probe;
            }
        }
        return parent;
    }

    static Node* _leftmost(Node* node) {
        auto probe = node;
        while (probe->left() != nullptr) {
            probe = probe->left();
        }
        return probe;
    }

    static Node* _rightmost(Node* node) {
        auto probe = node;
        while (probe->right() != nullptr) {
            probe = probe->right();
        }
        return probe;
    }

    static void link_left(Node* parent, Node* child) {
        if (parent != nullptr) {
            parent->set_left(child);
        }
        if (child != nullptr) {
            child->parent = parent;
        }
    }

    static void link_right(Node* parent, Node* child) {
        if (parent != nullptr) {
            parent->set_right(child);
        }
        if (child != nullptr) {
            child->parent = parent;
        }
    }

    static void update_size(Node* node) {
        if (node != nullptr) {
            auto left = node->left();
            auto left_size = left != nullptr ? left->size : 0;
            auto right = node->right();
            auto right_size = right != nullptr ? right->size : 0;
            node->size = 1 + left_size + right_size;
        }
    }

    static void update_size_path(Node* node) {
        for (auto ancestor = node; ancestor != nullptr; ancestor = ancestor->parent) {
            update_size(ancestor);
        }
    }

    static void inc_size_path(Node* node) {
        for (auto ancestor = node; ancestor != nullptr; ancestor = ancestor->parent) {
            ++(ancestor->size);
        }
    }

    static void dec_size_path(Node* node) {
        for (auto ancestor = node; ancestor != nullptr; ancestor = ancestor->parent) {
            --(ancestor->size);
        }
    }

private:
    static void destroy(Node* node) {
        if (node != nullptr) {
            destroy(node->left());
            destroy(node->right());
            delete node;
        }
    }
};

}

#endif
