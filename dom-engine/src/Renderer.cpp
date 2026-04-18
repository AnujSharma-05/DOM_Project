#include "Renderer.h"

#include "Node.h"

#include <iostream>

void Renderer::render(const Node& root) const {
    std::cout << "Render root: " << root.get_id() << '\n';
}
