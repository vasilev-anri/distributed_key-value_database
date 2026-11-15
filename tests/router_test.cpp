// Three complete nodes (membership, store, request handler, router) connected
// by a simulated network in memory: no sockets, deterministic.

#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <poll.h>
#include <sys/epoll.h>

#include "cluster/membership.h"
#include "handlers/timer_queue.h"
#include "node/request_handler.h"
#include "node/router.h"
#include "protocol/codec.h"
#include "storage/kv_store.h"
#include "utils/address.h"

using namespace protocol;
using namespace std::chrono_literals;

namespace {

bool same(const sockaddr_in6& a, const sockaddr_in6& b) {
    return a.sin6_port == b.sin6_port &&
           std::memcmp(&a.sin6_addr, &b.sin6_addr, sizeof a.sin6_addr) == 0;
}

class ClusterTest : public ::testing::Test {
protected:
    struct Packet {
        std::vector<std::byte> data;
        sockaddr_in6 from;
        sockaddr_in6 to;
    };

    struct Node {
        sockaddr_in6 addr;
        std::unique_ptr<cluster::Membership> members;
        KeyValueStore store;
        std::unique_ptr<RequestHandler> requests;
        std::unique_ptr<Router> router;
        bool alive = true;
    };

    const sockaddr_in6 client = *parse_endpoint("[::1]:5555");
    TimerQueue timers;
    std::deque<Packet> network;
    std::vector<Packet> client_inbox;
    std::vector<Packet> sent;       // every packet sent on the network
    std::vector<Packet> delivered;  // every packet that reached a live node
    std::vector<std::unique_ptr<Node>> nodes;
    std::uint64_t next_request_id = 1;

    void SetUp() override {
        const char* endpoints[] = {"[::1]:9001", "[::1]:9002", "[::1]:9003"};
        for (const char* self : endpoints) {
            auto n = std::make_unique<Node>();
            n->addr = *parse_endpoint(self);
            n->members = std::make_unique<cluster::Membership>(cluster::make_node(n->addr));
            for (const char* peer : endpoints)
                if (std::string(peer) != self) n->members->add(cluster::make_node(*parse_endpoint(peer)));
            n->requests = std::make_unique<RequestHandler>(n->store);

            const sockaddr_in6 from = n->addr;
            n->router = std::make_unique<Router>(
                *n->members, *n->requests, timers,
                [this, from](std::span<const std::byte> data, const sockaddr_in6& to) {
                    network.push_back({{data.begin(), data.end()}, from, to});
                    return true;
                },
                20ms);
            nodes.push_back(std::move(n));
        }
    }

    // Delivers queued packets until the network is quiet.
    void deliver() {
        while (!network.empty()) {
            Packet p = std::move(network.front());
            network.pop_front();
            sent.push_back(p);
            if (same(p.to, client)) {
                client_inbox.push_back(std::move(p));
                continue;
            }
            for (auto& n : nodes) {
                if (same(n->addr, p.to) && n->alive) {
                    delivered.push_back(p);
                    n->router->on_datagram(p.data, p.from);
                }
            }
        }
    }

    // The client sends a request to `entry` and the network runs.
    std::uint64_t send(std::size_t entry, MessageType type, std::vector<std::byte> payload,
                       std::uint8_t flags = 0) {
        const std::uint64_t id = next_request_id++;
        const auto msg = encode_message(
            {.type = type, .flags = flags, .request_id = id, .sender_id = 777}, payload);
        nodes[entry]->router->on_datagram(msg, client);
        deliver();
        return id;
    }
    std::uint64_t put(std::size_t entry, const std::string& k, const std::string& v) {
        return send(entry, MessageType::PutRequest, encode_payload(KeyValuePayload{k, v}));
    }
    std::uint64_t get(std::size_t entry, const std::string& k) {
        return send(entry, MessageType::GetRequest, encode_payload(KeyPayload{k}));
    }

    // Index of the node that owns `key` (all nodes agree).
    std::size_t owner_index(const std::string& key) {
        const auto id = nodes[0]->members->owner_of(key).id;
        for (std::size_t i = 0; i < nodes.size(); ++i)
            if (nodes[i]->members->self().id == id) return i;
        return nodes.size();
    }

    // A key owned by node `index`.
    std::string key_owned_by(std::size_t index) {
        for (int i = 0;; ++i) {
            std::string key = "key" + std::to_string(i);
            if (owner_index(key) == index) return key;
        }
    }

    // The last response the client received, parsed.
    Frame last_response() {
        const auto frame = parse_datagram(client_inbox.back().data);
        EXPECT_TRUE(frame.has_value());
        return *frame;
    }

    // Lets the TimerQueue fire, like the reactor would.
    void wait_for_timers() {
        pollfd p{.fd = timers.get_fd(), .events = POLLIN, .revents = 0};
        ASSERT_EQ(::poll(&p, 1, 1000), 1) << "timer never fired";
        timers.handle_event(EPOLLIN);
        deliver();
    }

