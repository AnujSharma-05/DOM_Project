#include "Node.h"

#include <cassert>
#include <memory>
#include <stdexcept>

static void test_add_remove_and_reparent() {
    auto root_a = std::make_shared<Node>("root-a");
    auto root_b = std::make_shared<Node>("root-b");
    auto child = std::make_shared<Node>("child");

    root_a->add_child(child);
    assert(root_a->children().size() == 1);
    assert(child->parent().get() == root_a.get());

    root_b->add_child(child);
    assert(root_a->children().empty());
    assert(root_b->children().size() == 1);
    assert(child->parent().get() == root_b.get());

    const bool removed = root_b->remove_child(child);
    assert(removed);
    assert(root_b->children().empty());
    assert(!child->parent());
}

static void test_cycle_guards() {
    auto root = std::make_shared<Node>("root");
    auto mid = std::make_shared<Node>("mid");
    auto leaf = std::make_shared<Node>("leaf");

    root->add_child(mid);
    mid->add_child(leaf);

    bool caught = false;
    try {
        leaf->add_child(root);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);
}

static void test_noop_geometry_and_dirty_monotonicity() {
    auto node = std::make_shared<Node>("box");
    assert(node->dirty_state() == DirtyState::CLEAN);

    node->set_position(0, 0);
    assert(node->dirty_state() == DirtyState::CLEAN);

    node->set_attribute("class", "x");
    assert(node->dirty_state() == DirtyState::PAINT_DIRTY);

    node->set_size(10, 4);
    assert(node->dirty_state() == DirtyState::LAYOUT_DIRTY);

    node->set_attribute("class", "y");
    assert(node->dirty_state() == DirtyState::LAYOUT_DIRTY);
}

int main() {
    test_add_remove_and_reparent();
    test_cycle_guards();
    test_noop_geometry_and_dirty_monotonicity();
    return 0;
}
