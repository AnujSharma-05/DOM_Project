#include "FrameRunner.h"

#include "Node.h"

FrameRunner::FrameRunner(FrameLoop& loop) : loop_(loop) {}

FrameRunResult FrameRunner::run_fixed_frames(
    Node& root,
    std::size_t frame_count,
    std::chrono::nanoseconds dt,
    const std::function<void(std::size_t, Node&)>& pre_tick) {
    FrameRunResult result;

    for (std::size_t frame = 0; frame < frame_count; ++frame) {
        if (pre_tick) {
            pre_tick(frame, root);
        }

        loop_.tick(root, dt);
        const auto& stats = loop_.last_stats();

        ++result.frames;
        result.total_visited_nodes += stats.layout.visited_nodes;
        result.total_changed_cells += stats.changed_cells;
        result.total_mutations += stats.mutation_count;
    }

    return result;
}
