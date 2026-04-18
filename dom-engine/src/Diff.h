#pragma once

#include <memory>
#include <string>
#include <vector>

class Node;

struct Mutation {
    enum class Type { ADD_CHILD, REMOVE_CHILD, SET_ATTRIBUTE };

    Type type;
    std::string target_id;
    std::string parent_id;
    std::string attr_name;
    std::string attr_value;
};

class Diff {
public:
    /// Construct a diff helper.
    /// Preconditions: None.
    /// Postconditions: Ready to compute/apply mutations.
    /// Exceptions: Does not throw.
    Diff() = default;

    /// Compute mutation operations that transform `live` toward `desired` using id-based matching.
    /// Preconditions: Nodes intended for matching should carry unique `id` attributes.
    /// Postconditions: Returned list may include add/remove/set-attribute operations; no tree mutation occurs.
    /// Exceptions: May throw allocation-related exceptions.
    std::vector<Mutation> compute(const std::shared_ptr<Node>& desired, std::shared_ptr<Node> live);

    /// Apply a previously computed mutation list to `live`.
    /// Preconditions: `live` is non-null and mutation ids refer to nodes in `live` where applicable.
    /// Postconditions: `live` tree is mutated according to operations that can be resolved.
    /// Exceptions: Propagates exceptions from underlying Node operations (for example cycle-guard invalid_argument).
    void apply(const std::vector<Mutation>& mutations, std::shared_ptr<Node> live);
};
