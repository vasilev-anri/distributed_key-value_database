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
