#include "node/status_service.h"

#include <algorithm>
#include <utility>

#include "cluster/hash.h"
#include "cluster/membership.h"
#include "storage/kv_store.h"

using namespace protocol;

StatusService::StatusService(const cluster::Membership& members, const KeyValueStore& store,
                             StateFn peer_state)
    : members_(members), store_(store), peer_state_(std::move(peer_state)) {}

std::optional<std::vector<std::byte>> StatusService::handle(const Frame& request) {
    switch (request.header.type) {
        case MessageType::StatusRequest: return status(request.header);
        case MessageType::LocateRequest: return locate(request);
        default: return std::nullopt;
    }
}

NodeStatus StatusService::describe(const cluster::NodeEntry& node) {
    NodeStatus s;
    s.id = node.id;
    s.address = node.name.substr(0, kMaxAddressSize);  // canonical "[ip]:port", always short
    if (members_.is_self(node)) s.state = NodeState::Self;
    else s.state = peer_state_ ? peer_state_(node.id) : NodeState::Disconnected;
    return s;
}

std::vector<std::byte> StatusService::status(const Header& request) {
    StatusPayload p;
    p.node_id = members_.self().id;
    p.key_count = static_cast<std::uint32_t>(store_.size());
    p.replication_factor = static_cast<std::uint8_t>(cluster::kReplicationFactor);

    const auto nodes = members_.nodes();  // self first
    const std::size_t n = std::min(nodes.size(), kMaxStatusNodes);  // one datagram's worth
    for (std::size_t i = 0; i < n; ++i) p.nodes.push_back(describe(nodes[i]));

    return encode_message({.type = MessageType::StatusResponse,
                           .request_id = request.request_id,
                           .sender_id = request.sender_id},
                          encode_payload(p));
}

std::vector<std::byte> StatusService::locate(const Frame& request) {
    const Header& h = request.header;
    const auto key = decode_key(request.payload);
    if (!key) {
        const Status status = key.error() == DecodeError::KeyTooLarge ? Status::KeyTooLarge
                                                                      : Status::InvalidRequest;
        return encode_message({.type = MessageType::LocateResponse,
                               .status = status,
                               .request_id = h.request_id,
                               .sender_id = h.sender_id});
    }

    LocatePayload p;
    p.key_hash = cluster::key_hash(key->key);
    for (const cluster::NodeEntry& node : members_.replicas_of(key->key)) p.nodes.push_back(describe(node));

    return encode_message({.type = MessageType::LocateResponse,
                           .request_id = h.request_id,
                           .sender_id = h.sender_id},
                          encode_payload(p));
}
