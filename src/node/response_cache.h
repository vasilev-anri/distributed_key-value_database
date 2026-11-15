#pragma once

// Duplicate detection: remembers the responses to recently executed writes.
//
// A client that gets no answer resends the same request with the same
// (sender_id, request_id). If the first attempt was actually executed and only
// the response was lost, executing it again could give a different answer
// (a repeated DELETE would say NOT_FOUND). Instead, the node finds the stored
// response here and sends it again.
//
// Bounded: once `capacity` responses are stored, the oldest is forgotten.
// Retries arrive within a second or two, so a few thousand entries are plenty.

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <span>
#include <unordered_map>
#include <vector>

// A request is identified by (sender_id, request_id), see docs/protocol.md.
struct RequestKey {
    std::uint64_t sender_id = 0;
    std::uint64_t request_id = 0;
    bool operator==(const RequestKey&) const = default;
};

struct RequestKeyHash {
    std::size_t operator()(const RequestKey& k) const noexcept {
        return std::hash<std::uint64_t>{}(k.sender_id * 0x9e3779b97f4a7c15ull ^ k.request_id);
    }
};

class ResponseCache {
public:
    static constexpr std::size_t kDefaultCapacity = 4096;

    explicit ResponseCache(std::size_t capacity = kDefaultCapacity) : capacity_(capacity) {}

    // The stored response, or nullptr. Valid until the next insert().
    [[nodiscard]] const std::vector<std::byte>* find(const RequestKey& key) const {
        const auto it = responses_.find(key);
        return it == responses_.end() ? nullptr : &it->second;
    }

    void insert(const RequestKey& key, std::span<const std::byte> response) {
        if (capacity_ == 0) return;
        const auto [it, inserted] = responses_.insert_or_assign(
            key, std::vector<std::byte>(response.begin(), response.end()));
        if (!inserted) return;  // replaced an existing entry: order unchanged

        order_.push_back(key);
        if (order_.size() > capacity_) {  // forget the oldest
            responses_.erase(order_.front());
            order_.pop_front();
        }
    }

    [[nodiscard]] std::size_t size() const noexcept { return responses_.size(); }

private:
    std::size_t capacity_;
    std::unordered_map<RequestKey, std::vector<std::byte>, RequestKeyHash> responses_;
    std::deque<RequestKey> order_;  // insertion order, oldest first
};
