#ifndef _YFAST_IMPL_AVL_H
#define _YFAST_IMPL_AVL_H

#include <functional>
#include <iostream>

#include <yfast/impl/bst.h>
#include <__ostream/basic_ostream.h>

#define DEBUG

namespace yfast::impl {

template <typename Node, typename Compare = std::less<typename Node::Key>>
class AVL: public BST<Node, Compare> {
public:
    using typename BST<Node, Compare>::Key;

    typedef struct {
        AVL left;
        AVL right;
        Node* left_max;
    } SplitResult;

protected:
    using BST<Node, Compare>::_cmp;
    using BST<Node, Compare>::_root;

public:
    explicit AVL(Compare cmp = Compare()): AVL(nullptr, cmp) {}
    AVL(const AVL& other) = delete;
    AVL(AVL&& other) noexcept: BST<Node, Compare>(std::move(other)) {
#ifdef DEBUG
        asm("nop");
#endif
    }

    [[nodiscard]] unsigned int height() const { return height(_root); }

    Node* insert(Node* node);
    Node* remove(Node* node);

    SplitResult split();
    static AVL merge(AVL&& subtree1, AVL&& subtree2);

protected:
    explicit AVL(Node* root, Compare cmp = Compare()): BST<Node, Compare>(root, cmp) {}

    using BST<Node, Compare>::link_left;
    using BST<Node, Compare>::link_right;
    using BST<Node, Compare>::update_size;
    using BST<Node, Compare>::update_size_path;
    using BST<Node, Compare>::_leftmost;
    using BST<Node, Compare>::_rightmost;

    static void update_balance_factor(Node* node);

private:
    static unsigned int height(const Node* node);

    static Node* rotate_left(Node* parent, Node* child);
    static Node* rotate_right(Node* parent, Node* child);
    static Node* rotate_right_left(Node* parent, Node* child);
    static Node* rotate_left_right(Node* parent, Node* child);

    static Node* join_left(Node* left, Node* subroot, Node* right);
    static Node* join_right(Node* left, Node* subroot, Node* right);
    static Node* rebalance(Node* node);
#ifdef DEBUG
    static void check_balance(const Node* node) {
        if (node != nullptr) {
            if (node->balance_factor > 1) {
                asm("nop");
            }
            if (node->balance_factor < -1) {
                asm("nop");
            }
            if (node->balance_factor != height(node->right) - height(node->left)) {
                asm("nop");
            }
            check_balance(node->left);
            check_balance(node->right);
        }
    }
#endif
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
#ifdef DEBUG
    check_balance(_root);
    this->check_size(_root);
#endif
    auto remove_report = this->template BST<Node, Compare>::remove(node);
#ifdef DEBUG
    this->check_size(_root);
#endif
    auto substitution = remove_report.substitution;
    auto new_subroot = remove_report.subtree_child;
    auto parent = remove_report.subtree_parent;
    bool is_left_child = remove_report.is_left_child;
#ifdef DEBUG
    Node* sibling;
#endif

