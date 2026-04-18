#pragma once

#include "../IAdapter.h"

#include <chrono>
#include <span>
#include <memory>

class Node;
class CharBuffer;
struct MutationRecord;

/**
 * ChessBoardAdapter demonstrates the full IAdapter contract.
 * It could drive a chess board visualization:
 * - on_update: Process user moves (not implemented in stub)
 * - on_render: Could emit board state to external display (stub logs)
 * - on_notify_mutations: Track DOM changes caused by move execution
 */
class ChessBoardAdapter final : public IAdapter {
public:
    ChessBoardAdapter();

    void on_update(Node& root, std::chrono::nanoseconds frame_dt) override;
    void on_render(const CharBuffer& buffer) override;
    void on_notify_mutations(std::span<const MutationRecord> records) override;

private:
    int frame_count_{0};
    int mutation_events_{0};
    std::size_t total_mutations_{0};
};
