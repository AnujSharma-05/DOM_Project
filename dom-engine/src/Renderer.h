#pragma once

class Node;
class CharBuffer;

class Renderer {
public:
    Renderer() = default;

    void render(const Node& root) const;
    void render_to_buffer(const Node& root, CharBuffer& buffer) const;
};
