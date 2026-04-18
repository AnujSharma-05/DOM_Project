#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct StringHash {
    using is_transparent = void;

    std::size_t operator()(std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
};

enum class DirtyState {
    CLEAN = 0,
    PAINT_DIRTY = 1,
    LAYOUT_DIRTY = 2,
};

class Node : public std::enable_shared_from_this<Node> {
public:
    using IdChangeCallback = std::function<void(std::string_view old_id, std::string_view new_id)>;

    /// Construct a node with a tag/type name and optional id-change callback.
    /// Preconditions: `type` should not be empty for meaningful rendering/debug output.
    /// Postconditions: The node starts detached, CLEAN, and with zeroed geometry.
    /// Exceptions: May throw allocation-related exceptions.
    explicit Node(std::string type, IdChangeCallback id_change_callback = {});

    /// Attach `child` to this node, reparenting from its old parent if needed.
    /// Preconditions: `child` is non-null, not `this`, and does not introduce an ancestor cycle.
    /// Postconditions: `child->parent() == this`; dirty state escalates to LAYOUT_DIRTY.
    /// Exceptions: Throws std::invalid_argument when preconditions are violated.
    void add_child(const std::shared_ptr<Node>& child);

    /// Detach `child` from this node if present.
    /// Preconditions: None.
    /// Postconditions: Returns true only when `child` was an immediate child; on success child parent is cleared and layout dirtied.
    /// Exceptions: Does not throw.
    bool remove_child(const std::shared_ptr<Node>& child);

    /// Update local layout offset of this node.
    /// Preconditions: None.
    /// Postconditions: `x()` and `y()` reflect provided values; dirty state may escalate to LAYOUT_DIRTY.
    /// Exceptions: Does not throw.
    void set_position(int x, int y);

    /// Update local size of this node.
    /// Preconditions: None.
    /// Postconditions: `width()` and `height()` reflect provided values; dirty state may escalate to LAYOUT_DIRTY.
    /// Exceptions: Does not throw.
    void set_size(int width, int height);

    /// Set or replace an attribute value.
    /// Preconditions: None.
    /// Postconditions: Attribute map contains `key=value`; dirty state may escalate to PAINT_DIRTY; id callback fires when key is "id".
    /// Exceptions: May throw allocation-related exceptions.
    void set_attribute(std::string key, std::string value);

    /// Read an attribute value by key.
    /// Preconditions: None.
    /// Postconditions: Returns empty string when the key is absent.
    /// Exceptions: Does not throw.
    std::string get_attribute(std::string_view key) const;

    /// Check whether an attribute exists.
    /// Preconditions: None.
    /// Postconditions: Returns true if key exists in the attribute map.
    /// Exceptions: Does not throw.
    bool has_attribute(std::string_view key) const;

    /// Escalate dirty state for this node and propagate descendant-dirty status upward.
    /// Preconditions: None.
    /// Postconditions: Dirty rank is monotonic (never downgraded by this call).
    /// Exceptions: Does not throw.
    void mark_dirty(DirtyState state);

    /// Clear dirty state for this node and all descendants.
    /// Preconditions: None.
    /// Postconditions: Entire subtree rooted at this node becomes CLEAN with no dirty-descendant flags.
    /// Exceptions: Does not throw.
    void mark_clean();

    /// Return current dirty state.
    /// Preconditions: None.
    /// Postconditions: Does not modify state.
    /// Exceptions: Does not throw.
    DirtyState dirty_state() const;

    /// Return whether any descendant is marked dirty.
    /// Preconditions: None.
    /// Postconditions: Does not modify state.
    /// Exceptions: Does not throw.
    bool has_dirty_descendant() const;

    /// Return node type/tag name.
    /// Preconditions: None.
    /// Postconditions: Reference remains valid for node lifetime.
    /// Exceptions: Does not throw.
    const std::string& type() const;

