#pragma once

class Node;

class Renderer {
public:
    Renderer() = default;

    void render(const Node& root) const;
};
