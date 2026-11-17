#pragma once

// Encoding and decoding of protocol messages. Specification: docs/protocol.md
//
// Decoding never trusts its input: every length is checked against the
// buffer and the protocol limits, and all failures are returned as values.

#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "protocol/protocol.h"

namespace protocol {

// ── Payloads ────────────────────────────────────────────────────────────

struct KeyPayload {  // GetRequest, DeleteRequest, LocateRequest
    std::string key;
};

struct KeyValuePayload {  // PutRequest
    std::string key;
    std::string value;
};

struct ReplicaPutPayload {  // ReplicaPut: a copy of one write, with its version
    std::string key;
    std::string value;
    std::uint64_t version = 0;
};

struct ReplicaDeletePayload {  // ReplicaDelete: a tombstone, with its version
    std::string key;
    std::uint64_t version = 0;
};

struct ValuePayload {  // GetResponse with status Ok
    std::string value;
};

struct HelloPayload {  // Hello (the node id travels in header.sender_id)
    std::uint16_t udp_port = 0;
    std::uint16_t tcp_port = 0;
};

// STATUS / LOCATE: how the answering node sees another node.
enum class NodeState : std::uint8_t {
    Self = 0,          // the answering node itself
    Connected = 1,     // control connection up and answering heartbeats
    Connecting = 2,    // trying to (re)connect
    Disconnected = 3,  // down, waiting for it to connect to us
};
inline constexpr std::uint8_t kMaxNodeState = 3;

struct NodeStatus {
    std::uint64_t id = 0;
    std::string address;  // "[::1]:9001", at most kMaxAddressSize bytes
    NodeState state = NodeState::Disconnected;
    bool operator==(const NodeStatus&) const = default;
};

struct StatusPayload {  // StatusResponse
    std::uint64_t node_id = 0;              // the answering node
    std::uint32_t key_count = 0;            // keys stored on it (primary + replica copies)
    std::uint8_t replication_factor = 0;
    std::vector<NodeStatus> nodes;          // every node, the answering one first (Self)
};

struct LocatePayload {  // LocateResponse with status Ok
    std::uint64_t key_hash = 0;             // the key's position in the id space
    std::vector<NodeStatus> nodes;          // replica set: primary first
};

// ── Framing ─────────────────────────────────────────────────────────────

// Builds a complete message: the header (with `length` filled in) + payload.
// Throws std::length_error if the payload exceeds kMaxPayloadSize.
[[nodiscard]] std::vector<std::byte> encode_message(Header header,
                                                    std::span<const std::byte> payload = {});

// TCP: parses one message from the start of a byte stream.
//   frame          a complete message; remove frame.size bytes from the buffer
//   std::nullopt   not enough bytes yet; wait for more data
//   error          the stream is invalid; close the connection
[[nodiscard]] std::expected<std::optional<Frame>, DecodeError>
try_parse_message(std::span<const std::byte> data);

// UDP: a datagram must contain exactly one complete message.
[[nodiscard]] std::expected<Frame, DecodeError> parse_datagram(std::span<const std::byte> datagram);

// ── Payload encoding / decoding ─────────────────────────────────────────
// Encoding throws std::length_error if a key or value exceeds its limit.

[[nodiscard]] std::vector<std::byte> encode_payload(const KeyPayload& p);
[[nodiscard]] std::vector<std::byte> encode_payload(const KeyValuePayload& p);
[[nodiscard]] std::vector<std::byte> encode_payload(const ValuePayload& p);
[[nodiscard]] std::vector<std::byte> encode_payload(const HelloPayload& p);
[[nodiscard]] std::vector<std::byte> encode_payload(const ReplicaPutPayload& p);
[[nodiscard]] std::vector<std::byte> encode_payload(const ReplicaDeletePayload& p);
[[nodiscard]] std::vector<std::byte> encode_payload(const StatusPayload& p);
[[nodiscard]] std::vector<std::byte> encode_payload(const LocatePayload& p);

[[nodiscard]] std::expected<KeyPayload, DecodeError> decode_key(std::span<const std::byte> payload);
[[nodiscard]] std::expected<KeyValuePayload, DecodeError> decode_key_value(std::span<const std::byte> payload);
[[nodiscard]] std::expected<ValuePayload, DecodeError> decode_value(std::span<const std::byte> payload);
[[nodiscard]] std::expected<HelloPayload, DecodeError> decode_hello(std::span<const std::byte> payload);
[[nodiscard]] std::expected<ReplicaPutPayload, DecodeError> decode_replica_put(std::span<const std::byte> payload);
[[nodiscard]] std::expected<ReplicaDeletePayload, DecodeError> decode_replica_delete(std::span<const std::byte> payload);
[[nodiscard]] std::expected<StatusPayload, DecodeError> decode_status(std::span<const std::byte> payload);
[[nodiscard]] std::expected<LocatePayload, DecodeError> decode_locate(std::span<const std::byte> payload);

[[nodiscard]] std::string_view to_string(NodeState state) noexcept;

}  // namespace protocol