    // if (substitution != nullptr) {
    //     substitution->balance_factor = node->balance_factor;
    // }

#define WHILE_OVER_FOR 1
#if WHILE_OVER_FOR
    while (parent != nullptr) {
#else
    for (; parent != nullptr; parent = parent->parent) {
#endif
        int sibling_balance_factor;
        auto grand_parent = parent->parent;
#if 1
        if (new_subroot != nullptr) {
            is_left_child = (new_subroot == parent->left);
        }
        if (is_left_child) {
#else
        if (new_subroot == parent->left) {
#endif
#ifdef DEBUG
            sibling = parent->right;
#else
            auto sibling = parent->right;
#endif
#define CHECK_SIBLING 0
#if CHECK_SIBLING
            if (parent->balance_factor > 0 && sibling != nullptr) {
#else
            if (parent->balance_factor > 0) {
#ifdef DEBUG
                if (sibling == nullptr) {
                    asm("nop");
                }
#endif
#endif
                sibling_balance_factor = sibling->balance_factor;
                if (sibling_balance_factor < 0) {
                    new_subroot = rotate_right_left(parent, sibling);
                }
                else {
                    new_subroot = rotate_left(parent, sibling);
                }
#ifdef DEBUG
                this->check_size(_root);
                check_balance(parent);
#endif
            }
            else {
                if (parent->balance_factor == 0) {
                    parent->balance_factor = 1;
#ifdef DEBUG
                    check_balance(parent);
#endif
                    break;
                }
                new_subroot = parent;
                new_subroot->balance_factor = 0;
#ifdef DEBUG
                check_balance(new_subroot);
#endif
#if WHILE_OVER_FOR
                parent = grand_parent;
#endif
                continue;
            }
        }
        else {
#ifdef DEBUG
            sibling = parent->left;
#else
            auto sibling = parent->left;
#endif
#if CHECK_SIBLING
            if (parent->balance_factor < 0 && sibling != nullptr) {
#else
            if (parent->balance_factor < 0) {
#ifdef DEBUG
                if (sibling == nullptr) {
                    asm("nop");
                }
#endif
#endif
                sibling_balance_factor = sibling->balance_factor;
                if (sibling_balance_factor > 0) {
                    new_subroot = rotate_left_right(parent, sibling);
                }
                else {
                    new_subroot = rotate_right(parent, sibling);
                }
#ifdef DEBUG
                this->check_size(_root);
                check_balance(parent);
#endif
            }
            else {
                if (parent->balance_factor == 0) {
                    parent->balance_factor = -1;
#ifdef DEBUG
                    check_balance(parent);
#endif
                    break;
                }
                new_subroot = parent;
                new_subroot->balance_factor = 0;
#ifdef DEBUG
                check_balance(new_subroot);
#endif
#if WHILE_OVER_FOR
                parent = grand_parent;
#endif
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
#if WHILE_OVER_FOR
        parent = grand_parent;
#endif
    }
#ifdef DEBUG
    check_balance(_root);
#endif
    return node;
}

template <typename Node, typename Compare>
typename AVL<Node, Compare>::SplitResult AVL<Node, Compare>::split() {
    if (_root == nullptr) {
        return { AVL(_cmp), AVL(_cmp), nullptr };
    }

    SplitResult split_result { AVL(_root->left, _cmp), AVL(_root->right, _cmp), _root };
    split_result.left.insert(_root);

    _root = nullptr;

    return split_result;
}

template <typename Node, typename Compare>
AVL<Node, Compare> AVL<Node, Compare>::merge(AVL&& subtree1, AVL&& subtree2) {
    auto first_is_less = subtree1._cmp(subtree1._root->key, subtree2._root->key);
#if 1
    auto& left = first_is_less ? subtree1 : subtree2;
    auto& right = first_is_less ? subtree2 : subtree1;
#else
    auto left = first_is_less ? std::move(subtree1) : std::move(subtree2);
    auto right = first_is_less ? std::move(subtree2) : std::move(subtree1);
#endif
    auto new_subroot = AVL::_leftmost(right._root);
#ifdef DEBUG
    const auto left_size = left.size();
    const auto right_size = right.size();
    std::cerr << "new_subroot=" << new_subroot << std::endl;
#endif
    right.remove(new_subroot);
    new_subroot->parent = nullptr;
    const auto left_height = left.height();
    const auto right_height = right.height();
#ifdef DEBUG
    std::cerr << "left size=" << left.size() << " height=" << left_height << " right size=" << right.size() << " height=" << right_height << std::endl;
#endif
#if 1
    if ((left_height + 1 >= right_height) && (left_height <= right_height + 1)) {
        link_left(new_subroot, left._root);
        link_right(new_subroot, right._root);
        update_size(new_subroot);
        new_subroot->balance_factor = right_height - left_height;
        left._root = nullptr;
        right._root = nullptr;
        return AVL(new_subroot, left._cmp);
    }
#define LN2_PATH 0
    if (left_height > right_height) {
#if LN2_PATH
        auto probe = left._root;
        int probe_height;

        do {
            probe = probe->right;
            probe_height = height(probe);
        }
        while (probe_height > right_height);  // FIXME: ln^2
#else
        auto probe = _rightmost(left._root);
        int probe_height = height(probe);
        while (probe_height < right_height) {
            probe = probe->parent;
            if (probe->balance_factor < 0) {
                probe_height += 2;
            }
            else {
                ++probe_height;
            }
        }
#endif

        auto parent = probe->parent;
        link_left(new_subroot, probe);
        link_right(new_subroot, right._root);
        link_right(parent, new_subroot);
        update_size_path(new_subroot);
        new_subroot->balance_factor = right_height - probe_height;
#ifdef DEBUG
        if (left.size() != left_size + right_size) {
            asm("nop");
        }
        check_balance(new_subroot);
#endif

        while (parent != nullptr) {
            auto grand_parent = parent->parent;
#ifdef DEBUG
            Node* node = nullptr;
#endif
            if (parent->balance_factor > 0) {
                if (new_subroot->balance_factor < 0) {
// #ifdef DEBUG
//                     node = rotate_right(new_subroot, new_subroot->left);
// #else
//                     auto node = rotate_right(new_subroot, new_subroot->left);
// #endif
//                     link_right(parent, node);
//                     rotate_left(parent, node);
#ifdef DEBUG
                    node = rotate_right_left(parent, new_subroot);
#else
                    auto node = rotate_right_left(parent, new_subroot);
#endif
                    if (grand_parent != nullptr) {
                        link_right(grand_parent, node);
                    }
                    else {
                        node->parent = nullptr;
                        left._root = node;
                    }
                    if (node->balance_factor == 0) {  // FIXME: always true?
                        break;
                    }
                    new_subroot = node;
                    parent = grand_parent;
                }
                else {
#ifdef DEBUG
                    node = rotate_left(parent, new_subroot);
#else
                    auto node = rotate_left(parent, new_subroot);
#endif
                    if (grand_parent != nullptr) {
                        link_right(grand_parent, node);
                    }
                    else {
                        node->parent = nullptr;
                        left._root = node;
                    }
                    if (node->balance_factor == 0) {
                        break;
                    }
                    new_subroot = node;  // FIXME: redundant
                    parent = grand_parent;
                }
            }
            else {
                ++(parent->balance_factor);
#ifdef DEBUG
                std::cerr << "parent->balance_factor=" << parent->balance_factor << std::endl;
#endif
                if (parent->balance_factor == 0) {
                    break;
                }
                new_subroot = parent;
                parent = grand_parent;
            }
#ifdef DEBUG
            std::cerr << "new_subroot=" << new_subroot << std::endl;
            check_balance(new_subroot);
            if (left.size() != left_size + right_size) {
                asm("nop");
            }
#endif
        }

#ifdef DEBUG
        check_balance(left._root);
#endif
        right._root = nullptr;
#ifdef DEBUG
        if (left.size() != left_size + right_size) {
            asm("nop");
        }
#endif
        return std::move(left);
    }
    else {
#if LN2_PATH
        auto probe = right._root;
        int probe_height;

        do {
            probe = probe->left;
            probe_height = height(probe);
        }
        while (probe_height > left_height);  // FIXME: ln^2
#else
        auto probe = _leftmost(right._root);
        int probe_height = height(probe);
        while (probe_height < left_height) {
            probe = probe->parent;
            if (probe->balance_factor > 0) {
                probe_height += 2;
            }
            else {
                ++probe_height;
            }
        }
#endif

        auto parent = probe->parent;
        link_left(new_subroot, left._root);
        link_right(new_subroot, probe);
        link_left(parent, new_subroot);
        update_size_path(new_subroot);
        new_subroot->balance_factor = probe_height - left_height;
#ifdef DEBUG
        if (right.size() != left_size + right_size) {
            asm("nop");
        }
        check_balance(new_subroot);
#endif

        while (parent != nullptr) {
            auto grand_parent = parent->parent;
#ifdef DEBUG
            Node* node = nullptr;
#endif
            if (parent->balance_factor < 0) {
                if (new_subroot->balance_factor > 0) {
// #ifdef DEBUG
//                     node = rotate_left(new_subroot, new_subroot->right);
// #else
//                     auto node = rotate_left(new_subroot, new_subroot->right);
// #endif
//                     link_left(parent, node);
//                     rotate_right(parent, node);
#ifdef DEBUG
                    node = rotate_left_right(parent, new_subroot);
#else
                    auto node = rotate_left_right(parent, new_subroot);
#endif
                    if (grand_parent != nullptr) {
                        link_left(grand_parent, node);
                    }
                    else {
                        node->parent = nullptr;
                        right._root = node;
                    }
                    if (node->balance_factor == 0) {
                        break;
                    }
                    new_subroot = node;
                    parent = grand_parent;
                }
                else {
#ifdef DEBUG
                    node = rotate_right(parent, new_subroot);
#else
                    auto node = rotate_right(parent, new_subroot);
#endif
                    if (grand_parent != nullptr) {
                        link_left(grand_parent, node);
                    }
                    else {
                        node->parent = nullptr;
                        right._root = node;
                    }
                    if (node->balance_factor == 0) {
                        break;
                    }
                    new_subroot = node;  // FIXME: redundant
                    parent = grand_parent;
                }
            }
            else {
                --(parent->balance_factor);
#ifdef DEBUG
                std::cerr << "parent->balance_factor=" << parent->balance_factor << std::endl;
#endif
                if (parent->balance_factor == 0) {
                    break;
                }
                new_subroot = parent;
                parent = grand_parent;
            }
#ifdef DEBUG
            std::cerr << "new_subroot=" << new_subroot << std::endl;
            check_balance(new_subroot);
            if (right.size() != left_size + right_size) {
                asm("nop");
            }
#endif
        }

#ifdef DEBUG
        check_balance(right._root);
#endif
        left._root = nullptr;
#ifdef DEBUG
        if (right.size() != left_size + right_size) {
            asm("nop");
        }
#endif
        return std::move(right);
    }
#else
#ifdef DEBUG
    check_balance(left._root);
    check_balance(right._root);
#endif
    if (left_height > right_height + 1) {
        auto new_root = join_right(left._root, new_subroot, right._root);
#ifdef DEBUG
        new_root->parent = nullptr;
        AVL::check_size(new_root);
        check_balance(new_root);
#endif
        left._root = nullptr;
        right._root = nullptr;
        return AVL(new_root, left._cmp);
    }
    if (right_height > left_height + 1) {
        auto new_root = join_left(left._root, new_subroot, right._root);
#ifdef DEBUG
        new_root->parent = nullptr;
        AVL::check_size(new_root);
        check_balance(new_root);
#endif
        left._root = nullptr;
        right._root = nullptr;
        return AVL(new_root, left._cmp);
    }
    link_left(new_subroot, left._root);
    link_right(new_subroot, right._root);
    update_size(new_subroot);
    new_subroot->balance_factor = right_height - left_height;
#ifdef DEBUG
    check_balance(new_subroot);
#endif
    left._root = nullptr;
    right._root = nullptr;
    return AVL(new_subroot, left._cmp);
#endif
}

template <typename Node, typename Compare>
void AVL<Node, Compare>::update_balance_factor(Node* node) {
    if (node != nullptr) {
        node->balance_factor = height(node->right) - height(node->left);
    }
}

template <typename Node, typename Compare>
unsigned int AVL<Node, Compare>::height(const Node* node) {
    if (node == nullptr) {
        return 0;
    }
    if (node->balance_factor > 0) {
        return 2 + height(node->left);
    }
    if (node->balance_factor < 0) {
        return 2 + height(node->right);
    }
    return 1 + height(node->right);
}

template <typename Node, typename Compare>
Node* AVL<Node, Compare>::rotate_left(Node* parent, Node* child) {
    link_right(parent, child->left);
    link_left(child, parent);

    update_size(parent);
    update_size(child);

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

#define WIKI 0

#if WIKI
template <typename Node, typename Compare>
Node* AVL<Node, Compare>::join_left(Node* left, Node* subroot, Node* right) {
    if (height(right->left) <= height(left) + 1) {
        link_right(subroot, right->left);
        link_left(subroot, left);
        update_size(subroot);
        subroot->balance_factor = height(subroot->right) - height(subroot->left);
        if (height(subroot) <= height(right->right) + 1) {
            link_left(right, subroot);
            update_size(right);
            right->balance_factor = height(right->right) - height(right->left);
            return right;
        }
        else {
            auto node = rotate_left(subroot, subroot->right);
            link_left(right, node);
            update_size(right);
            right->balance_factor = height(right->right) - height(right->left);
            return rotate_right(right, node);
        }
    }
    else {
        auto subtree = join_left(left, subroot, right->left);
        link_left(right, subtree);
        update_size(right);
        right->balance_factor = height(right->right) - height(right->left);
        if (height(subtree) <= height(right->right) + 1) {
            return right;
        }
        else {
            return rotate_right(right, subtree);
        }
    }
}

template <typename Node, typename Compare>
Node* AVL<Node, Compare>::join_right(Node* left, Node* subroot, Node* right) {
    if (height(left->right) <= height(right) + 1) {
        link_left(subroot, left->right);
        link_right(subroot, right);
        update_size(subroot);
        subroot->balance_factor = height(subroot->right) - height(subroot->left);
        if (height(subroot) <= height(left->left) + 1) {
#ifdef DEBUG
            std::cerr << "height(subroot)=" << height(subroot) << " height(left->left)=" << height(left->left) << std::endl;
#endif
            link_right(left, subroot);
            update_size(left);
            left->balance_factor = height(left->right) - height(left->left);
#ifdef DEBUG
            check_balance(left);
#endif
            return left;
        }
        else {
            auto node = rotate_right(subroot, subroot->left);
            link_right(left, node);
            update_size(left);
            left->balance_factor = height(left->right) - height(left->left);
#ifdef DEBUG
            check_balance(left);
            auto result = rotate_left(left, node);
            check_balance(result);
            return result;
#else
            return rotate_left(left, node);
#endif
        }
    }
    else {
        auto subtree = join_right(left->right, subroot, right);
        link_right(left, subtree);
        update_size(left);
        left->balance_factor = height(left->right) - height(left->left);
        if (height(subtree) <= height(left->left) + 1) {
#ifdef DEBUG
            check_balance(left);
#endif
            return left;
        }
        else {
#ifdef DEBUG
            auto result = rotate_left(left, subtree);
            check_balance(result);
            return result;
#else
            return rotate_left(left, subtree);
#endif
        }
    }
}
#else
template <typename Node, typename Compare>
    Node* AVL<Node, Compare>::join_left(Node* left, Node* subroot, Node* right) {
    if (height(right->left) <= height(left) + 1) {
        link_left(subroot, left);
        link_right(subroot, right->left);
        update_size(subroot);
        link_left(right, rebalance(subroot));
        return rebalance(right);
    }
    else {
        auto subtree = join_left(left, subroot, right->left);
        link_left(right, subtree);
        update_size(right);
        return rebalance(right);
    }
}

template <typename Node, typename Compare>
Node* AVL<Node, Compare>::join_right(Node* left, Node* subroot, Node* right) {
    if (height(left->right) <= height(right) + 1) {
        link_left(subroot, left->right);
        link_right(subroot, right);
        update_size(subroot);
        link_right(left, rebalance(subroot));
        return rebalance(left);
    }
    else {
        auto subtree = join_right(left->right, subroot, right);
        link_right(left, subtree);
        update_size(left);
        return rebalance(left);
    }
}

template <typename Node, typename Compare>
Node* AVL<Node, Compare>::rebalance(Node* node) {
    Node* new_subroot = node;
    node->balance_factor = height(node->right) - height(node->left);
    if (node->balance_factor > 1) {
        if (node->right->balance_factor < 0) {
            new_subroot = rotate_right_left(node, node->right);
        }
        else {
            new_subroot = rotate_left(node, node->right);
        }
    }
    if (node->balance_factor < -1) {
        if (node->left->balance_factor > 0) {
            new_subroot = rotate_left_right(node, node->left);
        }
        else {
            new_subroot = rotate_right(node, node->left);
        }
    }
    return new_subroot;
}
#endif

}

#endif
