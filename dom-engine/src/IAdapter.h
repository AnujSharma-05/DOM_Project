#pragma once

#include <chrono>
#include <span>

class CharBuffer;
class Node;
struct MutationRecord;

class IAdapter {
public:
    virtual ~IAdapter() = default;

    virtual void on_update(Node& root, std::chrono::nanoseconds dt) = 0;
    virtual void on_render(const CharBuffer& buffer) = 0;
    virtual void on_notify_mutations(std::span<const MutationRecord> records) = 0;
};
