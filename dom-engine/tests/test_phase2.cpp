#include "CharBuffer.h"
#include "FrameLoop.h"
#include "IAdapter.h"
#include "LayoutEngine.h"
#include "MutationObserver.h"
#include "Node.h"
#include "Renderer.h"

#include <cassert>
#include <chrono>
#include <memory>
#include <string>

class CountingAdapter final : public IAdapter {
public:
    void on_update(Node& root, std::chrono::nanoseconds) override {
        ++update_calls;
        root.set_attribute("tick", std::to_string(update_calls));
    }

    void on_render(const CharBuffer&) override {
        ++render_calls;
    }

    void on_notify_mutations(std::span<const MutationRecord> records) override {
        ++notify_calls;
        last_mutation_count = records.size();
    }

    int update_calls{0};
    int render_calls{0};
    int notify_calls{0};
    std::size_t last_mutation_count{0};
};

static void test_layout_prunes_clean_subtrees() {
    auto root = std::make_shared<Node>("root");
    auto a = std::make_shared<Node>("a");
    auto b = std::make_shared<Node>("b");

    root->add_child(a);
    root->add_child(b);
    root->mark_clean();

    a->set_position(1, 1);

    LayoutEngine engine;
    const auto stats = engine.run(*root);
    assert(stats.visited_nodes >= 2);
    assert(stats.dirty_subtree_roots == 1);
}

static void test_char_buffer_diff_and_swap() {
    CharBuffer buffer(10, 3);
    Renderer renderer;

    auto root = std::make_shared<Node>("root");
    root->set_attribute("id", "board");
    root->set_position(0, 0);
    root->set_size(10, 3);

    // First render: should show differences
    renderer.render_to_buffer(*root, buffer);
    const auto changed = buffer.diff_count();
    assert(changed > 0);

    buffer.swap_buffers();
    buffer.clear_back();

    // Render the same content again: should show zero differences
    renderer.render_to_buffer(*root, buffer);
    const auto changed_again = buffer.diff_count();
    assert(changed_again == 0);  // Same content rendered twice should produce no diffs
}

static void test_frame_loop_calls_adapter_hooks() {
    auto root = std::make_shared<Node>("root");
    root->set_attribute("id", "dashboard");

    auto adapter = std::make_shared<CountingAdapter>();
    FrameLoop loop(20, 4);
    loop.add_adapter(adapter);

    loop.tick(*root, std::chrono::milliseconds(16));

    assert(adapter->update_calls == 1);
    assert(adapter->render_calls == 1);
    assert(adapter->notify_calls == 1);
    assert(adapter->last_mutation_count >= 1);
    assert(root->dirty_state() == DirtyState::CLEAN);
}

int main() {
    test_layout_prunes_clean_subtrees();
    test_char_buffer_diff_and_swap();
    test_frame_loop_calls_adapter_hooks();
    return 0;
}
