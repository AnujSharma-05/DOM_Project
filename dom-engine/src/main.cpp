#include "Diff.h"
#include "FrameLoop.h"
#include "FrameRunner.h"
#include "Node.h"
#include "Renderer.h"

#include <chrono>
#include <iostream>
#include <memory>

int main() {
    auto root = std::make_shared<Node>("root");
    auto child = std::make_shared<Node>("child-1");
    child->set_attribute("id", "child-1");

    root->add_child(child);
    child->set_position(2, 3);
    child->set_size(8, 2);

    Diff diff;
    auto mutations = diff.compute(root, root);
    diff.apply(mutations, root);

    FrameLoop loop(40, 5);
    FrameRunner runner(loop);
    const auto run = runner.run_fixed_frames(*root, 2, std::chrono::milliseconds(16));

    Renderer renderer;
    renderer.render(*root);

    std::cout << "Root children: " << root->children().size() << '\n';

    return 0;
}
