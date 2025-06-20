#ifndef _YFAST_AVL_H
#define _YFAST_AVL_H

#include <concepts>
#include <functional>

#include <bst.h>

namespace yfast {

template <typename Node>
concept SelfBalancedNodeGeneric = NodeGeneric<Node> && requires (Node node) {
    { node.balance_factor } -> std::convertible_to<int>;
};

template <SelfBalancedNodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq = std::equal_to<typename _Node::Key>, CompareGeneric<typename _Node::Key> _Compare = std::less<typename _Node::Key>>
class avl: public bst<_Node, _Eq, _Compare> {
public:
    using typename bst<_Node, _Eq, _Compare>::Node;
    using typename bst<_Node, _Eq, _Compare>::Key;
    using typename bst<_Node, _Eq, _Compare>::Eq;
    using typename bst<_Node, _Eq, _Compare>::Compare;

    typedef struct {
        avl left;
        avl right;
    } SplitResult;

protected:
    using bst<_Node, _Eq, _Compare>::_eq;
    using bst<_Node, _Eq, _Compare>::_cmp;
    using bst<_Node, _Eq, _Compare>::_root;

public:
    explicit avl(Eq eq = Eq(), Compare cmp = Compare()): avl(nullptr, eq, cmp) {}

    Node* insert(Node* node);
    Node* remove(Node* node);

    SplitResult split();  // TODO: move to 'bsl'

protected:
    explicit avl(Node* root, Eq eq = Eq(), Compare cmp = Compare()): bst<_Node, _Eq, _Compare>(root, eq, cmp) {}

    using bst<_Node, _Eq, _Compare>::link_left;
    using bst<_Node, _Eq, _Compare>::link_right;
    using bst<_Node, _Eq, _Compare>::update_size;

private:
    static Node* rotate_left(Node* parent, Node* child);
    static Node* rotate_right(Node* parent, Node* child);
    static Node* rotate_right_left(Node* parent, Node* child);
    static Node* rotate_left_right(Node* parent, Node* child);
};

template <SelfBalancedNodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename avl<_Node, _Eq, _Compare>::Node* avl<_Node, _Eq, _Compare>::insert(Node* node) {
    auto new_node = this->template bst<_Node, _Eq, _Compare>::insert(node);
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
        auto left_path = (probe == parent->left);
        if (left_path) {
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
            if (left_path) {
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

template <SelfBalancedNodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename avl<_Node, _Eq, _Compare>::Node* avl<_Node, _Eq, _Compare>::remove(Node* node) {
    auto remove_report = this->template bst<_Node, _Eq, _Compare>::remove(node);
    auto new_subroot = remove_report.subtree_child;
    auto parent = remove_report.subtree_parent;

    // while (parent != nullptr) {
    for (; parent != nullptr; parent = parent->parent) {
        int sibling_balance_factor;
        auto grand_parent = parent->parent;
        if (new_subroot == parent->left) {
            if (parent->balance_factor > 0) {
                auto sibling = parent->right;
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
            if (parent->balance_factor < 0) {
                auto sibling = parent->left;
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

template <SelfBalancedNodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename avl<_Node, _Eq, _Compare>::SplitResult avl<_Node, _Eq, _Compare>::split() {
    if (_root == nullptr) {
        return {avl(_eq, _cmp), avl(_eq, _cmp)};
    }

    SplitResult split_result {avl(_root->left, _eq, _cmp), avl(_root->right, _eq, _cmp)};
    split_result.left.insert(_root);

    _root = nullptr;

    return split_result;
}

template <SelfBalancedNodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename avl<_Node, _Eq, _Compare>::Node* avl<_Node, _Eq, _Compare>::rotate_left(Node* parent, Node* child) {
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

template <SelfBalancedNodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename avl<_Node, _Eq, _Compare>::Node* avl<_Node, _Eq, _Compare>::rotate_right(Node* parent, Node* child) {
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

template <SelfBalancedNodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename avl<_Node, _Eq, _Compare>::Node* avl<_Node, _Eq, _Compare>::rotate_right_left(Node* parent, Node* child) {
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

template <SelfBalancedNodeGeneric _Node, EqGeneric<typename _Node::Key> _Eq, CompareGeneric<typename _Node::Key> _Compare>
typename avl<_Node, _Eq, _Compare>::Node* avl<_Node, _Eq, _Compare>::rotate_left_right(Node* parent, Node* child) {
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

}

#endif
