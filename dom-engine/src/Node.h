#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

enum class DirtyState {
    CLEAN = 0,
    PAINT_DIRTY = 1,
    LAYOUT_DIRTY = 2,
};

class Node : public std::enable_shared_from_this<Node> {
public:
    explicit Node(std::string type);

    void add_child(const std::shared_ptr<Node>& child);
    bool remove_child(const std::shared_ptr<Node>& child);

    void set_position(int x, int y);
    void set_size(int width, int height);

    void set_attribute(std::string key, std::string value);
    std::string get_attribute(std::string_view key) const;
    bool has_attribute(std::string_view key) const;

    void mark_dirty(DirtyState state);
    void mark_clean();
    DirtyState dirty_state() const;
    bool has_dirty_descendant() const;

    const std::string& type() const;
    std::string get_id() const;

    std::shared_ptr<Node> parent() const;
    const std::vector<std::shared_ptr<Node>>& children() const;

    int x() const;
    int y() const;
    int width() const;
    int height() const;

#ifndef NDEBUG
    bool debug_validate_subtree() const;
#endif

private:
    bool is_ancestor_of(const Node* maybe_descendant) const;
    bool remove_direct_child(const Node* child_ptr);
    void mark_descendant_dirty();

    std::string type_;
    std::unordered_map<std::string, std::string> attributes_;

    std::vector<std::shared_ptr<Node>> children_;
    std::weak_ptr<Node> parent_;

    int x_{0};
    int y_{0};
    int width_{0};
    int height_{0};

    DirtyState state_{DirtyState::CLEAN};
    bool has_dirty_descendant_{false};
};
