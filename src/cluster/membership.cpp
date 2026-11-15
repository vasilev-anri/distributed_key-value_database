#include "cluster/membership.h"

#include <stdexcept>
#include <utility>

#include "cluster/hash.h"
#include "utils/address.h"

namespace cluster {

NodeEntry make_node(const sockaddr_in6& addr) {
    NodeEntry node;
    node.addr = addr;
    node.name = to_string(addr);
    node.id = key_hash(node.name);
    return node;
}

const NodeEntry& closest_node(std::uint64_t key_id, std::span<const NodeEntry> nodes) {
    if (nodes.empty()) throw std::logic_error("closest_node: no nodes");

    const NodeEntry* best = &nodes.front();
    for (const NodeEntry& node : nodes) {
        if (distance(node.id, key_id) < distance(best->id, key_id)) best = &node;
    }
    return *best;
}

Membership::Membership(NodeEntry self) { nodes_.push_back(std::move(self)); }

void Membership::add(NodeEntry peer) {
    for (const NodeEntry& node : nodes_) {
        if (node.id == peer.id) throw std::invalid_argument("duplicate node: " + peer.name);
    }
    nodes_.push_back(std::move(peer));
}

const NodeEntry& Membership::owner_of(std::string_view key) const {
    return closest_node(key_hash(key), nodes_);
}

}  // namespace cluster
