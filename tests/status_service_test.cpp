#include <gtest/gtest.h>

#include <string>

#include "cluster/hash.h"
#include "cluster/membership.h"
#include "node/status_service.h"
#include "protocol/codec.h"
#include "storage/kv_store.h"
#include "utils/address.h"

using namespace protocol;

namespace {

cluster::NodeEntry node_at(const char* ep) { return cluster::make_node(*parse_endpoint(ep)); }

class StatusServiceTest : public ::testing::Test {
protected:
    cluster::Membership members{node_at("[::1]:9001")};
    KeyValueStore store;
    std::uint64_t id2 = node_at("[::1]:9002").id;
    std::uint64_t id3 = node_at("[::1]:9003").id;
    StatusService service{members, store, [this](std::uint64_t id) {
                              return id == id2 ? NodeState::Connected : NodeState::Connecting;
                          }};

    void SetUp() override {
        members.add(node_at("[::1]:9002"));
        members.add(node_at("[::1]:9003"));
    }

    // Sends a request through the service and parses the response.
    Frame ask(MessageType type, std::vector<std::byte> payload = {}) {
        request_ = encode_message({.type = type, .request_id = 5, .sender_id = 77}, payload);
        const auto reply = service.handle(*parse_datagram(request_));
        EXPECT_TRUE(reply.has_value());
        response_ = reply.value_or(std::vector<std::byte>{});
        return *parse_datagram(response_);
    }

private:
    std::vector<std::byte> request_, response_;
};

}  // namespace

TEST_F(StatusServiceTest, StatusDescribesEveryNode) {
    store.put("a", "1");
    store.put("b", "2");

    const Frame r = ask(MessageType::StatusRequest);
    EXPECT_EQ(r.header.type, MessageType::StatusResponse);
    EXPECT_EQ(r.header.status, Status::Ok);
    EXPECT_EQ(r.header.request_id, 5u);  // the client can match it
    EXPECT_EQ(r.header.sender_id, 77u);

    const auto st = decode_status(r.payload);
    ASSERT_TRUE(st.has_value());
    EXPECT_EQ(st->node_id, members.self().id);
    EXPECT_EQ(st->key_count, 2u);
    EXPECT_EQ(st->replication_factor, cluster::kReplicationFactor);
    ASSERT_EQ(st->nodes.size(), 3u);
    EXPECT_EQ(st->nodes[0], (NodeStatus{members.self().id, "[::1]:9001", NodeState::Self}));
    EXPECT_EQ(st->nodes[1], (NodeStatus{id2, "[::1]:9002", NodeState::Connected}));
    EXPECT_EQ(st->nodes[2], (NodeStatus{id3, "[::1]:9003", NodeState::Connecting}));
}

TEST_F(StatusServiceTest, LocateReturnsTheReplicaSetPrimaryFirst) {
    const Frame r = ask(MessageType::LocateRequest, encode_payload(KeyPayload{"student101"}));
    EXPECT_EQ(r.header.type, MessageType::LocateResponse);
    ASSERT_EQ(r.header.status, Status::Ok);

    const auto loc = decode_locate(r.payload);
    ASSERT_TRUE(loc.has_value());
    EXPECT_EQ(loc->key_hash, cluster::key_hash("student101"));

    const auto expected = members.replicas_of("student101");
    ASSERT_EQ(loc->nodes.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(loc->nodes[i].id, expected[i].id);
        EXPECT_EQ(loc->nodes[i].address, expected[i].name);
    }
}

TEST_F(StatusServiceTest, LocateWithOversizedKeyIsRejected) {
    // Build the payload by hand: encode_payload refuses oversized keys.
    std::vector<std::byte> payload{std::byte{0x01}, std::byte{0x00}};  // length 256
    payload.resize(2 + 256, std::byte{'k'});
    const Frame r = ask(MessageType::LocateRequest, payload);
    EXPECT_EQ(r.header.status, Status::KeyTooLarge);
    EXPECT_EQ(r.payload.size(), 0u);
}

TEST_F(StatusServiceTest, OtherMessagesAreLeftToTheRouter) {
    const auto msg = encode_message({.type = MessageType::GetRequest}, encode_payload(KeyPayload{"a"}));
    EXPECT_FALSE(service.handle(*parse_datagram(msg)).has_value());
}
