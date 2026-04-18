#include "FrameLoop.h"

#include "IAdapter.h"
#include "Node.h"

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
    for (const auto& adapter : adapters_) {
        adapter->on_update(root, dt);
    }

    last_stats_.layout = layout_engine_.run(root);

    collect_mutations(root, observer_);
    auto records = observer_.flush();
    last_stats_.mutation_count = records.size();

    renderer_.render_to_buffer(root, buffer_);
    last_stats_.changed_cells = buffer_.diff_count();
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
