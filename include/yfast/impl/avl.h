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
        Node* left_max;
    } SplitResult;

protected:
    using BST<Node, Compare>::_cmp;
    using BST<Node, Compare>::_root;

public:
    explicit AVL(Compare cmp = Compare()): AVL(nullptr, cmp) {}
    AVL(const AVL& other) = delete;
    AVL(AVL&& other) noexcept: BST<Node, Compare>(std::move(other)) {}

    [[nodiscard]] unsigned int height() const { return height(_root); }

    /**
     * insert a new node, replacing node with the equal key (if any)
     * @param node node to insert
     * @return node being replaced or \a nullptr
     */
    Node* insert(Node* node) {
        auto replaced = this->template BST<Node, Compare>::insert(node);
        if (replaced != nullptr) {
            if (replaced->is_left_heavy()) {
                node->set_left_heavy();
            }
            else if (replaced->is_right_heavy()) {
                node->set_right_heavy();
            }
            else {
                node->set_balanced();
            }
            return replaced;
        }
        node->set_balanced();
        for (auto probe = node; probe != nullptr; probe = probe->parent) {
            auto parent = probe->parent;
            if (parent == nullptr) {
                break;
            }
            auto grand_parent = parent->parent;
            Node* new_subroot;
            if (probe == parent->left()) {
                if (parent->is_left_heavy()) {
                    if (probe->is_right_heavy()) {
                        new_subroot = rotate_left_right(parent, probe);
                    }
                    else {
                        new_subroot = rotate_right(parent, probe);
                    }
                }
                else {
                    if (parent->is_right_heavy()) {
                        parent->set_balanced();
                        break;
                    }
                    else {
                        parent->set_left_heavy();
                        continue;
                    }
                }
            }
            else {
                if (parent->is_right_heavy()) {
                    if (probe->is_left_heavy()) {
                        new_subroot = rotate_right_left(parent, probe);
                    }
                    else {
                        new_subroot = rotate_left(parent, probe);
                    }
                }
                else {
                    if (parent->is_left_heavy()) {
                        parent->set_balanced();
                        break;
                    }
                    else {
                        parent->set_right_heavy();
                        continue;
                    }
                }
            }
            if (grand_parent != nullptr) {
                if (parent == grand_parent->left()) {
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
        return nullptr;
    }

    Node* remove(Node* node) {
        auto remove_report = this->template BST<Node, Compare>::remove(node);
        auto substitution = remove_report.substitution;
        auto new_subroot = remove_report.subtree_child;
        auto parent = remove_report.subtree_parent;
        bool is_left_child = remove_report.is_left_child;

        if (substitution != new_subroot) {
            if (node->is_left_heavy()) {
                substitution->set_left_heavy();
            }
            else if (node->is_right_heavy()) {
                substitution->set_right_heavy();
            }
            else {
                substitution->set_balanced();
            }
        }

        while (parent != nullptr) {
            bool sibling_balanced;
            auto grand_parent = parent->parent;
            if (is_left_child) {
                auto sibling = parent->right();
                if (parent->is_right_heavy()) {
                    sibling_balanced = sibling->is_balanced();
                    if (sibling->is_left_heavy()) {
                        new_subroot = rotate_right_left(parent, sibling);
                    }
                    else {
                        new_subroot = rotate_left(parent, sibling);
                    }
                }
                else {
                    if (parent->is_balanced()) {
                        parent->set_right_heavy();
                        break;
                    }
                    new_subroot = parent;
                    new_subroot->set_balanced();
                    parent = grand_parent;
                    if (parent != nullptr) {
                        is_left_child = (new_subroot == parent->left());
                    }
                    continue;
                }
            }
            else {
                auto sibling = parent->left();
                if (parent->is_left_heavy()) {
                    sibling_balanced = sibling->is_balanced();
                    if (sibling->is_right_heavy()) {
                        new_subroot = rotate_left_right(parent, sibling);
                    }
                    else {
                        new_subroot = rotate_right(parent, sibling);
                    }
                }
                else {
                    if (parent->is_balanced()) {
                        parent->set_left_heavy();
                        break;
                    }
                    new_subroot = parent;
                    new_subroot->set_balanced();
                    parent = grand_parent;
                    if (parent != nullptr) {
                        is_left_child = (new_subroot == parent->left());
                    }
                    continue;
                }
            }
            if (grand_parent != nullptr) {
                if (parent == grand_parent->left()) {
                    link_left(grand_parent, new_subroot);
                    is_left_child = true;
                }
                else {
                    link_right(grand_parent, new_subroot);
                    is_left_child = false;
                }
            }
            else {
                new_subroot->parent = nullptr;
                _root = new_subroot;
            }
            if (sibling_balanced) {
                break;
            }
            parent = grand_parent;
        }
        return node;
    }

    SplitResult split() {
        if (_root == nullptr) {
            return { AVL(_cmp), AVL(_cmp), nullptr };
        }

        SplitResult split_result { AVL(_root->left(), _cmp), AVL(_root->right(), _cmp), _root };
        split_result.left.insert(_root);

        _root = nullptr;

        return split_result;
    }

    static AVL merge(AVL&& subtree1, AVL&& subtree2) {
        auto first_is_less = subtree1._cmp(subtree1._root->key, subtree2._root->key);
        auto& left = first_is_less ? subtree1 : subtree2;
        auto& right = first_is_less ? subtree2 : subtree1;
        auto new_subroot = AVL::_leftmost(right._root);
        right.remove(new_subroot);
        new_subroot->parent = nullptr;
        const auto left_height = left.height();
        const auto right_height = right.height();
        if (left_height > right_height + 1) {
            auto probe = _rightmost(left._root);
            int probe_height = height(probe);
            while (probe_height < right_height) {
                probe = probe->parent;
                if (probe->is_left_heavy()) {
                    probe_height += 2;
                }
                else {
                    ++probe_height;
                }
            }

            auto parent = probe->parent;
            link_left(new_subroot, probe);
            link_right(new_subroot, right._root);
            link_right(parent, new_subroot);
            update_size_path(new_subroot);
            if (right_height > probe_height) {
                new_subroot->set_right_heavy();
            }
            else if (right_height < probe_height) {
                new_subroot->set_left_heavy();
            }
            else {
                new_subroot->set_balanced();
            }

            while (parent != nullptr) {
                auto grand_parent = parent->parent;
                if (parent->is_right_heavy()) {
                    if (new_subroot->is_left_heavy()) {
                        auto node = rotate_right_left(parent, new_subroot);
                        if (grand_parent != nullptr) {
                            link_right(grand_parent, node);
                        }
                        else {
                            node->parent = nullptr;
                            left._root = node;
                        }
                        break;  // subroot is not balanced
                    }
                    else {
                        auto node = rotate_left(parent, new_subroot);
                        if (grand_parent != nullptr) {
                            link_right(grand_parent, node);
                        }
                        else {
                            node->parent = nullptr;
                            left._root = node;
                        }
                        if (node->is_balanced()) {
                            break;
                        }
                        // new_subroot = node; // redundant
                        parent = grand_parent;
                    }
                }
                else {
                    if (parent->is_left_heavy()) {
                        parent->set_balanced();
                        break;
                    }
                    parent->set_right_heavy();
                    new_subroot = parent;
                    parent = grand_parent;
                }
            }

            right._root = nullptr;
            return std::move(left);
        }
        if (right_height > left_height + 1) {
            auto probe = _leftmost(right._root);
            int probe_height = height(probe);
            while (probe_height < left_height) {
                probe = probe->parent;
                if (probe->is_right_heavy()) {
                    probe_height += 2;
                }
                else {
                    ++probe_height;
                }
            }

            auto parent = probe->parent;
            link_left(new_subroot, left._root);
            link_right(new_subroot, probe);
            link_left(parent, new_subroot);
            update_size_path(new_subroot);
            if (probe_height < left_height) {
                new_subroot->set_left_heavy();
            }
            else if (probe_height > left_height) {
                new_subroot->set_right_heavy();
            }
            else {
                new_subroot->set_balanced();
            }

            while (parent != nullptr) {
                auto grand_parent = parent->parent;
                if (parent->is_left_heavy()) {
                    if (new_subroot->is_right_heavy()) {
                        auto node = rotate_left_right(parent, new_subroot);
                        if (grand_parent != nullptr) {
                            link_left(grand_parent, node);
                        }
                        else {
                            node->parent = nullptr;
                            right._root = node;
                        }
                        break;  // subroot is not balanced
                    }
                    else {
                        auto node = rotate_right(parent, new_subroot);
                        if (grand_parent != nullptr) {
                            link_left(grand_parent, node);
                        }
                        else {
                            node->parent = nullptr;
                            right._root = node;
                        }
                        if (node->is_balanced()) {
                            break;
                        }
                        // new_subroot = node; // redundant
                        parent = grand_parent;
                    }
                }
                else {
                    if (parent->is_right_heavy()) {
                        parent->set_balanced();
                        break;
                    }
                    parent->set_left_heavy();
                    new_subroot = parent;
                    parent = grand_parent;
                }
            }

            left._root = nullptr;
            return std::move(right);
        }

        link_left(new_subroot, left._root);
        link_right(new_subroot, right._root);
        update_size(new_subroot);
        if (left_height > right_height) {
            new_subroot->set_left_heavy();
        }
        else if (right_height > left_height) {
            new_subroot->set_right_heavy();
        }
        else {
            new_subroot->set_balanced();
        }
        left._root = nullptr;
        right._root = nullptr;
        return AVL(new_subroot, left._cmp);
    }

protected:
    explicit AVL(Node* root, Compare cmp = Compare()): BST<Node, Compare>(root, cmp) {}

    using BST<Node, Compare>::link_left;
    using BST<Node, Compare>::link_right;
    using BST<Node, Compare>::update_size;
    using BST<Node, Compare>::update_size_path;
    using BST<Node, Compare>::_leftmost;
    using BST<Node, Compare>::_rightmost;

private:
    static unsigned int height(const Node* node) {
        if (node == nullptr) {
            return 0;
        }
        if (node->is_right_heavy()) {
            return 2 + height(node->left());
        }
        if (node->is_left_heavy()) {
            return 2 + height(node->right());
        }
        return 1 + height(node->right());
    }

    static Node* rotate_left(Node* parent, Node* child) {
        link_right(parent, child->left());
        link_left(child, parent);

        update_size(parent);
        update_size(child);

        if (child->is_balanced()) {
            parent->set_right_heavy();
            child->set_left_heavy();
        }
        else {
            parent->set_balanced();
            child->set_balanced();
        }

        return child;
    }

    static Node* rotate_right(Node* parent, Node* child) {
        link_left(parent, child->right());
        link_right(child, parent);

        update_size(parent);
        update_size(child);

        if (child->is_balanced()) {
            parent->set_left_heavy();
            child->set_right_heavy();
        }
        else {
            parent->set_balanced();
            child->set_balanced();
        }

        return child;
    }

    static Node* rotate_right_left(Node* parent, Node* child) {
        auto grand_child = child->left();

        link_left(child, grand_child->right());
        link_right(grand_child, child);
        link_right(parent, grand_child->left());
        link_left(grand_child, parent);

        update_size(parent);
        update_size(child);
        update_size(grand_child);

        if (grand_child->is_balanced()) {
            parent->set_balanced();
            child->set_balanced();
        }
        else if (grand_child->is_right_heavy()) {
            parent->set_left_heavy();
            child->set_balanced();
        }
        else {
            parent->set_balanced();
            child->set_right_heavy();
        }
        grand_child->set_balanced();

        return grand_child;
    }

    static Node* rotate_left_right(Node* parent, Node* child) {
        auto grand_child = child->right();

        link_right(child, grand_child->left());
        link_left(grand_child, child);
        link_left(parent, grand_child->right());
        link_right(grand_child, parent);

        update_size(parent);
        update_size(child);
        update_size(grand_child);

        if (grand_child->is_balanced()) {
            parent->set_balanced();
            child->set_balanced();
        }
        else if (grand_child->is_right_heavy()) {
            parent->set_balanced();
            child->set_left_heavy();
        }
        else {
            parent->set_right_heavy();
            child->set_balanced();
        }
        grand_child->set_balanced();

        return grand_child;
    }
};

}

#endif