    /// Return full attribute map for read-only traversal/serialization.
    /// Preconditions: None.
    /// Postconditions: Reference remains valid until node mutation.
    /// Exceptions: Does not throw.
    const std::unordered_map<std::string, std::string, StringHash, std::equal_to<>>& attributes() const { return attributes_; }

    /// Return `id` attribute value, or empty string if missing.
    /// Preconditions: None.
    /// Postconditions: Does not modify state.
    /// Exceptions: Does not throw.
    std::string get_id() const;

    /// Return parent node if attached.
    /// Preconditions: None.
    /// Postconditions: Returns null when detached.
    /// Exceptions: Does not throw.
    std::shared_ptr<Node> parent() const;

    /// Return direct children in insertion order.
    /// Preconditions: None.
    /// Postconditions: Reference remains valid until structural mutation.
    /// Exceptions: Does not throw.
    const std::vector<std::shared_ptr<Node>>& children() const;

    /// Return local x offset relative to parent.
    /// Preconditions: None.
    /// Postconditions: Does not modify state.
    /// Exceptions: Does not throw.
    int x() const;

    /// Return local y offset relative to parent.
    /// Preconditions: None.
    /// Postconditions: Does not modify state.
    /// Exceptions: Does not throw.
    int y() const;

    /// Return local width.
    /// Preconditions: None.
    /// Postconditions: Does not modify state.
    /// Exceptions: Does not throw.
    int width() const;

    /// Return local height.
    /// Preconditions: None.
    /// Postconditions: Does not modify state.
    /// Exceptions: Does not throw.
    int height() const;

    /// Return computed absolute x position written by LayoutEngine.
    /// Preconditions: Layout pass should have run for meaningful values.
    /// Postconditions: Does not modify state.
    /// Exceptions: Does not throw.
    int abs_x() const;

    /// Return computed absolute y position written by LayoutEngine.
    /// Preconditions: Layout pass should have run for meaningful values.
    /// Postconditions: Does not modify state.
    /// Exceptions: Does not throw.
    int abs_y() const;

    // Query selectors: tag name ("div"), id ("#board"), attribute ("[piece]"), attribute value ("[piece=K]")
    /// Return first node matching a supported selector using depth-first search.
    /// Preconditions: Selector must be one of: tag, #id, [attr], [attr=value].
    /// Postconditions: Returns null when no match or selector is invalid.
    /// Exceptions: May throw std::bad_weak_ptr if called on an object not owned by std::shared_ptr.
    std::shared_ptr<Node> querySelector(std::string_view selector) const;

    /// Return all nodes matching a supported selector in depth-first order.
    /// Preconditions: Selector must be one of: tag, #id, [attr], [attr=value].
    /// Postconditions: Returns empty list for invalid selectors or no matches.
    /// Exceptions: May throw std::bad_weak_ptr if called on an object not owned by std::shared_ptr.
    std::vector<std::shared_ptr<Node>> querySelectorAll(std::string_view selector) const;

#ifndef NDEBUG
    /// Validate subtree invariants in debug builds.
    /// Preconditions: None.
    /// Postconditions: Returns true only when parent links, uniqueness, and cycle constraints hold.
    /// Exceptions: Does not throw.
    bool debug_validate_subtree() const;
#endif

private:
    friend class LayoutEngine;

    bool is_ancestor_of(const Node* maybe_descendant) const;
    bool remove_direct_child(const Node* child_ptr);
    void mark_descendant_dirty();
    void apply_layout(int abs_x, int abs_y);

    std::string type_;
    std::unordered_map<std::string, std::string, StringHash, std::equal_to<>> attributes_;

    std::vector<std::shared_ptr<Node>> children_;
    std::weak_ptr<Node> parent_;

    int x_{0};
    int y_{0};
    int width_{0};
    int height_{0};
    int abs_x_{0};
    int abs_y_{0};

    DirtyState state_{DirtyState::CLEAN};
    bool has_dirty_descendant_{false};
    IdChangeCallback id_change_callback_;
};
