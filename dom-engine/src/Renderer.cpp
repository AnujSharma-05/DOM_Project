#include "Renderer.h"

#include "CharBuffer.h"
#include "Node.h"

#include <iostream>

void Renderer::render(const Node& root) const {
    std::cout << "Render node type='" << root.type() << "' id='" << root.get_id()
              << "' children=" << root.children().size() << '\n';
}

void Renderer::render_to_buffer(const Node& root, CharBuffer& buffer) const {
    buffer.clear_back();

    const std::string label = root.get_id().empty() ? root.type() : root.get_id();
    for (std::size_t i = 0; i < label.size() && i < buffer.width(); ++i) {
        Cell cell;
        cell.ch = label[i];
        cell.fg = 15;
        buffer.set_back(i, 0, cell);
    }

    if (buffer.height() > 1) {
        Cell marker;
        marker.ch = (root.dirty_state() == DirtyState::CLEAN) ? 'C' : 'D';
        marker.fg = 10;
        buffer.set_back(0, 1, marker);
    }
}
