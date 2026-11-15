#pragma once

// The set of nodes in the cluster, and which node owns which key.
//
// Ownership is a pure function of (key, node list): every node that knows the
// same nodes computes the same owner, without asking anyone. That is what
// makes a central directory server unnecessary.

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <netinet/in.h>

namespace cluster {

struct NodeEntry {
    std::uint64_t id = 0;   // position in the 64-bit key space
    sockaddr_in6 addr{};    // where the node's UDP data channel is reached
    std::string name;       // canonical "[ip]:port", for logs
};

// A node's id is derived from its address, so no ids need configuring and all
// nodes compute the same id for the same node. The address is first formatted
// canonically ("[::1]:9001"), so different spellings of one IPv6 address agree.
[[nodiscard]] NodeEntry make_node(const sockaddr_in6& addr);

// XOR distance: how "far" a node is from a key in the id space.
constexpr std::uint64_t distance(std::uint64_t a, std::uint64_t b) noexcept { return a ^ b; }

// The node whose id is closest to `key_id` by XOR distance.
// Precondition: `nodes` is not empty. Ids are unique, so there are no ties.
[[nodiscard]] const NodeEntry& closest_node(std::uint64_t key_id, std::span<const NodeEntry> nodes);

class Membership {
public:
    explicit Membership(NodeEntry self);

    // Adds a peer. Throws std::invalid_argument if a node with the same id
    // (i.e. the same address) is already known, including ourselves.
    void add(NodeEntry peer);

    [[nodiscard]] const NodeEntry& self() const noexcept { return nodes_.front(); }

    // All nodes, ourselves included (always first).
    [[nodiscard]] std::span<const NodeEntry> nodes() const noexcept { return nodes_; }

    // The node responsible for `key`.
    [[nodiscard]] const NodeEntry& owner_of(std::string_view key) const;

    [[nodiscard]] bool is_self(const NodeEntry& node) const noexcept { return node.id == self().id; }

private:
    std::vector<NodeEntry> nodes_;  // nodes_[0] is self
};

}  // namespace cluster
