#include "Node.h"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

int dirty_rank(DirtyState state) {
    switch (state) {
        case DirtyState::CLEAN:
            return 0;
        case DirtyState::PAINT_DIRTY:
            return 1;
        case DirtyState::LAYOUT_DIRTY:
            return 2;
    }
    return 0;
}

}  // namespace

Node::Node(std::string type, IdChangeCallback id_change_callback)
    : type_(std::move(type)), id_change_callback_(std::move(id_change_callback)) {
    children_.reserve(8);
}

void Node::add_child(const std::shared_ptr<Node>& child) {
    if (!child) {
        throw std::invalid_argument("add_child requires a non-null child");
    }
    if (child.get() == this) {
        throw std::invalid_argument("add_child cannot attach a node to itself");
    }
    if (child->is_ancestor_of(this)) {
        throw std::invalid_argument("add_child would create an ancestor cycle");
    }

    auto this_ref = shared_from_this();
    if (auto current_parent = child->parent_.lock()) {
        if (current_parent.get() == this) {
            return;
        }
        current_parent->remove_direct_child(child.get());
        current_parent->mark_dirty(DirtyState::LAYOUT_DIRTY);
    }

    children_.push_back(child);
    child->parent_ = this_ref;
    mark_dirty(DirtyState::LAYOUT_DIRTY);

#ifndef NDEBUG
    assert(debug_validate_subtree());
#endif
}

bool Node::remove_child(const std::shared_ptr<Node>& child) {
    if (!child) {
        return false;
    }

    const bool removed = remove_direct_child(child.get());
    if (!removed) {
        return false;
    }

    child->parent_.reset();
    mark_dirty(DirtyState::LAYOUT_DIRTY);

#ifndef NDEBUG
    assert(debug_validate_subtree());
#endif

    return true;
}

void Node::set_position(int x, int y) {
    if (x_ == x && y_ == y) {
        return;
    }

    x_ = x;
    y_ = y;
    mark_dirty(DirtyState::LAYOUT_DIRTY);
}

void Node::set_size(int width, int height) {
    if (width_ == width && height_ == height) {
        return;
    }

    width_ = width;
    height_ = height;
    mark_dirty(DirtyState::LAYOUT_DIRTY);
}

void Node::set_attribute(std::string key, std::string value) {
    const auto it = attributes_.find(key);
    if (it != attributes_.end() && it->second == value) {
        return;
    }

    const bool is_id_key = key == "id";
    const std::string old_id = is_id_key && it != attributes_.end() ? it->second : std::string{};

    attributes_[std::move(key)] = std::move(value);
    mark_dirty(DirtyState::PAINT_DIRTY);

    if (is_id_key && id_change_callback_) {
        const auto new_it = attributes_.find("id");
        const std::string new_id = new_it != attributes_.end() ? new_it->second : std::string{};
        id_change_callback_(old_id, new_id);
    }
}

std::string Node::get_attribute(std::string_view key) const {
    auto it = attributes_.find(key);
    if (it == attributes_.end()) {
        return {};
    }
    return it->second;
}

bool Node::has_attribute(std::string_view key) const {
    return attributes_.find(key) != attributes_.end();
}

void Node::mark_dirty(DirtyState state) {
    if (dirty_rank(state) > dirty_rank(state_)) {
        state_ = state;
    }

    if (auto p = parent_.lock()) {
        p->mark_descendant_dirty();
    }
}

void Node::mark_clean() {
    state_ = DirtyState::CLEAN;
    has_dirty_descendant_ = false;
    for (const auto& child : children_) {
        child->mark_clean();
    }
}

DirtyState Node::dirty_state() const {
    return state_;
}

bool Node::has_dirty_descendant() const {
    return has_dirty_descendant_;
}

const std::string& Node::type() const {
    return type_;
}

std::string Node::get_id() const {
    auto it = attributes_.find("id");
    if (it == attributes_.end()) {
        return {};
    }
    return it->second;
}

std::shared_ptr<Node> Node::parent() const {
    return parent_.lock();
}

const std::vector<std::shared_ptr<Node>>& Node::children() const {
    return children_;
}

int Node::x() const {
    return x_;
}

int Node::y() const {
    return y_;
}

int Node::width() const {
    return width_;
}

int Node::height() const {
    return height_;
}

int Node::abs_x() const {
    return abs_x_;
}

int Node::abs_y() const {
    return abs_y_;
}