    std::size_t packets_sent_to(std::size_t index) const {
        std::size_t n = 0;
        for (const auto& p : sent) n += same(p.to, nodes[index]->addr);
        return n;
    }
};

}  // namespace

TEST_F(ClusterTest, RequestToOwnerIsHandledLocally) {
    const std::string key = key_owned_by(1);
    const auto id = put(1, key, "John");

    ASSERT_EQ(client_inbox.size(), 1u);
    const Frame r = last_response();
    EXPECT_EQ(r.header.status, Status::Ok);
    EXPECT_EQ(r.header.request_id, id);
    EXPECT_EQ(nodes[1]->store.get(key), "John");
    EXPECT_TRUE(delivered.empty());  // no node-to-node traffic at all
}

TEST_F(ClusterTest, PutThroughNonOwnerLandsOnOwner) {
    const std::string key = key_owned_by(2);
    const auto id = put(0, key, "John");  // enters at node 0

    ASSERT_EQ(client_inbox.size(), 1u);
    const Frame r = last_response();
    EXPECT_EQ(r.header.type, MessageType::PutResponse);
    EXPECT_EQ(r.header.status, Status::Ok);
    EXPECT_EQ(r.header.request_id, id);  // the client can match it
    EXPECT_EQ(r.header.sender_id, 777u);

    EXPECT_EQ(nodes[2]->store.get(key), "John");  // stored on the owner...
    EXPECT_EQ(nodes[0]->store.size(), 0u);        // ...and nowhere else
    EXPECT_EQ(nodes[1]->store.size(), 0u);
    EXPECT_EQ(nodes[0]->router->pending_forwards(), 0u);
}

TEST_F(ClusterTest, AnyNodeCanReadAnyKey) {
    const std::string key = key_owned_by(2);
    put(0, key, "John");

    for (std::size_t entry = 0; entry < 3; ++entry) {
        get(entry, key);
        const Frame r = last_response();
        ASSERT_EQ(r.header.status, Status::Ok) << "via node " << entry;
        EXPECT_EQ(decode_value(r.payload)->value, "John");
    }
}

TEST_F(ClusterTest, ForwardedRequestIsNeverForwardedAgain) {
    const std::string key = key_owned_by(2);
    // Arrives at node 0 already marked FORWARDED, but node 0 isn't the owner.
    send(0, MessageType::GetRequest, encode_payload(KeyPayload{key}), flags::kForwarded);

    EXPECT_EQ(last_response().header.status, Status::Unavailable);
    EXPECT_EQ(packets_sent_to(2), 0u);  // nothing was forwarded
}

TEST_F(ClusterTest, DeadOwnerGivesUnavailableAfterTimeout) {
    const std::string key = key_owned_by(2);
    nodes[2]->alive = false;

    put(0, key, "John");
    EXPECT_TRUE(client_inbox.empty());  // nothing yet: waiting for node 2
    EXPECT_EQ(nodes[0]->router->pending_forwards(), 1u);

    wait_for_timers();

    ASSERT_EQ(client_inbox.size(), 1u);
    EXPECT_EQ(last_response().header.status, Status::Unavailable);
    EXPECT_EQ(nodes[0]->router->pending_forwards(), 0u);
}

TEST_F(ClusterTest, ClientRetryIsNotForwardedTwice) {
    const std::string key = key_owned_by(2);
    nodes[2]->alive = false;  // the first attempt stays pending

    const auto msg = encode_message({.type = MessageType::GetRequest, .request_id = 42, .sender_id = 777},
                                    encode_payload(KeyPayload{key}));
    nodes[0]->router->on_datagram(msg, client);
    deliver();
    nodes[0]->router->on_datagram(msg, client);  // same request again
    deliver();

    EXPECT_EQ(packets_sent_to(2), 1u);                    // forwarded once, not twice
    EXPECT_EQ(nodes[0]->router->pending_forwards(), 1u);
}

TEST_F(ClusterTest, ResponseFromWrongNodeIsIgnored) {
    const std::string key = key_owned_by(2);
    nodes[2]->alive = false;
    const auto id = get(0, key);  // pending at node 0, waiting for node 2

    // Node 1 (not the owner) sends a response with the right ids.
    const auto fake = encode_message({.type = MessageType::GetResponse, .status = Status::Ok,
                                      .request_id = id, .sender_id = 777},
                                     encode_payload(ValuePayload{"forged"}));
    nodes[0]->router->on_datagram(fake, nodes[1]->addr);
    deliver();

    EXPECT_TRUE(client_inbox.empty());
    EXPECT_EQ(nodes[0]->router->pending_forwards(), 1u);
}

TEST_F(ClusterTest, InvalidDatagramsAreDropped) {
    const std::vector<std::byte> garbage{std::byte{'h'}, std::byte{'i'}};
    nodes[0]->router->on_datagram(garbage, client);
    deliver();
    EXPECT_TRUE(client_inbox.empty());
}
