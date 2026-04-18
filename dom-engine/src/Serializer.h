#pragma once

#include <memory>
#include <string>

class Node;

class Serializer {
public:
    // Serialize a node tree to JSON string
    static std::string serialize(const Node& root);

    // Deserialize a JSON string to a node tree
    static std::shared_ptr<Node> deserialize(const std::string& json);
};
