#include "Diff.h"
#include "FrameLoop.h"
#include "FrameRunner.h"
#include "Node.h"
#include "Serializer.h"
#include "adapters/ChessBoardAdapter.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

static void test_query_selector_supports_supported_forms() {
    auto root = std::make_shared<Node>("div");
    root->set_attribute("id", "board");

    auto first = std::make_shared<Node>("span");
    first->set_attribute("piece", "K");
    first->set_attribute("id", "first");

    auto second = std::make_shared<Node>("span");
    second->set_attribute("piece", "Q");
    second->set_attribute("id", "second");

    auto wrapper = std::make_shared<Node>("section");
    auto nested = std::make_shared<Node>("span");
    nested->set_attribute("piece", "K");
    nested->set_attribute("id", "nested");

    wrapper->add_child(nested);
    root->add_child(first);
    root->add_child(second);
    root->add_child(wrapper);

    assert(root->querySelector("div") == root);
    assert(root->querySelector("#board") == root);
    assert(root->querySelector("[piece]") == first);
    assert(root->querySelector("[piece=K]") == first);

    const auto spans = root->querySelectorAll("span");
    assert(spans.size() == 3);
    assert(spans[0] == first);
    assert(spans[1] == second);
    assert(spans[2] == nested);

    const auto kings = root->querySelectorAll("[piece=K]");
    assert(kings.size() == 2);
    assert(kings[0] == first);
    assert(kings[1] == nested);
}

static void test_serializer_round_trip() {
    auto root = std::make_shared<Node>("board");
    root->set_attribute("id", "game");

    auto row = std::make_shared<Node>("row");
    row->set_attribute("id", "rank1");

    auto square_a1 = std::make_shared<Node>("square");
    square_a1->set_attribute("id", "a1");
    square_a1->set_attribute("piece", "R");

    auto square_b1 = std::make_shared<Node>("square");
    square_b1->set_attribute("id", "b1");
    square_b1->set_attribute("piece", "N");

    row->add_child(square_a1);
    row->add_child(square_b1);
    root->add_child(row);

    const std::string json = Serializer::serialize(*root);
    const auto restored = Serializer::deserialize(json);

    assert(restored);
    assert(restored->type() == "board");
    assert(restored->get_attribute("id") == "game");
    assert(restored->children().size() == 1);

    const auto restored_row = restored->children()[0];
    assert(restored_row->type() == "row");
    assert(restored_row->get_attribute("id") == "rank1");
    assert(restored_row->children().size() == 2);

    const auto restored_a1 = restored_row->children()[0];
    assert(restored_a1->type() == "square");
    assert(restored_a1->get_attribute("id") == "a1");
    assert(restored_a1->get_attribute("piece") == "R");

    const auto restored_b1 = restored_row->children()[1];
    assert(restored_b1->type() == "square");
    assert(restored_b1->get_attribute("id") == "b1");
    assert(restored_b1->get_attribute("piece") == "N");
}

