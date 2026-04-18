#include "Diff.h"
#include "Node.h"
#include "Renderer.h"

#include <memory>

int main() {
    auto root = std::make_shared<Node>("root");
    auto child = std::make_shared<Node>("child-1");

    root->add_child(child);
    child->set_position(2, 3);

    Diff diff;
    diff.reconcile(*root);

    Renderer renderer;
    renderer.render(*root);

    return 0;
}
