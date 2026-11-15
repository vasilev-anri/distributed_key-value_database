#include <gtest/gtest.h>

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "cluster/hash.h"
#include "cluster/membership.h"
#include "utils/address.h"

using namespace cluster;

namespace {

NodeEntry node_at(const char* endpoint) { return make_node(*parse_endpoint(endpoint)); }

Membership three_nodes(const char* self) {
    Membership m(node_at(self));
    for (const char* ep : {"[::1]:9001", "[::1]:9002", "[::1]:9003"}) {
        if (std::string(ep) != self) m.add(node_at(ep));
    }
    return m;
}

}  // namespace

// ── Hash ────────────────────────────────────────────────────────────────

TEST(Hash, Fnv1aMatchesReferenceValues) {
    // Published FNV-1a 64-bit test vectors: proves we implement the real algorithm,
    // so any other implementation (e.g. in Kotlin) can produce the same values.
    EXPECT_EQ(fnv1a64(""), 0xcbf29ce484222325ull);
    EXPECT_EQ(fnv1a64("a"), 0xaf63dc4c8601ec8cull);
    EXPECT_EQ(fnv1a64("foobar"), 0x85944171f73967e8ull);
}

TEST(Hash, IsDeterministicAndSpreads) {
    EXPECT_EQ(key_hash("student101"), key_hash("student101"));
    EXPECT_NE(key_hash("student101"), key_hash("student102"));
    // Similar keys must differ in the high bits too (XOR distance depends on them).
    EXPECT_NE(key_hash("student101") >> 60, key_hash("student102") >> 60);
}

// ── Node identity ───────────────────────────────────────────────────────

TEST(Membership, NodeIdComesFromCanonicalAddress) {
    // Different spellings of the same IPv6 address give the same node.
    EXPECT_EQ(node_at("[::1]:9001").id, node_at("[0:0:0:0:0:0:0:1]:9001").id);
    EXPECT_EQ(node_at("[::1]:9001").name, "[::1]:9001");
    // Different port -> different node.
    EXPECT_NE(node_at("[::1]:9001").id, node_at("[::1]:9002").id);
}

TEST(Membership, RejectsDuplicateNodes) {
    Membership m(node_at("[::1]:9001"));
    m.add(node_at("[::1]:9002"));
    EXPECT_THROW(m.add(node_at("[::1]:9002")), std::invalid_argument);
    EXPECT_THROW(m.add(node_at("[::1]:9001")), std::invalid_argument);  // ourselves
    EXPECT_EQ(m.nodes().size(), 2u);
}

// ── Ownership ───────────────────────────────────────────────────────────

TEST(Membership, ClosestNodeUsesXorDistance) {
    std::vector<NodeEntry> nodes(3);
    nodes[0].id = 0b1000;
    nodes[1].id = 0b0100;
    nodes[2].id = 0b0001;
    EXPECT_EQ(closest_node(0b1001, nodes).id, 0b1000u);  // distance 1
    EXPECT_EQ(closest_node(0b0110, nodes).id, 0b0100u);  // distance 2
    EXPECT_EQ(closest_node(0b0011, nodes).id, 0b0001u);  // distance 2 (vs 7 and 11)
}

TEST(Membership, AllNodesAgreeOnOwners) {
    const Membership a = three_nodes("[::1]:9001");
    const Membership b = three_nodes("[::1]:9002");
    const Membership c = three_nodes("[::1]:9003");

    for (int i = 0; i < 500; ++i) {
        const std::string key = "key" + std::to_string(i);
        const auto owner = a.owner_of(key).id;
        EXPECT_EQ(b.owner_of(key).id, owner) << key;
        EXPECT_EQ(c.owner_of(key).id, owner) << key;
    }
}

TEST(Membership, KeysSpreadOverAllNodes) {
    const Membership m = three_nodes("[::1]:9001");
    std::map<std::uint64_t, int> count;
    constexpr int kKeys = 3000;
    for (int i = 0; i < kKeys; ++i) ++count[m.owner_of("key" + std::to_string(i)).id];

    ASSERT_EQ(count.size(), 3u);  // every node owns something
    // XOR placement with only 3 random ids isn't perfectly even, but no node
    // should be (almost) empty.
    for (const auto& [id, n] : count) EXPECT_GT(n, kKeys / 20) << std::hex << id;
}
