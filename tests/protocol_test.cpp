#include <gtest/gtest.h>

#include <cstddef>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "protocol/byte_io.h"
#include "protocol/codec.h"

using namespace protocol;

namespace {

std::vector<std::byte> make_put(std::uint64_t request_id = 12345, std::uint64_t sender_id = 42) {
    const auto payload = encode_payload(KeyValuePayload{"student101", "John"});
    return encode_message({.type = MessageType::PutRequest,
                           .request_id = request_id,
                           .sender_id = sender_id},
                          payload);
}

std::vector<std::byte> concat(const std::vector<std::byte>& a, const std::vector<std::byte>& b) {
    std::vector<std::byte> out(a);
    out.insert(out.end(), b.begin(), b.end());
    return out;
}

}  // namespace

// ── Byte order ──────────────────────────────────────────────────────────

TEST(ByteIO, WritesBigEndian) {
    ByteWriter w;
    w.u16(0x0102);
    w.u32(0x03040506);
    const auto bytes = w.take();
    ASSERT_EQ(bytes.size(), 6u);
    EXPECT_EQ(bytes[0], std::byte{0x01});
    EXPECT_EQ(bytes[1], std::byte{0x02});
    EXPECT_EQ(bytes[2], std::byte{0x03});
    EXPECT_EQ(bytes[5], std::byte{0x06});
}

TEST(ByteIO, ReaderNeverReadsPastEnd) {
    const std::vector<std::byte> data{std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};
    ByteReader r(data);
    EXPECT_EQ(r.u16(), 0x0102);
    EXPECT_FALSE(r.u16().has_value());  // only 1 byte left
    EXPECT_EQ(r.u8(), 0x03);
    EXPECT_TRUE(r.empty());
}

// ── Framing ─────────────────────────────────────────────────────────────

TEST(Codec, HeaderStartsWithMagicAndHasFixedSize) {
    const auto msg = encode_message({.type = MessageType::Ping});
    ASSERT_EQ(msg.size(), kHeaderSize);
    EXPECT_EQ(msg[0], std::byte{0x4B});  // 'K'
    EXPECT_EQ(msg[1], std::byte{0x56});  // 'V'
    EXPECT_EQ(msg[2], std::byte{kVersion});
}

TEST(Codec, PutRequestRoundTrip) {
    const auto msg = make_put(12345, 42);

    const auto frame = parse_datagram(msg);
    ASSERT_TRUE(frame.has_value()) << to_string(frame.error());
    EXPECT_EQ(frame->header.type, MessageType::PutRequest);
    EXPECT_EQ(frame->header.request_id, 12345u);
    EXPECT_EQ(frame->header.sender_id, 42u);
    EXPECT_EQ(frame->header.status, Status::Ok);
    EXPECT_EQ(frame->size, msg.size());

    const auto put = decode_key_value(frame->payload);
    ASSERT_TRUE(put.has_value());
    EXPECT_EQ(put->key, "student101");
    EXPECT_EQ(put->value, "John");
}

TEST(Codec, GetResponseWithStatusAndValue) {
    const auto msg = encode_message({.type = response_for(MessageType::GetRequest),
                                     .status = Status::Ok,
                                     .request_id = 7},
                                    encode_payload(ValuePayload{"John"}));
    const auto frame = parse_datagram(msg);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->header.type, MessageType::GetResponse);
    EXPECT_EQ(decode_value(frame->payload)->value, "John");
}

TEST(Codec, NotFoundResponseHasNoPayload) {
    const auto msg = encode_message({.type = MessageType::GetResponse, .status = Status::NotFound});
    const auto frame = parse_datagram(msg);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->header.status, Status::NotFound);
    EXPECT_TRUE(frame->payload.empty());
}

TEST(Codec, ForwardedFlagSurvivesRoundTrip) {
    const auto msg = encode_message({.type = MessageType::GetRequest, .flags = flags::kForwarded},
                                    encode_payload(KeyPayload{"k"}));
    const auto frame = parse_datagram(msg);
    ASSERT_TRUE(frame.has_value());
    EXPECT_TRUE(frame->header.flags & flags::kForwarded);
}

TEST(Codec, HelloRoundTrip) {
    const auto msg = encode_message({.type = MessageType::Hello, .sender_id = 0xABCDEF},
                                    encode_payload(HelloPayload{5222, 6222}));
    const auto parsed = try_parse_message(msg);
    ASSERT_TRUE(parsed.has_value() && parsed->has_value());
    EXPECT_EQ((*parsed)->header.sender_id, 0xABCDEFu);

    const auto hello = decode_hello((*parsed)->payload);
    ASSERT_TRUE(hello.has_value());
    EXPECT_EQ(hello->udp_port, 5222);
    EXPECT_EQ(hello->tcp_port, 6222);
}

