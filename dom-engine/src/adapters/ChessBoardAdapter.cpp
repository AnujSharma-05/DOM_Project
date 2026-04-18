#include "ChessBoardAdapter.h"

#include "../CharBuffer.h"
#include "../MutationObserver.h"
#include "../Node.h"

ChessBoardAdapter::ChessBoardAdapter(GameState& game_state) : game_state_(game_state) {}

void ChessBoardAdapter::on_update(Node& root, std::chrono::nanoseconds frame_dt) {
    (void)frame_dt;
    ++frame_count_;

    if (game_state_.pending_moves.empty()) {
        return;
    }

    const Move move = game_state_.pending_moves.front();
    game_state_.pending_moves.pop();

    const auto from_square = root.querySelector("#" + move.from_square);
    const auto to_square = root.querySelector("#" + move.to_square);
    if (!from_square || !to_square) {
        return;
    }

    from_square->set_attribute("piece", "");
    to_square->set_attribute("piece", std::string(1, move.piece));
}

void ChessBoardAdapter::on_render(const CharBuffer& buffer) {
    (void)buffer;
}

void ChessBoardAdapter::on_notify_mutations(std::span<const MutationRecord> records) {
    for (const auto& record : records) {
        const auto target = record.target.lock();
        if (!target) {
            continue;
        }

        const auto id = target->get_attribute("id");
        if (!id.empty()) {
            changed_squares_.push_back(id);
        }
    }
}

const std::vector<std::string>& ChessBoardAdapter::changed_squares() const {
    return changed_squares_;
}
