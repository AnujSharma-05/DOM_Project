#include "FrameLoop.h"

#include "IAdapter.h"
#include "Node.h"

#include <chrono>
#include <stdexcept>
#include <utility>

namespace {

MutationRecord::Type mutation_type_for(const Node& node) {
    return node.dirty_state() == DirtyState::LAYOUT_DIRTY ? MutationRecord::Type::Geometry
                                                           : MutationRecord::Type::Attributes;
}

}  // namespace

FrameLoop::FrameLoop(std::size_t width, std::size_t height) : buffer_(width, height) {}

void FrameLoop::add_adapter(std::shared_ptr<IAdapter> adapter) {
    if (!adapter) {
        throw std::invalid_argument("add_adapter requires non-null adapter");
    }
    adapters_.push_back(std::move(adapter));
}

void FrameLoop::tick(Node& root, std::chrono::nanoseconds dt) {
    mutation_queue_.drain(root);

    const auto update_start = std::chrono::high_resolution_clock::now();
    for (const auto& adapter : adapters_) {
        adapter->on_update(root, dt);
    }
    const auto update_end = std::chrono::high_resolution_clock::now();
    last_stats_.on_update_time = std::chrono::duration_cast<std::chrono::nanoseconds>(update_end - update_start);

    const auto layout_start = std::chrono::high_resolution_clock::now();
    last_stats_.layout = layout_engine_.run(root);
    const auto layout_end = std::chrono::high_resolution_clock::now();
    last_stats_.layout_time = std::chrono::duration_cast<std::chrono::nanoseconds>(layout_end - layout_start);

    const auto collect_start = std::chrono::high_resolution_clock::now();
    collect_mutations(root, observer_);
    auto records = observer_.flush();
    last_stats_.mutation_count = records.size();
    const auto collect_end = std::chrono::high_resolution_clock::now();
    last_stats_.collect_mutations_time = std::chrono::duration_cast<std::chrono::nanoseconds>(collect_end - collect_start);

    const auto render_start = std::chrono::high_resolution_clock::now();
    renderer_.render_to_buffer(root, buffer_);
    const auto render_end = std::chrono::high_resolution_clock::now();
    last_stats_.render_time = std::chrono::duration_cast<std::chrono::nanoseconds>(render_end - render_start);

    last_stats_.changed_cells = buffer_.diff_count();

    const auto emit_start = std::chrono::high_resolution_clock::now();
    [[maybe_unused]] const auto ansi = buffer_.emit_ansi();
    const auto emit_end = std::chrono::high_resolution_clock::now();
    last_stats_.emit_ansi_time = std::chrono::duration_cast<std::chrono::nanoseconds>(emit_end - emit_start);

    buffer_.swap_buffers();

    for (const auto& adapter : adapters_) {
        adapter->on_render(buffer_);
        adapter->on_notify_mutations(std::span<const MutationRecord>(records.data(), records.size()));
    }

    root.mark_clean();
}

const FrameStats& FrameLoop::last_stats() const {
    return last_stats_;
}

const CharBuffer& FrameLoop::front_buffer() const {
    return buffer_;
}

MutationQueue& FrameLoop::mutation_queue() {
    return mutation_queue_;
}

void FrameLoop::collect_mutations(const Node& node, MutationObserver& observer) {
    const bool dirty = node.dirty_state() != DirtyState::CLEAN;
    const bool needs_descend = node.has_dirty_descendant();
    if (!dirty && !needs_descend) {
        return;
    }

    if (dirty) {
        MutationRecord record;
        record.type = mutation_type_for(node);
        record.target = node.weak_from_this();
        record.detail = node.type();
        observer.enqueue(std::move(record));
    }

    for (const auto& child : node.children()) {
        if (child->dirty_state() != DirtyState::CLEAN || child->has_dirty_descendant()) {
            collect_mutations(*child, observer);
        }
    }
}
