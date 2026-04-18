#include "LayoutEngine.h"

#include "Node.h"

LayoutStats LayoutEngine::run(const Node& root) const {
    LayoutStats stats;
    visit(root, false, 0, 0, stats);
    return stats;
}

void LayoutEngine::visit(const Node& node, bool parent_dirty, int parent_abs_x, int parent_abs_y, LayoutStats& stats) const {
    const bool node_dirty = node.dirty_state() != DirtyState::CLEAN;
    const bool needs_descend = node.has_dirty_descendant();
    if (!node_dirty && !parent_dirty && !needs_descend) {
        return;
    }

    if (node_dirty && !parent_dirty) {
        ++stats.dirty_subtree_roots;
    }

    ++stats.visited_nodes;
    const int abs_x = parent_abs_x + node.x();
    const int abs_y = parent_abs_y + node.y();
    const_cast<Node&>(node).apply_layout(abs_x, abs_y);

    for (const auto& child : node.children()) {
        // Only recurse if child is dirty or has dirty descendants, or parent is dirty
        if (child->dirty_state() != DirtyState::CLEAN || child->has_dirty_descendant() || node_dirty) {
            visit(*child, node_dirty || parent_dirty, abs_x, abs_y, stats);
        }
    }
}
