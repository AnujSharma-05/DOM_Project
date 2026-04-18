#include "ChessBoardAdapter.h"

#include "../CharBuffer.h"
#include "../MutationObserver.h"
#include "../Node.h"

#include <iostream>

ChessBoardAdapter::ChessBoardAdapter() = default;

void ChessBoardAdapter::on_update(Node& root, std::chrono::nanoseconds frame_dt) {
    (void)root;
    (void)frame_dt;
    ++frame_count_;

    // TODO: Process input, compute moves, update board state
    // For now, just log that we're in the update phase
    if (frame_count_ % 60 == 0) {
        // Every 60 frames (~1 second at 60fps)
        // std::cerr << "[ChessBoard] Frame " << frame_count_ << " (dt=" << frame_dt.count() << "ns)\n";
    }
}

void ChessBoardAdapter::on_render(const CharBuffer& buffer) {
    (void)buffer;
    // TODO: Emit board render to external display or log move sequence
    // For now, stub is silent; could write to file, send over network, etc.
}

void ChessBoardAdapter::on_notify_mutations(std::span<const MutationRecord> records) {
    ++mutation_events_;
    total_mutations_ += records.size();

    // TODO: Reconcile mutation records with game state
    // For now, track that we received the notification
    // Could decode move notation, validate rules, update replay state
}
