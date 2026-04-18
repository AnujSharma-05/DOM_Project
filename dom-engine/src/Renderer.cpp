#include "Renderer.h"

#include "CharBuffer.h"
#include "Node.h"

#include <iostream>
#include <algorithm>

namespace {

// Paint a single node's label to the buffer, respecting bounds and clipping
void paint_node(const Node& node, CharBuffer& buffer) {
    const int x = node.abs_x();
    const int y = node.abs_y();
    const int width = node.width();
    const int height = node.height();
    const int buf_width = static_cast<int>(buffer.width());
    const int buf_height = static_cast<int>(buffer.height());

    // Skip if completely out of bounds
    if (x >= buf_width || y >= buf_height || width <= 0 || height <= 0) {
        return;
    }

    // Get node label
    const std::string label = node.get_id().empty() ? node.type() : node.get_id();

    // Paint label on first row of node's bounds
    int paint_x = std::max(0, x);
    int paint_y = std::max(0, y);
    if (paint_y < buf_height) {
        for (std::size_t i = 0; i < label.size(); ++i) {
            int cell_x = paint_x + static_cast<int>(i);
            if (cell_x >= buf_width) {
                break;
            }
            Cell cell;
            cell.ch = label[i];
            cell.fg = 15;
            buffer.set_back(static_cast<std::size_t>(cell_x), static_cast<std::size_t>(paint_y), cell);
        }
    }

    // Paint dirty state indicator on second row if room
    if (height > 1 && paint_y + 1 < buf_height) {
        Cell marker;
        marker.ch = (node.dirty_state() == DirtyState::CLEAN) ? 'C' : 'D';
        marker.fg = 10;
        buffer.set_back(static_cast<std::size_t>(paint_x), static_cast<std::size_t>(paint_y + 1), marker);
    }
}

}  // namespace

void Renderer::render(const Node& root) const {
    std::cout << "Render node type='" << root.type() << "' id='" << root.get_id()
              << "' children=" << root.children().size() << '\n';
}

void Renderer::render_to_buffer(const Node& root, CharBuffer& buffer) const {
    buffer.clear_back();
    render_subtree(root, buffer);
}

void Renderer::render_subtree(const Node& node, CharBuffer& buffer) const {
    if (node.dirty_state() == DirtyState::CLEAN && !node.has_dirty_descendant()) {
        return;
    }

    paint_node(node, buffer);

    // Recursively paint children
    for (const auto& child : node.children()) {
        if (child->dirty_state() != DirtyState::CLEAN || child->has_dirty_descendant()) {
            render_subtree(*child, buffer);
        }
    }
}
