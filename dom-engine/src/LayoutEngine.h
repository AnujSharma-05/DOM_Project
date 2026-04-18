#pragma once

#include <cstddef>

class Node;

struct LayoutStats {
    std::size_t visited_nodes{0};
    std::size_t dirty_subtree_roots{0};
};

class LayoutEngine {
public:
    LayoutEngine() = default;

    LayoutStats run(Node& root) const;

private:
    void visit(Node& node, bool parent_dirty, int parent_abs_x, int parent_abs_y, LayoutStats& stats) const;
};
