#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

#include "node/response_cache.h"

namespace {
std::vector<std::byte> bytes(int n) { return std::vector<std::byte>(static_cast<std::size_t>(n), std::byte{1}); }
}  // namespace

TEST(ResponseCache, FindsWhatWasInserted) {
    ResponseCache cache;
    EXPECT_EQ(cache.find({1, 1}), nullptr);
    cache.insert({1, 1}, bytes(3));
    ASSERT_NE(cache.find({1, 1}), nullptr);
    EXPECT_EQ(cache.find({1, 1})->size(), 3u);
}

TEST(ResponseCache, SenderAndRequestIdBothMatter) {
    ResponseCache cache;
    cache.insert({1, 2}, bytes(1));
    EXPECT_EQ(cache.find({2, 1}), nullptr);  // another client's request 1
    EXPECT_EQ(cache.find({1, 3}), nullptr);  // the same client's next request
}

TEST(ResponseCache, ForgetsTheOldestWhenFull) {
    ResponseCache cache(2);
    cache.insert({1, 1}, bytes(1));
    cache.insert({1, 2}, bytes(1));
    cache.insert({1, 3}, bytes(1));
    EXPECT_EQ(cache.size(), 2u);
    EXPECT_EQ(cache.find({1, 1}), nullptr);
    EXPECT_NE(cache.find({1, 2}), nullptr);
    EXPECT_NE(cache.find({1, 3}), nullptr);
}

TEST(ResponseCache, ReinsertingDoesNotGrowIt) {
    ResponseCache cache(2);
    cache.insert({1, 1}, bytes(1));
    cache.insert({1, 1}, bytes(5));
    EXPECT_EQ(cache.size(), 1u);
    EXPECT_EQ(cache.find({1, 1})->size(), 5u);
}
