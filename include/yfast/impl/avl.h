#ifndef _YFAST_IMPL_AVL_H
#define _YFAST_IMPL_AVL_H

#include <functional>

#include <yfast/impl/bst.h>

namespace yfast::impl {

template <typename Node, typename Compare = std::less<typename Node::Key>>
class AVL: public BST<Node, Compare> {
public:
    using typename BST<Node, Compare>::Key;

    typedef struct {
        AVL left;
        AVL right;
    } SplitResult;

protected:
    using BST<Node, Compare>::_cmp;
    using BST<Node, Compare>::_root;

public:
    explicit AVL(Compare cmp = Compare()): AVL(nullptr, cmp) {}
    AVL(const AVL& other) = delete;
    AVL(AVL&& other) noexcept: BST<Node, Compare>(std::move(other)) {}

    Node* insert(Node* node);
    Node* remove(Node* node);

    SplitResult split();
    static AVL merge(AVL& left, AVL& right);

protected:
    explicit AVL(Node* root, Compare cmp = Compare()): BST<Node, Compare>(root, cmp) {}

    using BST<Node, Compare>::link_left;
    using BST<Node, Compare>::link_right;
    using BST<Node, Compare>::update_size;
    using BST<Node, Compare>::update_size_path;
#ifdef WITH_HEIGHT
    using BST<Node, Compare>::update_height;
#endif
    using BST<Node, Compare>::leftmost;
    using BST<Node, Compare>::rightmost;

private:
    static Node* rotate_left(Node* parent, Node* child);
    static Node* rotate_right(Node* parent, Node* child);
    static Node* rotate_right_left(Node* parent, Node* child);
    static Node* rotate_left_right(Node* parent, Node* child);
};

template <typename Node, typename Compare>
Node* AVL<Node, Compare>::insert(Node* node) {
    auto new_node = this->template BST<Node, Compare>::insert(node);
    if (new_node != node) {
        return new_node;
    }
    node->balance_factor = 0;
    for (auto probe = node; probe != nullptr; probe = probe->parent) {
        auto parent = probe->parent;
        if (parent == nullptr) {
            break;
        }
        auto grand_parent = parent->parent;
        Node* new_subroot;
        if (probe == parent->left) {
            if (parent->balance_factor < 0) {
                if (probe->balance_factor > 0) {
                    new_subroot = rotate_left_right(parent, probe);
                }
                else {
                    new_subroot = rotate_right(parent, probe);
                }
            }
            else {
                if (parent->balance_factor > 0) {
                    parent->balance_factor = 0;
                    break;
                }
                else {
                    parent->balance_factor = -1;
                    continue;
                }
            }
        }
        else {
            if (parent->balance_factor > 0) {
                if (probe->balance_factor < 0) {
                    new_subroot = rotate_right_left(parent, probe);
                }
                else {
                    new_subroot = rotate_left(parent, probe);
                }
            }
            else {
                if (parent->balance_factor < 0) {
                    parent->balance_factor = 0;
                    break;
                }
                else {
                    parent->balance_factor = 1;
                    continue;
                }
            }
        }
        if (grand_parent != nullptr) {
            if (parent == grand_parent->left) {
                link_left(grand_parent, new_subroot);
            }
            else {
                link_right(grand_parent, new_subroot);
            }
        }
        else {
            new_subroot->parent = nullptr;
            _root = new_subroot;
        }
        break;
    }
    return node;
}

template <typename Node, typename Compare>
Node* AVL<Node, Compare>::remove(Node* node) {
    auto remove_report = this->template BST<Node, Compare>::remove(node);
    auto new_subroot = remove_report.subtree_child;
    auto parent = remove_report.subtree_parent;

    // while (parent != nullptr) {
    for (; parent != nullptr; parent = parent->parent) {
        int sibling_balance_factor;
        auto grand_parent = parent->parent;
        if (new_subroot == parent->left) {
            auto sibling = parent->right;
            if (parent->balance_factor > 0 && sibling != nullptr) {
                sibling_balance_factor = sibling->balance_factor;
                if (sibling_balance_factor < 0) {
                    new_subroot = rotate_right_left(parent, sibling);
                }
                else {
                    new_subroot = rotate_left(parent, sibling);
                }
            }
            else {
                if (parent->balance_factor == 0) {
                    parent->balance_factor = 1;
                    break;
                }
                new_subroot = parent;
                new_subroot->balance_factor = 0;
                // parent = grand_parent;
                continue;
            }
        }
        else {
            auto sibling = parent->left;
            if (parent->balance_factor < 0 && sibling != nullptr) {
                sibling_balance_factor = sibling->balance_factor;
                if (sibling_balance_factor > 0) {
                    new_subroot = rotate_left_right(parent, sibling);
                }
                else {
                    new_subroot = rotate_right(parent, sibling);
                }
            }
            else {
                if (parent->balance_factor == 0) {
                    parent->balance_factor = -1;
                    break;
                }
                new_subroot = parent;
                new_subroot->balance_factor = 0;
                // parent = grand_parent;
                continue;
            }
        }
        if (grand_parent != nullptr) {
            if (parent == grand_parent->left) {
                link_left(grand_parent, new_subroot);
            }
            else {
                link_right(grand_parent, new_subroot);
            }
        }
        else {
            new_subroot->parent = nullptr;
            _root = new_subroot;
        }
        if (sibling_balance_factor == 0) {
            break;
        }
        // parent = grand_parent;
    }
    return node;
}

template <typename Node, typename Compare>
typename AVL<Node, Compare>::SplitResult AVL<Node, Compare>::split() {
    if (_root == nullptr) {
        return {AVL(_cmp), AVL(_cmp)};
    }

    SplitResult split_result {AVL(_root->left, _cmp), AVL(_root->right, _cmp)};
    split_result.left.insert(_root);

    _root = nullptr;

    return split_result;
}

template <typename Node, typename Compare>
AVL<Node, Compare> AVL<Node, Compare>::merge(AVL& left, AVL& right) {
    auto new_subroot = right.leftmost(right._root);
    right.remove(new_subroot);
    if ((left.height() >= right.height() - 1) && (left.height() <= right.height() + 1)) {
        link_left(new_subroot, left._root);
        link_right(new_subroot, right._root);
        update_size(new_subroot);
        update_height(new_subroot);
        new_subroot->balance_factor = right.height() - left.height();
        left._root = nullptr;
        right._root = nullptr;
        return AVL(new_subroot, _cmp);
    }
    if (left.height() > right.height()) {
        auto probe = left._root;
        while (probe->height > right.height()) {
            probe = probe->right;
        }
        auto parent = probe->parent;
        link_left(new_subroot, probe);
        link_right(new_subroot, right._root);
        link_right(parent, new_subroot);
        update_size_path(new_subroot);
        // ...
        right._root = nullptr;
        return left;
    }
    else {
        // ...
        return right;
    }
}

template <typename Node, typename Compare>
Node* AVL<Node, Compare>::rotate_left(Node* parent, Node* child) {
    link_right(parent, child->left);
    link_left(child, parent);

    update_size(parent);
    update_size(child);
#ifdef WITH_HEIGHT
    update_height(parent);
    update_height(child);
#endif

    if (child->balance_factor == 0) {
        parent->balance_factor = 1;
        child->balance_factor = -1;
    }
    else {
        parent->balance_factor = 0;
        child->balance_factor = 0;
    }

    return child;
}

template <typename Node, typename Compare>
Node* AVL<Node, Compare>::rotate_right(Node* parent, Node* child) {
    link_left(parent, child->right);
    link_right(child, parent);

    update_size(parent);
    update_size(child);
#ifdef WITH_HEIGHT
    update_height(parent);
    update_height(child);
#endif

    if (child->balance_factor == 0) {
        parent->balance_factor = -1;
        child->balance_factor = 1;
    }
    else {
        parent->balance_factor = 0;
        child->balance_factor = 0;
    }

    return child;
}

template <typename Node, typename Compare>
Node* AVL<Node, Compare>::rotate_right_left(Node* parent, Node* child) {
    auto grand_child = child->left;

    link_left(child, grand_child->right);
    link_right(grand_child, child);
    link_right(parent, grand_child->left);
    link_left(grand_child, parent);

    update_size(parent);
    update_size(child);
    update_size(grand_child);
#ifdef WITH_HEIGHT
    update_height(parent);
    update_height(child);
    update_height(grand_child);
#endif

    if (grand_child->balance_factor == 0) {
        parent->balance_factor = 0;
        child->balance_factor = 0;
    }
    else if (grand_child->balance_factor > 0) {
        parent->balance_factor = -1;
        child->balance_factor = 0;
    }
    else {
        parent->balance_factor = 0;
        child->balance_factor = 1;
    }
    grand_child->balance_factor = 0;

    return grand_child;
}

template <typename Node, typename Compare>
Node* AVL<Node, Compare>::rotate_left_right(Node* parent, Node* child) {
    auto grand_child = child->right;

    link_right(child, grand_child->left);
    link_left(grand_child, child);
    link_left(parent, grand_child->right);
    link_right(grand_child, parent);

    update_size(parent);
    update_size(child);
    update_size(grand_child);
#ifdef WITH_HEIGHT
    update_height(parent);
    update_height(child);
    update_height(grand_child);
#endif

    if (grand_child->balance_factor == 0) {
        parent->balance_factor = 0;
        child->balance_factor = 0;
    }
    else if (grand_child->balance_factor > 0) {
        parent->balance_factor = 0;
        child->balance_factor = -1;
    }
    else {
        parent->balance_factor = 1;
        child->balance_factor = 0;
    }
    grand_child->balance_factor = 0;

    return grand_child;
}

}

#endif
