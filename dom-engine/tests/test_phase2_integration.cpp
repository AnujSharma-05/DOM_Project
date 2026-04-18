#include "FrameLoop.h"
#include "FrameRunner.h"
#include "IAdapter.h"
#include "Node.h"

#include <cassert>
#include <chrono>
#include <memory>
#include <span>
#include <string>
#include <vector>

class BurstMutationAdapter final : public IAdapter {
public:
    explicit BurstMutationAdapter(std::vector<std::shared_ptr<Node>> targets)
        : targets_(std::move(targets)) {}

    void on_update(Node&, std::chrono::nanoseconds) override {
        ++updates;
        const int offset = updates % 10;

        for (std::size_t i = 0; i < targets_.size(); ++i) {
            targets_[i]->set_position(static_cast<int>(i % 20), static_cast<int>((i + offset) % 10));
        }
    }

    void on_render(const CharBuffer&) override {
        ++renders;
    }

    void on_notify_mutations(std::span<const MutationRecord> records) override {
        ++notifies;
        last_batch_size = records.size();
    }

    int updates{0};
    int renders{0};
    int notifies{0};
    std::size_t last_batch_size{0};

private:
    std::vector<std::shared_ptr<Node>> targets_;
};

int main() {
    auto root = std::make_shared<Node>("root");
    root->set_position(0, 0);
    root->set_size(80, 10);
    std::vector<std::shared_ptr<Node>> hot_nodes;
    hot_nodes.reserve(100);

    for (int i = 0; i < 1000; ++i) {
        auto n = std::make_shared<Node>("n");
        n->set_attribute("id", "n-" + std::to_string(i));
        n->set_position(static_cast<int>(i % 80), static_cast<int>(i / 80));
        n->set_size(1, 1);
        root->add_child(n);
        if (i < 100) {
            hot_nodes.push_back(n);
        }
    }

    root->mark_clean();

    FrameLoop loop(80, 10);
    auto adapter = std::make_shared<BurstMutationAdapter>(hot_nodes);
    loop.add_adapter(adapter);

    FrameRunner runner(loop);
    const auto result = runner.run_fixed_frames(*root, 10, std::chrono::milliseconds(16));

    assert(result.frames == 10);
    assert(adapter->updates == 10);
    assert(adapter->renders == 10);
    assert(adapter->notifies == 10);
    assert(result.total_mutations > 0);
    assert(result.total_changed_cells > 0);
    assert(adapter->last_batch_size > 0);
    assert(result.total_visited_nodes < 1500);
    assert(root->dirty_state() == DirtyState::CLEAN);

    return 0;
}
