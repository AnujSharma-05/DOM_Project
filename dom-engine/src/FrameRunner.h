#pragma once

#include "FrameLoop.h"

#include <chrono>
#include <cstddef>
#include <functional>

class Node;

struct FrameRunResult {
    std::size_t frames{0};
    std::size_t total_visited_nodes{0};
    std::size_t total_changed_cells{0};
    std::size_t total_mutations{0};
};

class FrameRunner {
public:
    explicit FrameRunner(FrameLoop& loop);

    FrameRunResult run_fixed_frames(
        Node& root,
        std::size_t frame_count,
        std::chrono::nanoseconds dt,
        const std::function<void(std::size_t, Node&)>& pre_tick = {});

private:
    FrameLoop& loop_;
};
