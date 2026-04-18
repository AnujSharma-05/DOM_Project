#pragma once

#include "Node.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

class Document {
public:
    /// Construct an empty document.
    /// Preconditions: None.
    /// Postconditions: No root and empty id index.
    /// Exceptions: Does not throw.
    Document() = default;

    /// Create a detached element node associated with this document's id-index callback.
    /// Preconditions: `tag` should be non-empty for meaningful DOM semantics.
    /// Postconditions: Returned node is detached and can be inserted into a tree.
    /// Exceptions: May throw allocation-related exceptions.
    std::shared_ptr<Node> create_element(std::string_view tag);

    /// Set or replace the document root and rebuild id index from that subtree.
    /// Preconditions: None.
    /// Postconditions: `root()` equals provided node; id index is rebuilt.
    /// Exceptions: May throw allocation-related exceptions.
    void set_root(std::shared_ptr<Node> root);

    /// Return current root node.
    /// Preconditions: None.
    /// Postconditions: Returns null when no root was set.
    /// Exceptions: Does not throw.
    std::shared_ptr<Node> root() const;

    /// Return node by id in indexed O(1) average lookup time.
    /// Preconditions: None.
    /// Postconditions: Returns null when id is missing or weak entry expired.
    /// Exceptions: May throw allocation-related exceptions when materializing lookup key.
    std::shared_ptr<Node> get_by_id(std::string_view id) const;

    /// Debug counter for DFS lookups (kept for diagnostics).
    /// Preconditions: None.
    /// Postconditions: Does not modify state.
    /// Exceptions: Does not throw.
    std::size_t debug_dfs_lookups() const;

private:
    void index_subtree(const std::shared_ptr<Node>& node);
    void update_index(Node* node, std::string_view old_id, std::string_view new_id);

    std::shared_ptr<Node> root_;
    std::unordered_map<std::string, std::weak_ptr<Node>> id_index_;
    std::size_t dfs_lookups_{0};
};
