#pragma once

#include <functional>
#include <span>
#include <string>
#include <vector>

class Node;

struct MutationRecord {
    enum class Type {
        ChildList,
        Attributes,
        Geometry,
    };

    Type type{Type::Attributes};
    const Node* target{nullptr};
    std::string detail;
};

class MutationObserver {
public:
    using Callback = std::function<void(std::span<const MutationRecord>)>;

    void subscribe(Callback cb);
    void enqueue(MutationRecord record);

    std::size_t pending_count() const;
    std::vector<MutationRecord> flush();

private:
    std::vector<Callback> callbacks_;
    std::vector<MutationRecord> pending_;
};
