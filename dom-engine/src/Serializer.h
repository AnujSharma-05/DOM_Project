#pragma once

#include <memory>
#include <string>

class Node;

class Serializer {
public:
    /// Serialize a node subtree to JSON.
    /// Preconditions: `root` is a valid node graph with acyclic parent/child links.
    /// Postconditions: Returned string contains type/attributes/children for each serialized node.
    /// Exceptions: May throw allocation-related exceptions.
    static std::string serialize(const Node& root);

    /// Deserialize JSON produced by this serializer into a node tree.
    /// Preconditions: `json` must be syntactically valid for the expected object schema.
    /// Postconditions: Returned root owns a reconstructed subtree.
    /// Exceptions: Throws std::runtime_error for malformed JSON input.
    static std::shared_ptr<Node> deserialize(const std::string& json);
};
