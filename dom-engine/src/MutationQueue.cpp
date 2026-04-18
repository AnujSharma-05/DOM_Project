#include "MutationQueue.h"

#include "Node.h"

#include <utility>

void MutationQueue::push(Mutation mutation) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_.push_back(std::move(mutation));
}

void MutationQueue::drain(Node& root) {
    std::vector<Mutation> batch;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        batch.swap(pending_);
    }

    for (auto& mutation : batch) {
        if (mutation) {
            mutation(root);
        }
    }
}

std::size_t MutationQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_.size();
}
