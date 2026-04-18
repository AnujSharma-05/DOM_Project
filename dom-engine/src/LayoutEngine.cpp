#include "LayoutEngine.h"

#include "Node.h"

LayoutStats LayoutEngine::run(const Node& root) const {
    LayoutStats stats;
    visit(root, false, stats);
    return stats;
}

void LayoutEngine::visit(const Node& node, bool parent_dirty, LayoutStats& stats) const {
    const bool node_dirty = node.dirty_state() != DirtyState::CLEAN;
    const bool needs_descend = node.has_dirty_descendant();
    if (!node_dirty && !parent_dirty && !needs_descend) {
        return;
    }

    if (node_dirty && !parent_dirty) {
        ++stats.dirty_subtree_roots;
    }

    ++stats.visited_nodes;
    for (const auto& child : node.children()) {
        // Only recurse if child is dirty or has dirty descendants, or parent is dirty
        if (child->dirty_state() != DirtyState::CLEAN || child->has_dirty_descendant() || node_dirty) {
            visit(*child, node_dirty || parent_dirty, stats);
        }
    }
}
