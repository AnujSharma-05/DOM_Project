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
    Diff() = default;

    std::vector<Mutation> compute(const std::shared_ptr<Node>& desired, std::shared_ptr<Node> live);
    void apply(const std::vector<Mutation>& mutations, std::shared_ptr<Node> live);
};
