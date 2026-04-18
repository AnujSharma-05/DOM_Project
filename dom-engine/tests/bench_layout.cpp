#include "LayoutEngine.h"
#include "Node.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

int main() {
    constexpr int kTotalNodes = 10000;
    constexpr int kMutateCount = 100;

    auto root = std::make_shared<Node>("root");
    std::vector<std::shared_ptr<Node>> nodes;
    nodes.reserve(kTotalNodes);
    nodes.push_back(root);

    for (int i = 1; i < kTotalNodes; ++i) {
        auto n = std::make_shared<Node>("n");
        nodes.push_back(n);
        nodes[(i - 1) / 2]->add_child(n);
    }

    // Clean the tree after construction so we start with a baseline
    root->mark_clean();

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> index_dist(1, kTotalNodes - 1);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kMutateCount; ++i) {
        const int idx = index_dist(rng);
        nodes[idx]->set_position(i, i);
    }
    auto end = std::chrono::high_resolution_clock::now();

    int dirty_nodes = 0;
    for (const auto& n : nodes) {
        if (n->dirty_state() != DirtyState::CLEAN) {
            ++dirty_nodes;
        }
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    LayoutEngine engine;
    const auto layout_stats = engine.run(*root);

    std::cout << "phase1_bench\n";
    std::cout << "total_nodes=" << kTotalNodes << '\n';
    std::cout << "mutated_nodes=" << kMutateCount << '\n';
    std::cout << "dirty_nodes_after_mutation=" << dirty_nodes << '\n';
    std::cout << "mutation_time_us=" << elapsed << '\n';
    std::cout << "layout_visited_nodes=" << layout_stats.visited_nodes << '\n';
    std::cout << "layout_dirty_roots=" << layout_stats.dirty_subtree_roots << '\n';
    std::cout << "visit_ratio=" << (static_cast<double>(layout_stats.visited_nodes) / static_cast<double>(kTotalNodes))
              << '\n';

    return 0;
}