#ifndef NDEBUG
bool Node::debug_validate_subtree() const {
    for (const auto& child : children_) {
        if (!child) {
            return false;
        }
        auto p = child->parent_.lock();
        if (!p || p.get() != this) {
            return false;
        }
    }

    for (size_t i = 0; i < children_.size(); ++i) {
        for (size_t j = i + 1; j < children_.size(); ++j) {
            if (children_[i].get() == children_[j].get()) {
                return false;
            }
        }
    }

    for (const auto& child : children_) {
        if (child.get() == this) {
            return false;
        }
        if (child->is_ancestor_of(this)) {
            return false;
        }
        if (!child->debug_validate_subtree()) {
            return false;
        }
    }

    return true;
}
#endif

bool Node::is_ancestor_of(const Node* maybe_descendant) const {
    auto cursor = maybe_descendant ? maybe_descendant->parent_.lock() : nullptr;
    while (cursor) {
        if (cursor.get() == this) {
            return true;
        }
        cursor = cursor->parent_.lock();
    }
    return false;
}

bool Node::remove_direct_child(const Node* child_ptr) {
    auto it = std::find_if(children_.begin(), children_.end(), [child_ptr](const std::shared_ptr<Node>& n) {
        return n.get() == child_ptr;
    });
    if (it == children_.end()) {
        return false;
    }

    children_.erase(it);
    return true;
}

void Node::mark_descendant_dirty() {
    if (has_dirty_descendant_) {
        return;
    }

    has_dirty_descendant_ = true;
    if (auto p = parent_.lock()) {
        p->mark_descendant_dirty();
    }
}

void Node::apply_layout(int abs_x, int abs_y) {
    abs_x_ = abs_x;
    abs_y_ = abs_y;
}

namespace {

// Selector type for supported selector forms
struct Selector {
    enum class Type {
        TAG_NAME,           // "div"
        ID,                 // "#board"
        ATTRIBUTE,          // "[piece]"
        ATTRIBUTE_VALUE,    // "[piece=K]"
    };

    Type type;
    std::string value;
    std::string attr_name;
    std::string attr_value;
};

// Parse selector string into Selector struct
std::optional<Selector> parse_selector(std::string_view selector) {
    if (selector.empty()) {
        return std::nullopt;
    }

    if (selector[0] == '#') {
        // ID selector: #id
        return Selector{Selector::Type::ID, std::string(selector.substr(1)), "", ""};
    } else if (selector[0] == '[') {
        // Attribute selector: [name] or [name=value]
        if (selector.back() != ']') {
            return std::nullopt;
        }
        auto content = selector.substr(1, selector.size() - 2);
        auto eq_pos = content.find('=');
        if (eq_pos == std::string::npos) {
            // [name]
            return Selector{Selector::Type::ATTRIBUTE, std::string(content), std::string(content), ""};
        } else {
            // [name=value]
            auto name = content.substr(0, eq_pos);
            auto value = content.substr(eq_pos + 1);
            return Selector{Selector::Type::ATTRIBUTE_VALUE, "", std::string(name), std::string(value)};
        }
    } else {
        // Tag name selector
        return Selector{Selector::Type::TAG_NAME, std::string(selector), "", ""};
    }
}

// Check if a node matches the selector
bool node_matches(const Node& node, const Selector& selector) {
    switch (selector.type) {
        case Selector::Type::TAG_NAME:
            return node.type() == selector.value;
        case Selector::Type::ID: {
            const auto id = node.get_attribute("id");
            return id == selector.value;
        }
        case Selector::Type::ATTRIBUTE:
            return node.has_attribute(selector.attr_name);
        case Selector::Type::ATTRIBUTE_VALUE: {
            const auto value = node.get_attribute(selector.attr_name);
            return value == selector.attr_value;
        }
    }
    return false;
}

}  // namespace

std::shared_ptr<Node> Node::querySelector(std::string_view selector) const {
    auto parsed = parse_selector(selector);
    if (!parsed) {
        return nullptr;
    }

    // Check if root matches
    if (node_matches(*this, *parsed)) {
        return const_cast<Node*>(this)->shared_from_this();
    }

    // DFS through children (let recursive calls handle checking)
    for (const auto& child : children_) {
        auto result = child->querySelector(selector);
        if (result) {
            return result;
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<Node>> Node::querySelectorAll(std::string_view selector) const {
    std::vector<std::shared_ptr<Node>> results;

    auto parsed = parse_selector(selector);
    if (!parsed) {
        return results;
    }

    // Check if root matches
    if (node_matches(*this, *parsed)) {
        results.push_back(const_cast<Node*>(this)->shared_from_this());
    }

    // DFS through children (let recursive calls handle matching)
    for (const auto& child : children_) {
        auto child_results = child->querySelectorAll(selector);
        results.insert(results.end(), child_results.begin(), child_results.end());
    }

    return results;
}
