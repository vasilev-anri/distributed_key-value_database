#pragma once

// Deterministic 64-bit hashing for key placement.
//
// Every node must compute exactly the same hash for the same key, on any
// machine and with any compiler. std::hash gives no such guarantee (the
// standard lets each library implement it differently), so we use a fixed,
// well-known algorithm instead.

#include <cstdint>
#include <string_view>

namespace cluster {

// FNV-1a, 64-bit: simple and fully specified.
constexpr std::uint64_t fnv1a64(std::string_view data) noexcept {
    std::uint64_t h = 14695981039346656037ull;  // offset basis
    for (const char c : data) {
        h ^= static_cast<std::uint8_t>(c);
        h *= 1099511628211ull;  // FNV prime
    }
    return h;
}

// Final mixing step (from SplitMix64). FNV-1a spreads changes poorly into the
// *high* bits for short inputs, but XOR distance is decided by the highest
// differing bit, so the high bits must be well mixed.
constexpr std::uint64_t mix64(std::uint64_t x) noexcept {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ull;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebull;
    x ^= x >> 31;
    return x;
}

// The hash used for both keys and node ids: one 64-bit space for both,
// so "which node is closest to this key" is well defined.
constexpr std::uint64_t key_hash(std::string_view data) noexcept {
    return mix64(fnv1a64(data));
}

}  // namespace cluster