static void test_diff_move_and_attribute_change() {
    auto live = std::make_shared<Node>("root");
    live->set_attribute("id", "root");

    auto live_left = std::make_shared<Node>("group");
    live_left->set_attribute("id", "left");
    auto live_right = std::make_shared<Node>("group");
    live_right->set_attribute("id", "right");

    auto mover = std::make_shared<Node>("square");
    mover->set_attribute("id", "mover");
    mover->set_attribute("piece", "R");
    auto watcher = std::make_shared<Node>("square");
    watcher->set_attribute("id", "watcher");
    watcher->set_attribute("piece", "P");

    live_left->add_child(mover);
    live_left->add_child(watcher);
    live->add_child(live_left);
    live->add_child(live_right);

    auto desired = std::make_shared<Node>("root");
    desired->set_attribute("id", "root");

    auto desired_left = std::make_shared<Node>("group");
    desired_left->set_attribute("id", "left");
    auto desired_right = std::make_shared<Node>("group");
    desired_right->set_attribute("id", "right");

    auto desired_mover = std::make_shared<Node>("square");
    desired_mover->set_attribute("id", "mover");
    desired_mover->set_attribute("piece", "R");

    auto desired_watcher = std::make_shared<Node>("square");
    desired_watcher->set_attribute("id", "watcher");
    desired_watcher->set_attribute("piece", "Q");

    desired_right->add_child(desired_mover);
    desired_left->add_child(desired_watcher);
    desired->add_child(desired_left);
    desired->add_child(desired_right);

    Diff differ;
    const auto mutations = differ.compute(desired, live);

    assert(mutations.size() == 2);
    assert(std::count_if(mutations.begin(), mutations.end(), [](const Mutation& m) {
        return m.type == Mutation::Type::ADD_CHILD && m.target_id == "mover" && m.parent_id == "right";
    }) == 1);
    assert(std::count_if(mutations.begin(), mutations.end(), [](const Mutation& m) {
        return m.type == Mutation::Type::SET_ATTRIBUTE && m.target_id == "watcher" && m.attr_name == "piece" && m.attr_value == "Q";
    }) == 1);

    differ.apply(mutations, live);

    const auto moved = live->querySelector("#mover");
    const auto changed = live->querySelector("#watcher");
    assert(moved);
    assert(changed);
    assert(moved->parent()->get_attribute("id") == "right");
    assert(changed->get_attribute("piece") == "Q");
}

static void test_chessboard_adapter_replays_moves() {
    auto board = std::make_shared<Node>("board");
    board->set_attribute("id", "board");
    board->set_position(0, 0);
    board->set_size(8, 8);

    auto a1 = std::make_shared<Node>("square");
    a1->set_attribute("id", "a1");
    a1->set_attribute("piece", "R");
    a1->set_position(0, 0);
    a1->set_size(1, 1);

    auto a2 = std::make_shared<Node>("square");
    a2->set_attribute("id", "a2");
    a2->set_position(1, 0);
    a2->set_size(1, 1);

    auto b1 = std::make_shared<Node>("square");
    b1->set_attribute("id", "b1");
    b1->set_attribute("piece", "P");
    b1->set_position(0, 1);
    b1->set_size(1, 1);

    auto b2 = std::make_shared<Node>("square");
    b2->set_attribute("id", "b2");
    b2->set_position(1, 1);
    b2->set_size(1, 1);

    board->add_child(a1);
    board->add_child(a2);
    board->add_child(b1);
    board->add_child(b2);
    board->mark_clean();

    GameState game_state;
    game_state.pending_moves.push(Move{"a1", "a2", 'R'});
    game_state.pending_moves.push(Move{"b1", "b2", 'P'});
    game_state.pending_moves.push(Move{"a2", "a1", 'R'});

    FrameLoop loop(8, 8);
    auto adapter = std::make_shared<ChessBoardAdapter>(game_state);
    loop.add_adapter(adapter);

    FrameRunner runner(loop);
    const auto result = runner.run_fixed_frames(*board, 3, std::chrono::milliseconds(16));

    assert(result.frames == 3);
    assert(board->querySelector("#a1")->get_attribute("piece") == "R");
    assert(board->querySelector("#a2")->get_attribute("piece") == "");
    assert(board->querySelector("#b1")->get_attribute("piece") == "");
    assert(board->querySelector("#b2")->get_attribute("piece") == "P");

    const auto& changed_squares = adapter->changed_squares();
    assert(!changed_squares.empty());
    assert(std::count(changed_squares.begin(), changed_squares.end(), "a1") > 0);
    assert(std::count(changed_squares.begin(), changed_squares.end(), "a2") > 0);
    assert(std::count(changed_squares.begin(), changed_squares.end(), "b1") > 0);
    assert(std::count(changed_squares.begin(), changed_squares.end(), "b2") > 0);
}

int main() {
    test_query_selector_supports_supported_forms();
    test_serializer_round_trip();
    test_diff_move_and_attribute_change();
    test_chessboard_adapter_replays_moves();
    return 0;
}
