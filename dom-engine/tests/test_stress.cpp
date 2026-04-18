#include "LayoutEngine.h"
#include "Node.h"

#include <cassert>
#include <memory>
#include <random>
#include <string>
#include <vector>

int main() {
    constexpr int kNodeCount = 500;
    constexpr int kMutations = 10000;
    constexpr int kSeed = 1337;

    std::mt19937 rng(kSeed);
    std::uniform_int_distribution<int> node_dist(0, kNodeCount - 1);
    std::uniform_int_distribution<int> op_dist(0, 3);
    std::uniform_int_distribution<int> attr_dist(0, 15);
    std::uniform_int_distribution<int> pos_dist(0, 200);

    std::vector<std::shared_ptr<Node>> nodes;
    nodes.reserve(kNodeCount);

    auto root = std::make_shared<Node>("root");
    root->set_attribute("id", "root");
    nodes.push_back(root);

    // Start as a wide, stable tree.
    for (int i = 1; i < kNodeCount; ++i) {
        auto n = std::make_shared<Node>("n");
        n->set_attribute("id", "n-" + std::to_string(i));
        root->add_child(n);
        nodes.push_back(n);
    }

    for (int step = 1; step <= kMutations; ++step) {
        const int op = op_dist(rng);
        const int a = node_dist(rng);
        const int b = node_dist(rng);

        auto node_a = nodes[static_cast<std::size_t>(a)];
        auto node_b = nodes[static_cast<std::size_t>(b)];

        try {
            switch (op) {
                case 0: {
                    // add/reparent
                    if (node_a != node_b) {
                        node_a->add_child(node_b);
                    }
                    break;
                }
                case 1: {
                    // remove from current parent
                    auto p = node_a->parent();
                    if (p) {
                        p->remove_child(node_a);
                    }
                    break;
                }
                case 2: {
                    // set random attribute
                    node_a->set_attribute("k" + std::to_string(attr_dist(rng)), std::to_string(attr_dist(rng)));
                    break;
                }
                case 3: {
                    // set position
                    node_a->set_position(pos_dist(rng), pos_dist(rng));
                    break;
                }
            }
        } catch (const std::invalid_argument&) {
            // Cycle guards intentionally throw; ignore and continue deterministic stream.
        }

        if (step % 100 == 0) {
#ifndef NDEBUG
            assert(root->debug_validate_subtree());
#endif
        }
    }

    // Normalize to a star tree before final ratio assertion.
    root->mark_clean();
    for (int i = 1; i < kNodeCount; ++i) {
        try {
            root->add_child(nodes[static_cast<std::size_t>(i)]);
        } catch (const std::invalid_argument&) {
            // skip impossible reparent edge cases
        }
    }
    root->mark_clean();

    for (int i = 1; i <= 50; ++i) {
        nodes[static_cast<std::size_t>(i)]->set_position(i, i);
    }

    LayoutEngine engine;
    const auto stats = engine.run(*root);
    const double ratio = static_cast<double>(stats.visited_nodes) / static_cast<double>(kNodeCount);
    assert(ratio < 0.15);

    return 0;
}
