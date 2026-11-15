// Three complete nodes (membership, store, request handler, replicator, router)
// connected by a simulated network in memory: no sockets, deterministic.
// The data channel (UDP datagrams) and the control channel (replication
// messages between nodes) are both simulated.

#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <deque>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <poll.h>
#include <sys/epoll.h>

#include "cluster/membership.h"
#include "handlers/timer_queue.h"
#include "node/replicator.h"
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

    struct ControlMessage {
        std::vector<std::byte> data;
        std::uint64_t from;  // node ids
        std::uint64_t to;
    };

    struct Node {
        sockaddr_in6 addr;
        std::unique_ptr<cluster::Membership> members;
        KeyValueStore store;
        std::unique_ptr<RequestHandler> requests;
        std::unique_ptr<Replicator> replicator;
        std::unique_ptr<Router> router;
        bool alive = true;
        bool silent = false;  // connected, but its control messages get lost (hung node)
        std::uint64_t id() const { return members->self().id; }
    };

    const sockaddr_in6 client = *parse_endpoint("[::1]:5555");
    TimerQueue timers;
    std::deque<Packet> network;
    std::deque<ControlMessage> control;
    std::size_t control_sent = 0;
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

            // Control channel: "connected" to every live peer, like PeerManager.
            const std::uint64_t self_id = n->members->self().id;
            n->replicator = std::make_unique<Replicator>(
                *n->members, n->store, timers,
                [this, self_id](std::uint64_t peer, std::span<const std::byte> data) {
                    if (!node_by_id(peer).alive || !node_by_id(self_id).alive) return false;
                    control.push_back({{data.begin(), data.end()}, self_id, peer});
                    ++control_sent;
                    return true;
                },
                10ms);

            const sockaddr_in6 from = n->addr;
            n->router = std::make_unique<Router>(
                *n->members, *n->requests, *n->replicator, timers,
                [this, from](std::span<const std::byte> data, const sockaddr_in6& to) {
                    network.push_back({{data.begin(), data.end()}, from, to});
                    return true;
                },
                20ms);
            nodes.push_back(std::move(n));
        }
    }

    Node& node_by_id(std::uint64_t id) {
        for (auto& n : nodes)
            if (n->id() == id) return *n;
        throw std::logic_error("no such node");
    }

    // Delivers queued packets and control messages until both networks are quiet.
    void deliver() {
        while (!network.empty() || !control.empty()) {
            if (!control.empty()) {
                ControlMessage m = std::move(control.front());
                control.pop_front();
                Node& to = node_by_id(m.to);
                if (!to.alive || to.silent) continue;  // lost
                const auto frame = try_parse_message(m.data);
                ASSERT_TRUE(frame.has_value() && frame->has_value());
                to.replicator->on_message(m.from, **frame);
                continue;
            }

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
    std::uint64_t del(std::size_t entry, const std::string& k) {
        return send(entry, MessageType::DeleteRequest, encode_payload(KeyPayload{k}));
    }

    std::size_t index_of(std::uint64_t id) {
        for (std::size_t i = 0; i < nodes.size(); ++i)
            if (nodes[i]->id() == id) return i;
        return nodes.size();
    }

    // Index of the node holding the replica of `key`.
    std::size_t replica_index(const std::string& key) {
        return index_of(nodes[0]->members->replicas_of(key).at(1).id);
    }

    // The node that is neither primary nor replica of `key`.
    std::size_t other_index(const std::string& key) {
        return 3 - owner_index(key) - replica_index(key);
    }

    // Tells every router which nodes are down, as the control channel would.
    void use_liveness() {
        for (auto& n : nodes)
            n->router->set_liveness([this](std::uint64_t id) { return node_by_id(id).alive; });
    }

    // Index of the node that owns `key` (all nodes agree).
    std::size_t owner_index(const std::string& key) { return index_of(nodes[0]->members->owner_of(key).id); }

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

    // Lets the TimerQueue fire, like the reactor would, until the client
    // receives its next answer. (A wake-up may be for a timer that was
    // already cancelled, so one wake-up isn't enough.)
    void wait_for_timers() {
        const std::size_t answers = client_inbox.size();
        for (int i = 0; i < 50 && client_inbox.size() == answers; ++i) {
            pollfd p{.fd = timers.get_fd(), .events = POLLIN, .revents = 0};
            ASSERT_EQ(::poll(&p, 1, 1000), 1) << "timer never fired";
            timers.handle_event(EPOLLIN);
            deliver();
        }
        ASSERT_GT(client_inbox.size(), answers) << "no answer after the timers fired";
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
    EXPECT_TRUE(delivered.empty());  // no forwarding on the data channel
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

    EXPECT_EQ(nodes[2]->store.get(key), "John");  // stored on the owner
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
    const std::size_t outsider = other_index(key);  // holds no copy of the key
    // Arrives already marked FORWARDED at a node that can't serve it.
    send(outsider, MessageType::GetRequest, encode_payload(KeyPayload{key}), flags::kForwarded);

    EXPECT_EQ(last_response().header.status, Status::Unavailable);
    EXPECT_EQ(packets_sent_to(2), 0u);  // nothing was forwarded
}

TEST_F(ClusterTest, ForwardedWriteToReplicaIsRefused) {
    const std::string key = key_owned_by(2);
    // The replica may serve reads, but never executes writes: only the primary does.
    send(replica_index(key), MessageType::PutRequest, encode_payload(KeyValuePayload{key, "x"}),
         flags::kForwarded);

    EXPECT_EQ(last_response().header.status, Status::Unavailable);
    EXPECT_EQ(nodes[replica_index(key)]->store.size(), 0u);
}

TEST_F(ClusterTest, ForwardedReadToReplicaIsServed) {
    const std::string key = key_owned_by(2);
    put(2, key, "John");
    send(replica_index(key), MessageType::GetRequest, encode_payload(KeyPayload{key}), flags::kForwarded);

    EXPECT_EQ(last_response().header.status, Status::Ok);
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

// ── Replication ─────────────────────────────────────────────────────────

TEST_F(ClusterTest, PutIsStoredOnPrimaryAndReplica) {
    const std::string key = key_owned_by(2);
    put(0, key, "John");

    EXPECT_EQ(last_response().header.status, Status::Ok);
    EXPECT_EQ(nodes[owner_index(key)]->store.get(key), "John");
    EXPECT_EQ(nodes[replica_index(key)]->store.get(key), "John");
    EXPECT_EQ(nodes[other_index(key)]->store.size(), 0u);  // exactly two copies
    EXPECT_EQ(nodes[2]->router->pending_writes(), 0u);
    EXPECT_EQ(nodes[2]->replicator->pending(), 0u);
}

TEST_F(ClusterTest, DeleteRemovesBothCopies) {
    const std::string key = key_owned_by(1);
    put(0, key, "John");
    del(2, key);

    EXPECT_EQ(last_response().header.status, Status::Ok);
    EXPECT_FALSE(nodes[owner_index(key)]->store.get(key));
    EXPECT_FALSE(nodes[replica_index(key)]->store.get(key));
}

TEST_F(ClusterTest, ReadSurvivesDeadPrimary) {
    const std::string key = key_owned_by(2);
    put(0, key, "John");
    nodes[2]->alive = false;  // crashes; nobody has noticed yet

    const std::size_t entry = other_index(key);
    get(entry, key);
    EXPECT_EQ(client_inbox.size(), 1u);  // just the PUT's answer: waiting for node 2
    wait_for_timers();                    // node 2 didn't answer: ask the replica

    const Frame r = last_response();
    ASSERT_EQ(r.header.status, Status::Ok);
    EXPECT_EQ(decode_value(r.payload)->value, "John");
    EXPECT_EQ(nodes[entry]->router->pending_forwards(), 0u);
}

TEST_F(ClusterTest, KnownDeadPrimaryIsSkippedForReads) {
    use_liveness();
    const std::string key = key_owned_by(2);
    put(0, key, "John");
    nodes[2]->alive = false;  // and the control channel reports it

    const std::size_t entry = other_index(key);
    get(entry, key);  // goes straight to the replica: no timeout needed

    ASSERT_EQ(client_inbox.size(), 2u);
    EXPECT_EQ(last_response().header.status, Status::Ok);
    EXPECT_EQ(packets_sent_to(2), 1u);  // only the first PUT's forward
}

TEST_F(ClusterTest, ReplicaAnswersFromItsOwnStoreWhenPrimaryKnownDown) {
    use_liveness();
    const std::string key = key_owned_by(2);
    put(0, key, "John");
    nodes[2]->alive = false;

    const std::size_t replica = replica_index(key);
    const std::size_t before = sent.size();
    get(replica, key);

    EXPECT_EQ(last_response().header.status, Status::Ok);
    EXPECT_EQ(sent.size(), before + 1);  // just the response to the client
}

TEST_F(ClusterTest, WriteToKnownDeadPrimaryFailsFast) {
    use_liveness();
    const std::string key = key_owned_by(2);
    nodes[2]->alive = false;

    put(other_index(key), key, "John");

    ASSERT_EQ(client_inbox.size(), 1u);  // immediately, no timeout
    EXPECT_EQ(last_response().header.status, Status::Unavailable);
}

TEST_F(ClusterTest, DisconnectedReplicaGivesReplicationFailed) {
    const std::string key = key_owned_by(2);
    nodes[replica_index(key)]->alive = false;

    put(2, key, "John");  // straight to the primary

    ASSERT_EQ(client_inbox.size(), 1u);
    EXPECT_EQ(last_response().header.status, Status::ReplicationFailed);
}

TEST_F(ClusterTest, SilentReplicaGivesReplicationFailedAfterTimeout) {
    const std::string key = key_owned_by(2);
    nodes[replica_index(key)]->silent = true;

    put(2, key, "John");
    EXPECT_TRUE(client_inbox.empty());  // waiting for the ACK
    EXPECT_EQ(nodes[2]->router->pending_writes(), 1u);

    wait_for_timers();

    ASSERT_EQ(client_inbox.size(), 1u);
    EXPECT_EQ(last_response().header.status, Status::ReplicationFailed);
    EXPECT_EQ(nodes[2]->router->pending_writes(), 0u);
    EXPECT_EQ(nodes[2]->replicator->pending(), 0u);
}

TEST_F(ClusterTest, ReplicaRejectsKeysItIsNotResponsibleFor) {
    const std::string key = key_owned_by(2);
    const std::size_t outsider = other_index(key);

    // Node 2 (a genuine peer) claims `outsider` should store a copy.
    const auto msg = encode_message({.type = MessageType::ReplicaPut, .request_id = 9, .sender_id = nodes[2]->id()},
                                    encode_payload(KeyValuePayload{key, "x"}));
    nodes[outsider]->replicator->on_message(nodes[2]->id(), *parse_datagram(msg));

    EXPECT_EQ(nodes[outsider]->store.size(), 0u);
    ASSERT_EQ(control.size(), 1u);  // the ACK
    EXPECT_EQ(try_parse_message(control.back().data)->value().header.status, Status::InvalidRequest);
}

// ── Duplicate detection ─────────────────────────────────────────────────

TEST_F(ClusterTest, RetriedDeleteGetsTheSameAnswer) {
    const std::string key = key_owned_by(2);
    put(0, key, "John");

    const auto msg = encode_message({.type = MessageType::DeleteRequest, .request_id = 42, .sender_id = 777},
                                    encode_payload(KeyPayload{key}));
    nodes[0]->router->on_datagram(msg, client);
    deliver();
    EXPECT_EQ(last_response().header.status, Status::Ok);

    // The client never saw that answer and retries the same request:
    // executing it again would say NOT_FOUND.
    nodes[0]->router->on_datagram(msg, client);
    deliver();
    EXPECT_EQ(last_response().header.status, Status::Ok);
    EXPECT_EQ(last_response().header.request_id, 42u);

    // A new DELETE (new request id) is a new operation.
    del(0, key);
    EXPECT_EQ(last_response().header.status, Status::NotFound);
}

TEST_F(ClusterTest, RetryWhileReplicatingIsNotExecutedTwice) {
    const std::string key = key_owned_by(2);
    nodes[replica_index(key)]->silent = true;  // keeps the first attempt waiting

    const auto msg = encode_message({.type = MessageType::PutRequest, .request_id = 7, .sender_id = 777},
                                    encode_payload(KeyValuePayload{key, "v"}));
    nodes[2]->router->on_datagram(msg, client);
    deliver();
    const std::size_t replication_messages = control_sent;
    nodes[2]->router->on_datagram(msg, client);  // retry
    deliver();

    EXPECT_EQ(control_sent, replication_messages);  // not replicated a second time
    EXPECT_EQ(nodes[2]->router->pending_writes(), 1u);
}
