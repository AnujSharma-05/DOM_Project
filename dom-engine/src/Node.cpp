#include "Node.h"
#include <iostream>

Node::Node(const std::string& type) : type(type) {}

void Node::addChild(std::shared_ptr<Node> child) {
    child->parent = shared_from_this();
    children.push_back(child);
}

void Node::setProp(const std::string& key, const std::string& value) {
    props[key] = value;
}

void Node::print(int depth) {
    for (int i = 0; i < depth; i++) std::cout << "  ";

    std::cout << type;

    if (!props.empty()) {
        std::cout << " { ";
        for (auto& p : props) {
            std::cout << p.first << ":" << p.second << " ";
        }
        std::cout << "}";
    }

    std::cout << std::endl;

    for (auto& child : children) {
        child->print(depth + 1);
    }
}