#pragma once

#include "CharBuffer.h"
#include "LayoutEngine.h"
#include "MutationObserver.h"
#include "MutationQueue.h"
#include "Renderer.h"

#include <chrono>
#include <cstddef>
#include <memory>
#include <vector>

class IAdapter;
class Node;

struct FrameStats {
    LayoutStats layout{};
    std::size_t changed_cells{0};
    std::size_t mutation_count{0};
    std::chrono::nanoseconds on_update_time{0};
    std::chrono::nanoseconds layout_time{0};
    std::chrono::nanoseconds collect_mutations_time{0};
    std::chrono::nanoseconds render_time{0};
    std::chrono::nanoseconds emit_ansi_time{0};
};

class FrameLoop {
public:
    FrameLoop(std::size_t width, std::size_t height);

    void add_adapter(std::shared_ptr<IAdapter> adapter);
    void tick(Node& root, std::chrono::nanoseconds dt);
    MutationQueue& mutation_queue();

    const FrameStats& last_stats() const;
    const CharBuffer& front_buffer() const;

private:
    static void collect_mutations(const Node& node, MutationObserver& observer);

    CharBuffer buffer_;
    Renderer renderer_;
    LayoutEngine layout_engine_;
    MutationObserver observer_;
    MutationQueue mutation_queue_;
    std::vector<std::shared_ptr<IAdapter>> adapters_;
    FrameStats last_stats_{};
};
