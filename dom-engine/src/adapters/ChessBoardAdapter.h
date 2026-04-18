#pragma once

#include "../IAdapter.h"

#include <chrono>
#include <memory>
#include <queue>
#include <span>
#include <string>
#include <vector>

class Node;
class CharBuffer;
class Document;
struct MutationRecord;

// Represents a single chess move
struct Move {
    std::string from_square;  // e.g., "a1"
    std::string to_square;    // e.g., "a2"
    char piece;               // e.g., 'P' for pawn
};

// Game state for the chess board
struct GameState {
    std::queue<Move> pending_moves;
};

/**
 * ChessBoardAdapter demonstrates the full IAdapter contract.
 * Processes chess moves by updating the DOM tree via move commands.
 * - on_update: Dequeue moves from game state, execute them on the DOM
 * - on_render: Could emit board render (not needed for move test)
 * - on_notify_mutations: Track which squares were mutated
 */
class ChessBoardAdapter final : public IAdapter {
public:
    explicit ChessBoardAdapter(GameState& game_state, Document* document = nullptr);

    void on_update(Node& root, std::chrono::nanoseconds frame_dt) override;
    void on_render(const CharBuffer& buffer) override;
    void on_notify_mutations(std::span<const MutationRecord> records) override;

    const std::vector<std::string>& changed_squares() const;

private:
    int frame_count_{0};
    GameState& game_state_;
    Document* document_{nullptr};
    std::vector<std::string> changed_squares_;
};
