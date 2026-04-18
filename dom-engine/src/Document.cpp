#include "Document.h"

#include <utility>

std::shared_ptr<Node> Document::create_element(std::string_view tag) {
    auto node_holder = std::make_shared<Node*>(nullptr);

    Node::IdChangeCallback callback = [this, node_holder](std::string_view old_id, std::string_view new_id) {
        if (*node_holder) {
            update_index(*node_holder, old_id, new_id);
        }
    };

    auto node = std::shared_ptr<Node>(new Node(std::string(tag), std::move(callback)));
    *node_holder = node.get();
    return node;
}

void Document::set_root(std::shared_ptr<Node> root) {
    root_ = std::move(root);
    id_index_.clear();
    if (root_) {
        index_subtree(root_);
    }
}

std::shared_ptr<Node> Document::root() const {
    return root_;
}

std::shared_ptr<Node> Document::get_by_id(std::string_view id) const {
    auto it = id_index_.find(std::string(id));
    if (it == id_index_.end()) {
        return nullptr;
    }

    auto node = it->second.lock();
    if (!node) {
        return nullptr;
    }

    return node;
}

std::size_t Document::debug_dfs_lookups() const {
    return dfs_lookups_;
}

void Document::index_subtree(const std::shared_ptr<Node>& node) {
    if (!node) {
        return;
    }

    const auto id = node->get_attribute("id");
    if (!id.empty()) {
        id_index_[id] = node;
    }

    for (const auto& child : node->children()) {
        index_subtree(child);
    }
}

void Document::update_index(Node* node, std::string_view old_id, std::string_view new_id) {
    if (!node) {
        return;
    }

    if (!old_id.empty()) {
        auto it = id_index_.find(std::string(old_id));
        if (it != id_index_.end()) {
            auto current = it->second.lock();
            if (!current || current.get() == node) {
                id_index_.erase(it);
            }
        }
    }

    if (!new_id.empty()) {
        id_index_[std::string(new_id)] = node->shared_from_this();
    }
}
