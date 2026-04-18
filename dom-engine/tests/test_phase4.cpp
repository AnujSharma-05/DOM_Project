#include "CharBuffer.h"
#include "Document.h"
#include "FrameLoop.h"
#include "LayoutEngine.h"
#include "Node.h"
#include "Renderer.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

static void test_absolute_layout_propagation() {
    auto root = std::make_shared<Node>("root");
    root->set_position(5, 1);
    root->set_size(10, 10);

    auto child = std::make_shared<Node>("child");
    child->set_position(2, 3);
    child->set_size(2, 2);
    root->add_child(child);

    LayoutEngine engine;
    const auto stats = engine.run(*root);

    assert(stats.visited_nodes >= 2);
    assert(root->abs_x() == 5);
    assert(root->abs_y() == 1);
    assert(child->abs_x() == 7);
    assert(child->abs_y() == 4);
}

static void test_document_id_index() {
    Document document;
    auto root = document.create_element("root");
    document.set_root(root);
    root->set_attribute("id", "root");

    std::vector<std::shared_ptr<Node>> nodes;
    nodes.reserve(1000);

    for (int i = 0; i < 1000; ++i) {
        auto node = document.create_element("node");
        node->set_attribute("id", "n-" + std::to_string(i));
        root->add_child(node);
        nodes.push_back(node);
    }

    for (int i = 0; i < 1000; ++i) {
        const auto node = document.get_by_id("n-" + std::to_string(i));
        assert(node);
        assert(node == nodes[static_cast<std::size_t>(i)]);
    }

    assert(document.debug_dfs_lookups() == 0);
}

static void test_emit_ansi_minimal_sequences() {
    CharBuffer buffer(4, 2);
    Cell changed;
    changed.ch = 'X';
    changed.fg = 12;
    buffer.set_back(1, 0, changed);

    const std::string ansi = buffer.emit_ansi();
    assert(ansi.find('X') != std::string::npos);
    assert(ansi.find("\033[1;2H") != std::string::npos);
    assert(ansi.find("\033[38;5;12m") != std::string::npos);
    assert(ansi.find('A') == std::string::npos);
}

static void test_mutation_queue_applies_once() {
    auto root = std::make_shared<Node>("root");
    root->set_attribute("id", "root");
    root->set_position(0, 0);
    root->set_size(8, 8);

    FrameLoop loop(8, 8);
    std::vector<int> applied(50, 0);

    std::thread producer([&] {
        for (int i = 0; i < 50; ++i) {
            loop.mutation_queue().push([&, i](Node& node) {
                node.set_attribute("mut-" + std::to_string(i), "1");
                ++applied[static_cast<std::size_t>(i)];
            });
        }
    });

    for (int frame = 0; frame < 5; ++frame) {
        loop.tick(*root, std::chrono::milliseconds(16));
    }

    producer.join();
    loop.tick(*root, std::chrono::milliseconds(16));

    for (int i = 0; i < 50; ++i) {
        assert(applied[static_cast<std::size_t>(i)] == 1);
    }
}

static void test_renderer_prunes_clean_subtrees() {
    auto root = std::make_shared<Node>("root");
    root->set_position(0, 0);
    root->set_size(1, 1);

    std::vector<std::shared_ptr<Node>> nodes;
    nodes.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        auto n = std::make_shared<Node>("n");
        n->set_attribute("id", "x");
        n->set_position(i, 0);
        n->set_size(1, 1);
        root->add_child(n);
        nodes.push_back(n);
    }

    root->mark_clean();

    // Dirty only 10 nodes.
    for (int i = 0; i < 10; ++i) {
        nodes[static_cast<std::size_t>(i)]->set_attribute("id", "y");
    }

    LayoutEngine engine;
    engine.run(*root);

    CharBuffer buffer(1200, 2);
    Renderer renderer;
    renderer.render_to_buffer(*root, buffer);

    const auto changed = buffer.diff_count();
    // One changed cell per dirty node label plus root paint at most.
    assert(changed <= 11);
}

int main() {
    test_absolute_layout_propagation();
    test_document_id_index();
    test_emit_ansi_minimal_sequences();
    test_mutation_queue_applies_once();
    test_renderer_prunes_clean_subtrees();
    return 0;
}
