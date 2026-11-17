#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

// In-memory key-value storage of one node, with versions and tombstones.
//
// Every entry carries a VERSION. When two copies of a key disagree (after a
// node restarted, or after a write the replica never confirmed), the higher
// version wins. That makes replication and recovery order-independent:
// applying the same copy twice, or an old copy after a new one, changes
// nothing.
//
// A DELETE doesn't erase the key: it leaves a TOMBSTONE (deleted = true, with
// the delete's version). Otherwise an older copy of the key, still held by a
// node that missed the delete, could bring it back during recovery.
// Tombstones are invisible to get() and size().
//
// Versions come from a hybrid logical clock: wall-clock milliseconds in the
// high bits, a counter in the low 16 bits. They always increase, also after a
// restart (the clock moved on) and after seeing a newer version from a peer.
//
// Not thread-safe, on purpose: like all engine state, it is used only on the
// reactor thread, so it needs no locks.
class KeyValueStore {
public:
    struct Entry {
        std::string value;      // empty for tombstones
        std::uint64_t version = 0;
        bool deleted = false;
    };

    // ── Local writes (this node is the key's primary) ───────────────────

    // Inserts the key, or overwrites its value. Returns the new version.
    std::uint64_t put(std::string key, std::string value) {
        const std::uint64_t v = next_version();
        data_.insert_or_assign(std::move(key), Entry{std::move(value), v, false});
        return v;
    }

    // Deletes the key by leaving a tombstone (even if the key didn't exist:
    // a copy elsewhere might). Returns true if a live value was removed.
    bool remove(const std::string& key) {
        const auto it = data_.find(key);
        const bool existed = it != data_.end() && !it->second.deleted;
        data_.insert_or_assign(key, Entry{{}, next_version(), true});
        return existed;
    }

    // ── Copies from other nodes (replication and recovery) ──────────────

    // Stores `entry` only if it is newer than what we have. Returns whether
    // it was applied. Equal versions are the same write: nothing to do.
    bool apply(const std::string& key, Entry entry) {
        observe(entry.version);
        const auto it = data_.find(key);
        if (it != data_.end() && it->second.version >= entry.version) return false;
        if (entry.deleted) entry.value.clear();
        data_.insert_or_assign(key, std::move(entry));
        return true;
    }

    // ── Reads ───────────────────────────────────────────────────────────

    // A copy of the value, or std::nullopt if the key doesn't exist or was deleted.
    [[nodiscard]] std::optional<std::string> get(const std::string& key) const {
        const auto it = data_.find(key);
        if (it == data_.end() || it->second.deleted) return std::nullopt;
        return it->second.value;
    }

    // The entry including its version and tombstone flag, or nullptr.
    [[nodiscard]] const Entry* entry(const std::string& key) const {
        const auto it = data_.find(key);
        return it == data_.end() ? nullptr : &it->second;
    }

    // Live keys (tombstones not counted).
    [[nodiscard]] std::size_t size() const noexcept {
        return static_cast<std::size_t>(std::ranges::count_if(
            data_, [](const auto& kv) { return !kv.second.deleted; }));
    }

    // Every entry, tombstones included (for recovery).
    void for_each(const std::function<void(const std::string& key, const Entry& entry)>& fn) const {
        for (const auto& [key, entry] : data_) fn(key, entry);
    }

private:
    std::uint64_t next_version() {
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
        last_version_ = std::max(last_version_ + 1, static_cast<std::uint64_t>(ms) << 16);
        return last_version_;
    }

    // Lamport rule: never hand out a version below one we have seen.
    void observe(std::uint64_t version) noexcept { last_version_ = std::max(last_version_, version); }

    std::unordered_map<std::string, Entry> data_;
    std::uint64_t last_version_ = 0;
};
