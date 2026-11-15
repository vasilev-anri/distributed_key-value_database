#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

// In-memory key-value storage of one node.
//
// Not thread-safe, on purpose: like all engine state, it is used only on the
// reactor thread, so it needs no locks.
class KeyValueStore {
public:
    // Inserts the key, or overwrites its value if it already exists.
    void put(std::string key, std::string value) {
        data_.insert_or_assign(std::move(key), std::move(value));
    }

    // Returns a copy of the value, or std::nullopt if the key doesn't exist.
    [[nodiscard]] std::optional<std::string> get(const std::string& key) const {
        const auto it = data_.find(key);
        if (it == data_.end()) return std::nullopt;
        return it->second;
    }

    // Returns true if the key existed (and was removed).
    bool remove(const std::string& key) { return data_.erase(key) > 0; }

    [[nodiscard]] std::size_t size() const noexcept { return data_.size(); }

private:
    std::unordered_map<std::string, std::string> data_;
};
