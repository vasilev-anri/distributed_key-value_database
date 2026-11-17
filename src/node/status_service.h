#pragma once

// Answers STATUS and LOCATE requests: what this node knows about the cluster.
// Used by the Kotlin UI (cluster view, key placement) and by `kvcli status`.
//
// Both are answered by the node that receives them and never forwarded: each
// node reports its OWN view. A UI that asks every node can therefore show
// where the nodes disagree, e.g. a node that one peer still sees as
// CONNECTED while another already sees it as down.
//
// Like the other node components it knows nothing about sockets.

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "protocol/codec.h"
#include "protocol/protocol.h"

class KeyValueStore;
namespace cluster { class Membership; struct NodeEntry; }

class StatusService {
public:
    // How this node currently sees a peer (in the node: from PeerManager).
    using StateFn = std::move_only_function<protocol::NodeState(std::uint64_t peer_id)>;

    StatusService(const cluster::Membership& members, const KeyValueStore& store, StateFn peer_state);

    // The complete encoded response to a STATUS or LOCATE request, or
    // std::nullopt if `request` is neither (then it belongs to the Router).
    [[nodiscard]] std::optional<std::vector<std::byte>> handle(const protocol::Frame& request);

private:
    [[nodiscard]] protocol::NodeStatus describe(const cluster::NodeEntry& node);
    [[nodiscard]] std::vector<std::byte> status(const protocol::Header& request);
    [[nodiscard]] std::vector<std::byte> locate(const protocol::Frame& request);

    const cluster::Membership& members_;
    const KeyValueStore& store_;
    StateFn peer_state_;
};
