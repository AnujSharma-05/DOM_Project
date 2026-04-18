#pragma once

#include <chrono>
#include <span>

class CharBuffer;
class Node;
struct MutationRecord;

class IAdapter {
public:
    /// Virtual destructor for polymorphic adapter use.
    /// Preconditions: None.
    /// Postconditions: Adapter resources are released.
    /// Exceptions: Does not throw.
    virtual ~IAdapter() = default;

    /// Update hook called at frame start; adapters may mutate DOM here.
    /// Preconditions: `root` is a valid live tree root for this frame.
    /// Postconditions: DOM may be mutated; dirty flags should represent those changes.
    /// Exceptions: Implementations may throw; caller should treat exceptions as frame failures.
    virtual void on_update(Node& root, std::chrono::nanoseconds dt) = 0;

    /// Render hook called after render-to-buffer; must be read-only with respect to DOM topology.
    /// Preconditions: `buffer` reflects current frame back/front swap state.
    /// Postconditions: No DOM mutation should occur in this hook.
    /// Exceptions: Implementations may throw; caller should treat exceptions as frame failures.
    virtual void on_render(const CharBuffer& buffer) = 0;

    /// Mutation notification hook called with batched mutation records for the frame.
    /// Preconditions: `records` points to a valid contiguous batch for the duration of the call.
    /// Postconditions: Adapter may cache/consume records; DOM mutation should be avoided here.
    /// Exceptions: Implementations may throw; caller should treat exceptions as frame failures.
    virtual void on_notify_mutations(std::span<const MutationRecord> records) = 0;
};
