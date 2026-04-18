#include "MutationObserver.h"

#include <utility>

void MutationObserver::subscribe(Callback cb) {
    callbacks_.push_back(std::move(cb));
}

void MutationObserver::enqueue(MutationRecord record) {
    pending_.push_back(std::move(record));
}

std::size_t MutationObserver::pending_count() const {
    return pending_.size();
}

std::vector<MutationRecord> MutationObserver::flush() {
    std::vector<MutationRecord> delivered;
    delivered.swap(pending_);

    const std::span<const MutationRecord> records(delivered.data(), delivered.size());
    for (const auto& cb : callbacks_) {
        cb(records);
    }

    return delivered;
}
