#include "CharBuffer.h"
#include "Document.h"
#include "FrameLoop.h"
#include "FrameRunner.h"
#include "LayoutEngine.h"
#include "Node.h"
#include "adapters/ChessBoardAdapter.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace {

char initial_piece_for_square(char file, int rank) {
    if (rank == 2) {
        return 'P';
    }
    if (rank == 7) {
        return 'p';
    }

    if (rank == 1) {
        switch (file) {
            case 'a':
            case 'h':
                return 'R';
            case 'b':
            case 'g':
                return 'N';
            case 'c':
            case 'f':
                return 'B';
            case 'd':
                return 'Q';
            case 'e':
                return 'K';
            default:
                return '\0';
        }
    }

    if (rank == 8) {
        switch (file) {
            case 'a':
            case 'h':
                return 'r';
            case 'b':
            case 'g':
                return 'n';
            case 'c':
            case 'f':
                return 'b';
            case 'd':
                return 'q';
            case 'e':
                return 'k';
            default:
                return '\0';
        }
    }

    return '\0';
}

std::string square_id(char file, int rank) {
    return std::string{file} + std::to_string(rank);
}

void build_board(Document& document) {
    auto board = document.create_element("board");
    board->set_attribute("id", "board");
    board->set_position(0, 0);
    board->set_size(8, 8);

    for (int rank = 1; rank <= 8; ++rank) {
        for (char file = 'a'; file <= 'h'; ++file) {
            auto square = document.create_element("square");
            square->set_attribute("id", square_id(file, rank));
            const char piece = initial_piece_for_square(file, rank);
            square->set_attribute("piece", piece == '\0' ? "" : std::string(1, piece));
            square->set_position(static_cast<int>(file - 'a'), 8 - rank);
            square->set_size(1, 1);
            board->add_child(square);
        }
    }

    document.set_root(board);
    board->mark_clean();
}

void load_moves(GameState& game_state) {
    const std::vector<Move> moves = {
        {"e2", "e4", 'P'},
        {"e7", "e5", 'p'},
        {"g1", "f3", 'N'},
        {"b8", "c6", 'n'},
        {"f1", "c4", 'B'},
        {"f8", "c5", 'b'},
        {"c2", "c3", 'P'},
        {"g8", "f6", 'n'},
        {"d2", "d4", 'P'},
        {"e5", "d4", 'p'},
    };

    for (const auto& move : moves) {
        game_state.pending_moves.push(move);
    }
}

void assert_final_position(Document& document) {
    assert(document.get_by_id("d4")->get_attribute("piece") == "p");
    assert(document.get_by_id("e5")->get_attribute("piece") == "");
    assert(document.get_by_id("e4")->get_attribute("piece") == "P");
    assert(document.get_by_id("f3")->get_attribute("piece") == "N");
    assert(document.get_by_id("c6")->get_attribute("piece") == "n");
    assert(document.get_by_id("c4")->get_attribute("piece") == "B");
    assert(document.get_by_id("c5")->get_attribute("piece") == "b");
    assert(document.get_by_id("f6")->get_attribute("piece") == "n");
    assert(document.get_by_id("c3")->get_attribute("piece") == "P");
}

void render_board_ansi(Document& document) {
    CharBuffer board_buffer(8, 8);
    board_buffer.clear_back();

    for (int rank = 1; rank <= 8; ++rank) {
        for (char file = 'a'; file <= 'h'; ++file) {
            const auto square = document.get_by_id(square_id(file, rank));
            assert(square);

            const std::string piece = square->get_attribute("piece");
            Cell cell;
            cell.ch = piece.empty() ? '.' : piece[0];

            if (piece.empty()) {
                cell.fg = 8;
            } else if (piece[0] >= 'A' && piece[0] <= 'Z') {
                cell.fg = 15;
            } else {
                cell.fg = 11;
            }

            const bool light_square = ((file - 'a') + rank) % 2 == 0;
            cell.bg = light_square ? 0 : 236;

            const std::size_t x = static_cast<std::size_t>(file - 'a');
            const std::size_t y = static_cast<std::size_t>(8 - rank);
            board_buffer.set_back(x, y, cell);
        }
    }

    const std::string ansi = board_buffer.emit_ansi();
    std::cout << ansi << '\n';
}

}  // namespace

int main() {
    Document document;
    build_board(document);

    GameState game_state;
    load_moves(game_state);

    FrameLoop loop(8, 8);
    auto adapter = std::make_shared<ChessBoardAdapter>(game_state, &document);
    loop.add_adapter(adapter);

    FrameRunner runner(loop);
    auto root = document.root();
    assert(root);
    const auto result = runner.run_fixed_frames(*root, 10, std::chrono::milliseconds(16));
    assert(result.frames == 10);

    assert_final_position(document);

    render_board_ansi(document);

    return 0;
}
