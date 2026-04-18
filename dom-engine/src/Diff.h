#pragma once

class Node;

class Diff {
public:
    Diff() = default;

    void reconcile(Node& root);
};