TEST(Codec, LargestPutFitsInOneDatagram) {
    const std::string key(kMaxKeySize, 'k');
    const std::string value(kMaxValueSize, 'v');
    const auto msg = encode_message({.type = MessageType::PutRequest},
                                    encode_payload(KeyValuePayload{key, value}));
    EXPECT_LE(msg.size(), kMaxDatagramSize);
    EXPECT_TRUE(parse_datagram(msg).has_value());
}

TEST(Codec, EncodingOversizedKeyThrows) {
    EXPECT_THROW((void)encode_payload(KeyPayload{std::string(kMaxKeySize + 1, 'k')}),
                 std::length_error);
}

// ── TCP stream parsing ──────────────────────────────────────────────────

TEST(Codec, TcpWaitsForEveryMissingByte) {
    const auto msg = make_put();
    for (std::size_t n = 0; n < msg.size(); ++n) {
        const auto parsed = try_parse_message(std::span(msg).first(n));
        ASSERT_TRUE(parsed.has_value()) << "error at prefix length " << n;
        EXPECT_FALSE(parsed->has_value()) << "complete frame at prefix length " << n;
    }
    const auto full = try_parse_message(msg);
    ASSERT_TRUE(full.has_value() && full->has_value());
    EXPECT_EQ((*full)->size, msg.size());
}

TEST(Codec, TcpParsesTwoMessagesFromOneBuffer) {
    const auto stream = concat(make_put(1), make_put(2));

    const auto first = try_parse_message(stream);
    ASSERT_TRUE(first.has_value() && first->has_value());
    EXPECT_EQ((*first)->header.request_id, 1u);

    const auto second = try_parse_message(std::span(stream).subspan((*first)->size));
    ASSERT_TRUE(second.has_value() && second->has_value());
    EXPECT_EQ((*second)->header.request_id, 2u);
}

// ── Rejecting bad input ─────────────────────────────────────────────────

TEST(Codec, RejectsBadMagicImmediately) {
    std::vector<std::byte> garbage{std::byte{'G'}, std::byte{'E'}};  // "GE" from "GET /..."
    const auto parsed = try_parse_message(garbage);
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error(), DecodeError::BadMagic);
}

TEST(Codec, RejectsWrongVersion) {
    auto msg = make_put();
    msg[2] = std::byte{99};
    EXPECT_EQ(parse_datagram(msg).error(), DecodeError::BadVersion);
}

TEST(Codec, RejectsUnknownType) {
    auto msg = make_put();
    msg[3] = std::byte{0x7F};
    EXPECT_EQ(parse_datagram(msg).error(), DecodeError::UnknownType);
}

TEST(Codec, RejectsInvalidStatus) {
    auto msg = make_put();
    msg[5] = std::byte{200};
    EXPECT_EQ(parse_datagram(msg).error(), DecodeError::BadStatus);
}

TEST(Codec, RejectsHugeLengthFromHeaderAlone) {
    // Only the header arrived, claiming a 1 GiB payload. Must fail now,
    // not wait (and buffer) forever.
    auto msg = encode_message({.type = MessageType::ReplicaPut});
    msg[24] = std::byte{0x40};  // length = 0x40000000
    const auto parsed = try_parse_message(msg);
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error(), DecodeError::PayloadTooLarge);
}

TEST(Codec, DatagramMustNotBeTruncated) {
    const auto msg = make_put();
    const auto result = parse_datagram(std::span(msg).first(msg.size() - 1));
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), DecodeError::Truncated);
}

TEST(Codec, DatagramMustNotHaveTrailingBytes) {
    auto msg = make_put();
    msg.push_back(std::byte{0});
    EXPECT_EQ(parse_datagram(msg).error(), DecodeError::Malformed);
}

TEST(Codec, PayloadWithTrailingBytesIsMalformed) {
    auto payload = encode_payload(KeyPayload{"key"});
    payload.push_back(std::byte{0});
    EXPECT_EQ(decode_key(payload).error(), DecodeError::Malformed);
}

TEST(Codec, PayloadLengthLargerThanDataIsMalformed) {
    ByteWriter w;
    w.u16(10);       // claims a 10-byte key...
    w.bytes("abc");  // ...but only 3 bytes follow
    EXPECT_EQ(decode_key(w.take()).error(), DecodeError::Malformed);
}

TEST(Codec, OversizedKeyIsRejectedWhenDecoding) {
    ByteWriter w;
    w.u16(static_cast<std::uint16_t>(kMaxKeySize + 1));
    w.bytes(std::string(kMaxKeySize + 1, 'k'));
    EXPECT_EQ(decode_key(w.take()).error(), DecodeError::KeyTooLarge);
}
