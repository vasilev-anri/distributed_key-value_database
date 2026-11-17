#include "protocol/codec.h"

#include <stdexcept>
#include <string_view>
#include <utility>

#include "protocol/byte_io.h"

namespace protocol {

namespace {

bool is_known_type(std::uint8_t raw) noexcept {
    switch (static_cast<MessageType>(raw)) {
        case MessageType::PutRequest:
        case MessageType::GetRequest:
        case MessageType::DeleteRequest:
        case MessageType::StatusRequest:
        case MessageType::LocateRequest:
        case MessageType::PutResponse:
        case MessageType::GetResponse:
        case MessageType::DeleteResponse:
        case MessageType::StatusResponse:
        case MessageType::LocateResponse:
        case MessageType::Hello:
        case MessageType::HelloAck:
        case MessageType::Ping:
        case MessageType::Pong:
        case MessageType::ReplicaPut:
        case MessageType::ReplicaDelete:
        case MessageType::ReplicaAck:
            return true;
    }
    return false;
}

std::string to_std_string(std::span<const std::byte> data) {
    return {reinterpret_cast<const char*>(data.data()), data.size()};
}

// key   = u16 length + bytes
// value = u32 length + bytes
void write_key(ByteWriter& w, std::string_view key) {
    if (key.size() > kMaxKeySize) throw std::length_error("key exceeds kMaxKeySize");
    w.u16(static_cast<std::uint16_t>(key.size()));
    w.bytes(key);
}

void write_value(ByteWriter& w, std::string_view value) {
    if (value.size() > kMaxValueSize) throw std::length_error("value exceeds kMaxValueSize");
    w.u32(static_cast<std::uint32_t>(value.size()));
    w.bytes(value);
}

std::expected<std::string, DecodeError> read_key(ByteReader& r) {
    const auto len = r.u16();
    if (!len) return std::unexpected(DecodeError::Malformed);
    if (*len > kMaxKeySize) return std::unexpected(DecodeError::KeyTooLarge);
    const auto data = r.bytes(*len);
    if (!data) return std::unexpected(DecodeError::Malformed);
    return to_std_string(*data);
}

std::expected<std::string, DecodeError> read_value(ByteReader& r) {
    const auto len = r.u32();
    if (!len) return std::unexpected(DecodeError::Malformed);
    if (*len > kMaxValueSize) return std::unexpected(DecodeError::ValueTooLarge);
    const auto data = r.bytes(*len);
    if (!data) return std::unexpected(DecodeError::Malformed);
    return to_std_string(*data);
}

// node list = u8 count + per node: u64 id, u8 address length + bytes, u8 state
void write_nodes(ByteWriter& w, const std::vector<NodeStatus>& nodes) {
    if (nodes.size() > kMaxStatusNodes) throw std::length_error("too many nodes for one datagram");
    w.u8(static_cast<std::uint8_t>(nodes.size()));
    for (const NodeStatus& n : nodes) {
        if (n.address.size() > kMaxAddressSize) throw std::length_error("address exceeds kMaxAddressSize");
        w.u64(n.id);
        w.u8(static_cast<std::uint8_t>(n.address.size()));
        w.bytes(n.address);
        w.u8(std::to_underlying(n.state));
    }
}

std::expected<std::vector<NodeStatus>, DecodeError> read_nodes(ByteReader& r) {
    const auto count = r.u8();
    if (!count || *count > kMaxStatusNodes) return std::unexpected(DecodeError::Malformed);

    std::vector<NodeStatus> nodes;
    for (std::uint8_t i = 0; i < *count; ++i) {
        const auto id = r.u64();
        const auto len = r.u8();
        if (!id || !len || *len > kMaxAddressSize) return std::unexpected(DecodeError::Malformed);
        const auto address = r.bytes(*len);
        const auto state = r.u8();
        if (!address || !state || *state > kMaxNodeState) return std::unexpected(DecodeError::Malformed);
        nodes.push_back({*id, to_std_string(*address), static_cast<NodeState>(*state)});
    }
    return nodes;
}

// A payload must be consumed completely; leftover bytes mean it's malformed.
template <typename T>
std::expected<T, DecodeError> finish(const ByteReader& r, T value) {
    if (!r.empty()) return std::unexpected(DecodeError::Malformed);
    return value;
}

}  // namespace

// ── Framing ─────────────────────────────────────────────────────────────

std::vector<std::byte> encode_message(Header header, std::span<const std::byte> payload) {
    if (payload.size() > kMaxPayloadSize) throw std::length_error("payload exceeds kMaxPayloadSize");

    ByteWriter w;
    w.u16(kMagic);
    w.u8(kVersion);
    w.u8(std::to_underlying(header.type));
    w.u8(header.flags);
    w.u8(std::to_underlying(header.status));
    w.u16(0);  // reserved
    w.u64(header.request_id);
    w.u64(header.sender_id);
    w.u32(static_cast<std::uint32_t>(payload.size()));
    w.bytes(payload);
    return w.take();
}

std::expected<std::optional<Frame>, DecodeError>
try_parse_message(std::span<const std::byte> data) {
    // Reject garbage as early as possible, even before the full header arrives.
    if (data.size() >= 2) {
        ByteReader magic_reader(data.first(2));
        if (*magic_reader.u16() != kMagic) return std::unexpected(DecodeError::BadMagic);
    }
    if (data.size() < kHeaderSize) return std::optional<Frame>{};  // need more

    ByteReader r(data.first(kHeaderSize));
    r.u16();  // magic, checked above
    const std::uint8_t version = *r.u8();
    const std::uint8_t type = *r.u8();
    const std::uint8_t flag_bits = *r.u8();
    const std::uint8_t status = *r.u8();
    r.u16();  // reserved: ignored, so future versions can use it
    const std::uint64_t request_id = *r.u64();
    const std::uint64_t sender_id = *r.u64();
    const std::uint32_t length = *r.u32();

    if (version != kVersion) return std::unexpected(DecodeError::BadVersion);
    if (!is_known_type(type)) return std::unexpected(DecodeError::UnknownType);
    if (status > kMaxStatus) return std::unexpected(DecodeError::BadStatus);
    if (length > kMaxPayloadSize) return std::unexpected(DecodeError::PayloadTooLarge);

    if (data.size() < kHeaderSize + length) return std::optional<Frame>{};  // need more

    Frame frame;
    frame.header.type = static_cast<MessageType>(type);
    frame.header.flags = flag_bits;
    frame.header.status = static_cast<Status>(status);
    frame.header.request_id = request_id;
    frame.header.sender_id = sender_id;
    frame.header.length = length;
    frame.payload = data.subspan(kHeaderSize, length);
    frame.size = kHeaderSize + length;
    return frame;
}

std::expected<Frame, DecodeError> parse_datagram(std::span<const std::byte> datagram) {
    if (datagram.size() > kMaxDatagramSize) return std::unexpected(DecodeError::PayloadTooLarge);

    const auto result = try_parse_message(datagram);
    if (!result) return std::unexpected(result.error());
    if (!*result) return std::unexpected(DecodeError::Truncated);  // a datagram never "continues"

    const Frame& frame = **result;
    if (frame.size != datagram.size()) return std::unexpected(DecodeError::Malformed);  // trailing bytes
    return frame;
}

// ── Payloads ────────────────────────────────────────────────────────────

std::vector<std::byte> encode_payload(const KeyPayload& p) {
    ByteWriter w;
    write_key(w, p.key);
    return w.take();
}

std::vector<std::byte> encode_payload(const KeyValuePayload& p) {
    ByteWriter w;
    write_key(w, p.key);
    write_value(w, p.value);
    return w.take();
}

std::vector<std::byte> encode_payload(const ValuePayload& p) {
    ByteWriter w;
    write_value(w, p.value);
    return w.take();
}

std::vector<std::byte> encode_payload(const HelloPayload& p) {
    ByteWriter w;
    w.u16(p.udp_port);
    w.u16(p.tcp_port);
    return w.take();
}

std::expected<KeyPayload, DecodeError> decode_key(std::span<const std::byte> payload) {
    ByteReader r(payload);
    auto key = read_key(r);
    if (!key) return std::unexpected(key.error());
    return finish(r, KeyPayload{std::move(*key)});
}

std::expected<KeyValuePayload, DecodeError> decode_key_value(std::span<const std::byte> payload) {
    ByteReader r(payload);
    auto key = read_key(r);
    if (!key) return std::unexpected(key.error());
    auto value = read_value(r);
    if (!value) return std::unexpected(value.error());
    return finish(r, KeyValuePayload{std::move(*key), std::move(*value)});
}

std::expected<ValuePayload, DecodeError> decode_value(std::span<const std::byte> payload) {
    ByteReader r(payload);
    auto value = read_value(r);
    if (!value) return std::unexpected(value.error());
    return finish(r, ValuePayload{std::move(*value)});
}

std::expected<HelloPayload, DecodeError> decode_hello(std::span<const std::byte> payload) {
    ByteReader r(payload);
    const auto udp = r.u16();
    const auto tcp = r.u16();
    if (!udp || !tcp) return std::unexpected(DecodeError::Malformed);
    return finish(r, HelloPayload{*udp, *tcp});
}

std::vector<std::byte> encode_payload(const ReplicaPutPayload& p) {
    ByteWriter w;
    write_key(w, p.key);
    write_value(w, p.value);
    w.u64(p.version);
    return w.take();
}

std::vector<std::byte> encode_payload(const ReplicaDeletePayload& p) {
    ByteWriter w;
    write_key(w, p.key);
    w.u64(p.version);
    return w.take();
}

std::expected<ReplicaPutPayload, DecodeError> decode_replica_put(std::span<const std::byte> payload) {
    ByteReader r(payload);
    auto key = read_key(r);
    if (!key) return std::unexpected(key.error());
    auto value = read_value(r);
    if (!value) return std::unexpected(value.error());
    const auto version = r.u64();
    if (!version) return std::unexpected(DecodeError::Malformed);
    return finish(r, ReplicaPutPayload{std::move(*key), std::move(*value), *version});
}

std::expected<ReplicaDeletePayload, DecodeError> decode_replica_delete(std::span<const std::byte> payload) {
    ByteReader r(payload);
    auto key = read_key(r);
    if (!key) return std::unexpected(key.error());
    const auto version = r.u64();
    if (!version) return std::unexpected(DecodeError::Malformed);
    return finish(r, ReplicaDeletePayload{std::move(*key), *version});
}

std::vector<std::byte> encode_payload(const StatusPayload& p) {
    ByteWriter w;
    w.u64(p.node_id);
    w.u32(p.key_count);
    w.u8(p.replication_factor);
    write_nodes(w, p.nodes);
    return w.take();
}

std::vector<std::byte> encode_payload(const LocatePayload& p) {
    ByteWriter w;
    w.u64(p.key_hash);
    write_nodes(w, p.nodes);
    return w.take();
}

std::expected<StatusPayload, DecodeError> decode_status(std::span<const std::byte> payload) {
    ByteReader r(payload);
    const auto node_id = r.u64();
    const auto key_count = r.u32();
    const auto rf = r.u8();
    if (!node_id || !key_count || !rf) return std::unexpected(DecodeError::Malformed);
    auto nodes = read_nodes(r);
    if (!nodes) return std::unexpected(nodes.error());
    return finish(r, StatusPayload{*node_id, *key_count, *rf, std::move(*nodes)});
}

std::expected<LocatePayload, DecodeError> decode_locate(std::span<const std::byte> payload) {
    ByteReader r(payload);
    const auto key_hash = r.u64();
    if (!key_hash) return std::unexpected(DecodeError::Malformed);
    auto nodes = read_nodes(r);
    if (!nodes) return std::unexpected(nodes.error());
    return finish(r, LocatePayload{*key_hash, std::move(*nodes)});
}

// ── Names for logs ──────────────────────────────────────────────────────

std::string_view to_string(MessageType type) noexcept {
    switch (type) {
        case MessageType::PutRequest: return "PUT";
        case MessageType::GetRequest: return "GET";
        case MessageType::DeleteRequest: return "DELETE";
        case MessageType::StatusRequest: return "STATUS";
        case MessageType::LocateRequest: return "LOCATE";
        case MessageType::PutResponse: return "PUT_RESPONSE";
        case MessageType::GetResponse: return "GET_RESPONSE";
        case MessageType::DeleteResponse: return "DELETE_RESPONSE";
        case MessageType::StatusResponse: return "STATUS_RESPONSE";
        case MessageType::LocateResponse: return "LOCATE_RESPONSE";
        case MessageType::Hello: return "HELLO";
        case MessageType::HelloAck: return "HELLO_ACK";
        case MessageType::Ping: return "PING";
        case MessageType::Pong: return "PONG";
        case MessageType::ReplicaPut: return "REPLICA_PUT";
        case MessageType::ReplicaDelete: return "REPLICA_DELETE";
        case MessageType::ReplicaAck: return "REPLICA_ACK";
    }
    return "UNKNOWN";
}

std::string_view to_string(Status status) noexcept {
    switch (status) {
        case Status::Ok: return "OK";
        case Status::NotFound: return "NOT_FOUND";
        case Status::InvalidRequest: return "INVALID_REQUEST";
        case Status::KeyTooLarge: return "KEY_TOO_LARGE";
        case Status::ValueTooLarge: return "VALUE_TOO_LARGE";
        case Status::ReplicationFailed: return "REPLICATION_FAILED";
        case Status::Unavailable: return "UNAVAILABLE";
        case Status::InternalError: return "INTERNAL_ERROR";
    }
    return "UNKNOWN";
}

std::string_view to_string(NodeState state) noexcept {
    switch (state) {
        case NodeState::Self: return "SELF";
        case NodeState::Connected: return "CONNECTED";
        case NodeState::Connecting: return "CONNECTING";
        case NodeState::Disconnected: return "DISCONNECTED";
    }
    return "UNKNOWN";
}

std::string_view to_string(DecodeError error) noexcept {
    switch (error) {
        case DecodeError::BadMagic: return "bad magic";
        case DecodeError::BadVersion: return "unsupported version";
        case DecodeError::UnknownType: return "unknown message type";
        case DecodeError::BadStatus: return "invalid status";
        case DecodeError::PayloadTooLarge: return "payload too large";
        case DecodeError::Truncated: return "truncated message";
        case DecodeError::Malformed: return "malformed message";
        case DecodeError::KeyTooLarge: return "key too large";
        case DecodeError::ValueTooLarge: return "value too large";
    }
    return "unknown error";
}

}  // namespace protocol
