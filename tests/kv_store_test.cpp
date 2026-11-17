#include <gtest/gtest.h>

#include <string>

#include "storage/kv_store.h"

TEST(KeyValueStore, GetMissingKeyReturnsNothing) {
    KeyValueStore store;
    EXPECT_FALSE(store.get("missing").has_value());
    EXPECT_EQ(store.size(), 0u);
}

TEST(KeyValueStore, PutThenGet) {
    KeyValueStore store;
    store.put("student101", "John");
    EXPECT_EQ(store.get("student101"), "John");
    EXPECT_EQ(store.size(), 1u);
}

TEST(KeyValueStore, PutOverwritesExistingValue) {
    KeyValueStore store;
    store.put("k", "first");
    store.put("k", "second");
    EXPECT_EQ(store.get("k"), "second");
    EXPECT_EQ(store.size(), 1u);
}

TEST(KeyValueStore, RemoveReportsWhetherKeyExisted) {
    KeyValueStore store;
    store.put("k", "v");
    EXPECT_TRUE(store.remove("k"));
    EXPECT_FALSE(store.remove("k"));  // already gone
    EXPECT_FALSE(store.get("k").has_value());
}

TEST(KeyValueStore, ValuesMayContainAnyBytes) {
    KeyValueStore store;
    const std::string binary("a\0b\xff", 4);  // embedded NUL and non-ASCII byte
    store.put("bin", binary);
    EXPECT_EQ(store.get("bin"), binary);
}

// ── Versions and tombstones ─────────────────────────────────────────────

TEST(KeyValueStore, EveryWriteGetsAHigherVersion) {
    KeyValueStore store;
    const auto v1 = store.put("k", "a");
    const auto v2 = store.put("k", "b");
    store.remove("k");
    const auto v3 = store.entry("k")->version;
    EXPECT_LT(v1, v2);
    EXPECT_LT(v2, v3);
}

TEST(KeyValueStore, DeleteLeavesAnInvisibleTombstone) {
    KeyValueStore store;
    store.put("k", "v");
    store.remove("k");
    EXPECT_FALSE(store.get("k"));
    EXPECT_EQ(store.size(), 0u);
    ASSERT_NE(store.entry("k"), nullptr);
    EXPECT_TRUE(store.entry("k")->deleted);
}

TEST(KeyValueStore, DeletingAMissingKeyStillRecordsTheDelete) {
    KeyValueStore store;
    EXPECT_FALSE(store.remove("never-existed"));
    ASSERT_NE(store.entry("never-existed"), nullptr);  // so it can win over a stray copy elsewhere
}

TEST(KeyValueStore, ApplyKeepsTheNewerVersion) {
    KeyValueStore store;
    EXPECT_TRUE(store.apply("k", {"new", 200, false}));
    EXPECT_FALSE(store.apply("k", {"old", 100, false}));  // older: ignored
    EXPECT_FALSE(store.apply("k", {"same", 200, false})); // same version = same write
    EXPECT_EQ(store.get("k"), "new");
}

TEST(KeyValueStore, NewerTombstoneBeatsValueAndOlderValueLosesToTombstone) {
    KeyValueStore store;
    store.apply("k", {"v", 100, false});
    EXPECT_TRUE(store.apply("k", {"", 200, true}));
    EXPECT_FALSE(store.get("k"));
    EXPECT_FALSE(store.apply("k", {"v", 100, false}));  // the old value can't come back
    EXPECT_FALSE(store.get("k"));
}

TEST(KeyValueStore, LocalWritesAfterApplyGetHigherVersions) {
    KeyValueStore store;
    const std::uint64_t far_future = ~0ull >> 1;
    store.apply("x", {"v", far_future, false});
    EXPECT_GT(store.put("y", "w"), far_future);  // never below a version we've seen
}
