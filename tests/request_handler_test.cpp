#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "node/request_handler.h"
#include "protocol/byte_io.h"
#include "protocol/codec.h"
#include "storage/kv_store.h"

using namespace protocol;

namespace {

// Encodes a request, then sends it through the handler exactly the way the
// node does: parse the datagram first, then handle the frame.
class RequestHandlerTest : public ::testing::Test {
protected:
    KeyValueStore store;
    RequestHandler handler{store};
    std::uint64_t next_id = 1;

    std::optional<RequestHandler::Response> send(MessageType type, std::vector<std::byte> payload) {
        request_bytes_ = encode_message(
            {.type = type, .request_id = next_id++, .sender_id = 42}, payload);
        const auto frame = parse_datagram(request_bytes_);
        EXPECT_TRUE(frame.has_value());
        return handler.handle(*frame);
    }

    auto put(const std::string& key, const std::string& value) {
        return send(MessageType::PutRequest, encode_payload(KeyValuePayload{key, value}));
    }
    auto get(const std::string& key) {
        return send(MessageType::GetRequest, encode_payload(KeyPayload{key}));
    }
    auto del(const std::string& key) {
        return send(MessageType::DeleteRequest, encode_payload(KeyPayload{key}));
    }

    // The response must itself be a valid datagram; returns its frame.
    // (The frame points into `response`, so `response` must stay alive.)
    static Frame parse_response(const RequestHandler::Response& response) {
        const auto frame = parse_datagram(response.message);
        EXPECT_TRUE(frame.has_value());
        return *frame;
    }

private:
    std::vector<std::byte> request_bytes_;  // keeps the parsed frame's payload valid
};

}  // namespace

TEST_F(RequestHandlerTest, PutThenGetReturnsValue) {
    const auto put_resp = put("student101", "John");
    ASSERT_TRUE(put_resp.has_value());
    EXPECT_EQ(put_resp->status, Status::Ok);

    const auto get_resp = get("student101");
    ASSERT_TRUE(get_resp.has_value());
    EXPECT_EQ(get_resp->status, Status::Ok);

    const Frame frame = parse_response(*get_resp);
    EXPECT_EQ(frame.header.type, MessageType::GetResponse);
    EXPECT_EQ(decode_value(frame.payload)->value, "John");
}

TEST_F(RequestHandlerTest, ResponseCopiesRequestIdentity) {
    next_id = 777;
    const auto resp = put("k", "v");
    ASSERT_TRUE(resp.has_value());

    const Frame frame = parse_response(*resp);
    EXPECT_EQ(frame.header.type, MessageType::PutResponse);
    EXPECT_EQ(frame.header.request_id, 777u);
    EXPECT_EQ(frame.header.sender_id, 42u);
}

TEST_F(RequestHandlerTest, GetMissingKeyIsNotFound) {
    const auto resp = get("missing");
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(resp->status, Status::NotFound);
    EXPECT_TRUE(parse_response(*resp).payload.empty());
}

TEST_F(RequestHandlerTest, PutOverwrites) {
    (void)put("k", "first");
    (void)put("k", "second");
    const auto resp = get("k");
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(decode_value(parse_response(*resp).payload)->value, "second");
}

TEST_F(RequestHandlerTest, DeleteRemovesKey) {
    (void)put("k", "v");

    const auto first = del("k");
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->status, Status::Ok);
    EXPECT_EQ(parse_response(*first).header.type, MessageType::DeleteResponse);

    EXPECT_EQ(get("k")->status, Status::NotFound);
    EXPECT_EQ(del("k")->status, Status::NotFound);  // already deleted
}

TEST_F(RequestHandlerTest, MalformedPayloadIsInvalidRequest) {
    const std::vector<std::byte> garbage{std::byte{0xFF}};  // not a valid key encoding
    const auto resp = send(MessageType::GetRequest, garbage);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(resp->status, Status::InvalidRequest);
}

TEST_F(RequestHandlerTest, OversizedValueIsReported) {
    // Built by hand: the encoder itself refuses oversized values.
    ByteWriter w;
    w.u16(1);
    w.bytes("k");
    w.u32(static_cast<std::uint32_t>(kMaxValueSize + 1));
    w.bytes(std::string(kMaxValueSize + 1, 'v'));

    const auto resp = send(MessageType::PutRequest, w.take());
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(resp->status, Status::ValueTooLarge);
    EXPECT_EQ(store.size(), 0u);  // nothing was stored
}

TEST_F(RequestHandlerTest, NonRequestMessagesAreIgnored) {
    EXPECT_FALSE(send(MessageType::Ping, {}).has_value());
    EXPECT_FALSE(send(MessageType::GetResponse, {}).has_value());
}
