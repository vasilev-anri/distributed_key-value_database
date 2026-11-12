#pragma once

// Wire protocol constants and types. Specification: docs/protocol.md

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

namespace protocol {

// ── Constants ───────────────────────────────────────────────────────────

inline constexpr std::uint16_t kMagic = 0x4B56;  // "KV"
inline constexpr std::uint8_t kVersion = 1;
inline constexpr std::size_t kHeaderSize = 28;

// Largest UDP datagram we send or accept: fits the IPv6 minimum MTU
// (1280) minus the IPv6 (40) and UDP (8) headers, so it is never fragmented.
inline constexpr std::size_t kMaxDatagramSize = 1232;

inline constexpr std::size_t kMaxKeySize = 128;
inline constexpr std::size_t kMaxValueSize = 1024;

// Upper bound for any single message's payload (mainly TCP). Checked from the
// header alone, so a peer can't make us wait for / buffer a huge message.
inline constexpr std::size_t kMaxPayloadSize = 64 * 1024;

// ── Message types ───────────────────────────────────────────────────────

enum class MessageType : std::uint8_t {
    // UDP data channel. Response type = request type | 0x80.
    PutRequest = 0x01,
    GetRequest = 0x02,
    DeleteRequest = 0x03,
    PutResponse = 0x81,
    GetResponse = 0x82,
    DeleteResponse = 0x83,

    // TCP control channel (node <-> node).
    Hello = 0x10,
    HelloAck = 0x11,
    Ping = 0x12,
    Pong = 0x13,
    ReplicaPut = 0x20,
    ReplicaDelete = 0x21,
    ReplicaAck = 0x22,
};

constexpr MessageType response_for(MessageType request) noexcept {
    return static_cast<MessageType>(std::to_underlying(request) | 0x80);
}

// ── Status codes (responses and acks) ───────────────────────────────────

enum class Status : std::uint8_t {
    Ok = 0,
    NotFound = 1,
    InvalidRequest = 2,
    KeyTooLarge = 3,
    ValueTooLarge = 4,
    ReplicationFailed = 5,
    Unavailable = 6,
    InternalError = 7,
};

inline constexpr std::uint8_t kMaxStatus = std::to_underlying(Status::InternalError);

// ── Header flags ────────────────────────────────────────────────────────

namespace flags {
inline constexpr std::uint8_t kForwarded = 0x01;  // already forwarded once: never forward again
}

// ── Header and frame ────────────────────────────────────────────────────

struct Header {
    MessageType type{};
    std::uint8_t flags = 0;
    Status status = Status::Ok;
    std::uint64_t request_id = 0;  // chosen by the original sender
    std::uint64_t sender_id = 0;   // original sender (client id or node id)
    std::uint32_t length = 0;      // payload length; filled in by encode_message()
};

// A parsed message. `payload` points into the input buffer, so it is only
// valid as long as that buffer is.
struct Frame {
    Header header;
    std::span<const std::byte> payload;
    std::size_t size = 0;  // header + payload bytes
};

enum class DecodeError {
    BadMagic,
    BadVersion,
    UnknownType,
    BadStatus,
    PayloadTooLarge,
    Truncated,
    Malformed,
    KeyTooLarge,
    ValueTooLarge,
};

std::string_view to_string(MessageType type) noexcept;
std::string_view to_string(Status status) noexcept;
std::string_view to_string(DecodeError error) noexcept;

}  // namespace protocol
