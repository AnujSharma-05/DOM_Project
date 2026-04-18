#pragma once

#include "Node.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

class Document {
public:
    Document() = default;

    std::shared_ptr<Node> create_element(std::string_view tag);
    void set_root(std::shared_ptr<Node> root);
    std::shared_ptr<Node> root() const;

    std::shared_ptr<Node> get_by_id(std::string_view id) const;
    std::size_t debug_dfs_lookups() const;

private:
    void index_subtree(const std::shared_ptr<Node>& node);
    void update_index(Node* node, std::string_view old_id, std::string_view new_id);

    std::shared_ptr<Node> root_;
    std::unordered_map<std::string, std::weak_ptr<Node>> id_index_;
    std::size_t dfs_lookups_{0};
};
