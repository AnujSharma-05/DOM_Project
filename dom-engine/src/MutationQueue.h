#pragma once

#include <functional>
#include <mutex>
#include <vector>

class Node;

class MutationQueue {
public:
    using Mutation = std::function<void(Node&)>;

    void push(Mutation mutation);
    void drain(Node& root);
    std::size_t size() const;

private:
    mutable std::mutex mutex_;
    std::vector<Mutation> pending_;
};
