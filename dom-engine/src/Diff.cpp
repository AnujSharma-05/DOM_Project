#include "Diff.h"

#include "Node.h"

#include <map>
#include <string>

namespace {

struct NodeInfo {
    std::shared_ptr<Node> node;
    std::string parent_id;
};

void collect_desired(const std::shared_ptr<Node>& node,
                     const std::string& parent_id,
                     std::map<std::string, NodeInfo>& out) {
    if (!node) {
        return;
    }

    const std::string id = node->get_attribute("id");
    if (!id.empty()) {
        out[id] = NodeInfo{node, parent_id};
    }

    const std::string next_parent_id = id;
    for (const auto& child : node->children()) {
        collect_desired(child, next_parent_id, out);
    }
}

void collect_live(const std::shared_ptr<Node>& node,
                  const std::string& parent_id,
                  std::map<std::string, NodeInfo>& out) {
    if (!node) {
        return;
    }

    const std::string id = node->get_attribute("id");
    if (!id.empty()) {
        out[id] = NodeInfo{node, parent_id};
    }

    const std::string next_parent_id = id;
    for (const auto& child : node->children()) {
        collect_live(child, next_parent_id, out);
    }
}

std::shared_ptr<Node> find_by_id(const std::shared_ptr<Node>& root, const std::string& id) {
    if (!root) {
        return nullptr;
    }

    if (root->get_attribute("id") == id) {
        return root;
    }

    for (const auto& child : root->children()) {
        auto found = find_by_id(child, id);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

}  // namespace

std::vector<Mutation> Diff::compute(const std::shared_ptr<Node>& desired, std::shared_ptr<Node> live) {
    std::vector<Mutation> mutations;
    if (!desired || !live) {
        return mutations;
    }

    std::map<std::string, NodeInfo> desired_map;
    std::map<std::string, NodeInfo> live_map;
    collect_desired(desired, "", desired_map);
    collect_live(live, "", live_map);

    // Reparent existing nodes whose parent id changed.
    for (const auto& [id, desired_info] : desired_map) {
        const auto live_it = live_map.find(id);
        if (live_it == live_map.end()) {
            continue;
        }

        const std::string& desired_parent_id = desired_info.parent_id;
        const std::string& live_parent_id = live_it->second.parent_id;
        if (!desired_parent_id.empty() && desired_parent_id != live_parent_id) {
            mutations.push_back(Mutation{Mutation::Type::ADD_CHILD, id, desired_parent_id, {}, {}});
        }
    }

    // Attribute differences.
    for (const auto& [id, desired_info] : desired_map) {
        const auto live_it = live_map.find(id);
        if (live_it == live_map.end()) {
            continue;
        }

        const auto& desired_node = desired_info.node;
        const auto& live_node = live_it->second.node;
        for (const auto& [attr_name, desired_value] : desired_node->attributes()) {
            const auto live_value = live_node->get_attribute(attr_name);
            if (live_value != desired_value) {
                mutations.push_back(Mutation{Mutation::Type::SET_ATTRIBUTE, id, {}, attr_name, desired_value});
            }
        }
    }

    // Remove nodes that exist in live but not desired.
    for (const auto& [id, live_info] : live_map) {
        if (!desired_map.count(id) && !live_info.parent_id.empty()) {
            mutations.push_back(Mutation{Mutation::Type::REMOVE_CHILD, id, live_info.parent_id, {}, {}});
        }
    }

    return mutations;
}

void Diff::apply(const std::vector<Mutation>& mutations, std::shared_ptr<Node> live) {
    for (const auto& mutation : mutations) {
        const auto target = find_by_id(live, mutation.target_id);
        if (!target) {
            continue;
        }

        switch (mutation.type) {
            case Mutation::Type::SET_ATTRIBUTE:
                target->set_attribute(mutation.attr_name, mutation.attr_value);
                break;
            case Mutation::Type::ADD_CHILD: {
                const auto parent = find_by_id(live, mutation.parent_id);
                if (parent) {
                    parent->add_child(target);
                }
                break;
            }
            case Mutation::Type::REMOVE_CHILD: {
                const auto parent = target->parent();
                if (parent) {
                    parent->remove_child(target);
                }
                break;
            }
        }
    }
}
