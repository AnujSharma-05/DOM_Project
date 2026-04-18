#include "CharBuffer.h"
#include "Diff.h"
#include "Document.h"
#include "LayoutEngine.h"
#include "MutationQueue.h"
#include "Node.h"
#include "Serializer.h"

#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>

static void test_node_remove_child_non_existent() {
    auto root = std::make_shared<Node>("root");
    auto child = std::make_shared<Node>("child");

    const bool removed = root->remove_child(child);
    assert(!removed);
    assert(root->children().empty());
}

static void test_node_invalid_selectors() {
    auto root = std::make_shared<Node>("root");
    auto child = std::make_shared<Node>("n");
    child->set_attribute("id", "a");
    root->add_child(child);

    assert(root->querySelector("") == nullptr);
    assert(root->querySelector("[broken") == nullptr);

    const auto all_empty = root->querySelectorAll("");
    assert(all_empty.empty());
}

static void test_serializer_malformed_json_throws() {
    bool threw = false;
    try {
        (void)Serializer::deserialize("{\"type\":\"node\"");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

static void test_document_get_by_id_after_node_removed() {
    Document doc;
    auto root = doc.create_element("root");
    root->set_attribute("id", "root");
    doc.set_root(root);

    auto child = doc.create_element("child");
    child->set_attribute("id", "x");
    root->add_child(child);

    assert(doc.get_by_id("x") == child);

    const bool removed = root->remove_child(child);
    assert(removed);
    child.reset();

    assert(doc.get_by_id("x") == nullptr);
}

static void test_document_root_accessors() {
    Document doc;
    assert(doc.root() == nullptr);

    auto root = doc.create_element("root");
    doc.set_root(root);
    assert(doc.root() == root);
}

static void test_mutation_queue_drain_empty_noop() {
    MutationQueue queue;
    auto root = std::make_shared<Node>("root");

    queue.drain(*root);
    assert(queue.size() == 0);
}

static void test_charbuffer_core_accessors() {
    CharBuffer buffer(3, 2);
    assert(buffer.width() == 3);
    assert(buffer.height() == 2);

    Cell c;
    c.ch = 'Z';
    c.fg = 11;
    buffer.set_back(2, 1, c);

    const Cell& read_back = buffer.back(2, 1);
    assert(read_back.ch == 'Z');
    assert(read_back.fg == 11);

    buffer.resize(5, 4);
    assert(buffer.width() == 5);
    assert(buffer.height() == 4);
}

static void test_layout_engine_clean_tree_no_visit() {
    auto root = std::make_shared<Node>("root");
    root->mark_clean();

    LayoutEngine engine;
    const auto stats = engine.run(*root);
    assert(stats.visited_nodes == 0);
}

static void test_diff_null_inputs_are_safe() {
    Diff diff;
    auto live = std::make_shared<Node>("root");
    auto desired = std::make_shared<Node>("root");

    const auto mut1 = diff.compute(nullptr, live);
    assert(mut1.empty());
    const auto mut2 = diff.compute(desired, nullptr);
    assert(mut2.empty());

    diff.apply({}, live);
}

int main() {
    test_node_remove_child_non_existent();
    test_node_invalid_selectors();
    test_serializer_malformed_json_throws();
    test_document_get_by_id_after_node_removed();
    test_document_root_accessors();
    test_mutation_queue_drain_empty_noop();
    test_charbuffer_core_accessors();
    test_layout_engine_clean_tree_no_visit();
    test_diff_null_inputs_are_safe();
    return 0;
}
