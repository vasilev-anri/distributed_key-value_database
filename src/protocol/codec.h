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
#include <vector>

#include "protocol/protocol.h"

namespace protocol {

// ── Payloads ────────────────────────────────────────────────────────────

struct KeyPayload {  // GetRequest, DeleteRequest, ReplicaDelete
    std::string key;
};

struct KeyValuePayload {  // PutRequest, ReplicaPut
    std::string key;
    std::string value;
};

struct ValuePayload {  // GetResponse with status Ok
    std::string value;
};

struct HelloPayload {  // Hello (the node id travels in header.sender_id)
    std::uint16_t udp_port = 0;
    std::uint16_t tcp_port = 0;
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

[[nodiscard]] std::expected<KeyPayload, DecodeError> decode_key(std::span<const std::byte> payload);
[[nodiscard]] std::expected<KeyValuePayload, DecodeError> decode_key_value(std::span<const std::byte> payload);
[[nodiscard]] std::expected<ValuePayload, DecodeError> decode_value(std::span<const std::byte> payload);
[[nodiscard]] std::expected<HelloPayload, DecodeError> decode_hello(std::span<const std::byte> payload);

}  // namespace protocol
