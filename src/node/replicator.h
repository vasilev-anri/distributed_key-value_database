#pragma once

// Replication over the TCP control channel.
//
// Primary side: after the primary has applied a PUT or DELETE to its own
// store, replicate() sends REPLICA_PUT (or REPLICA_DELETE for a tombstone)
// to every replica of the key and report, through a callback, whether all of
// them confirmed with REPLICA_ACK in time.
//
// Replica side: on_message() applies incoming REPLICA_PUT / REPLICA_DELETE to
// the local store and answers REPLICA_ACK.
//
// Every copy carries the entry's version, and the receiver applies it only if
// it is newer than what it has (see KeyValueStore). So copies can arrive
// twice or out of order without harm.
//
// Recovery: when the control connection to a peer comes (back) up, sync_to()
// sends that peer every entry (tombstones included) it is responsible for,
// in batches, flagged SYNC so the peer doesn't ACK each one. A node that
// restarted empty gets its keys back; copies that diverged (e.g. after
// REPLICATION_FAILED) converge to the newest version. Both sides of a new
// connection sync to each other.
//
// Like Router, it knows nothing about sockets: messages go out through the
// injected PeerSendFn (PeerManager::send in the node) and come in through
// on_message(), so it is testable with a simulated network.

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "handlers/timer_queue.h"
#include "protocol/protocol.h"

class KeyValueStore;
namespace cluster { class Membership; }

class Replicator {
public:
    using PeerSendFn = std::move_only_function<bool(std::uint64_t peer_id, std::span<const std::byte> message)>;
    using DoneFn = std::move_only_function<void(protocol::Status status)>;
    using LogFn = std::move_only_function<void(const std::string& line)>;

    // How long the primary waits for the replicas' ACKs. Shorter than the
    // Router's forward timeout, so a forwarding node gets REPLICATION_FAILED
    // from the primary rather than giving up on it first.
    static constexpr std::chrono::milliseconds kDefaultAckTimeout{100};

    Replicator(cluster::Membership& members, KeyValueStore& store, TimerQueue& timers, PeerSendFn send,
               std::chrono::milliseconds ack_timeout = kDefaultAckTimeout);

    ~Replicator();
    Replicator(const Replicator&) = delete;
    Replicator& operator=(const Replicator&) = delete;

    void set_logger(LogFn log) noexcept { log_ = std::move(log); }

    // Entries sent per batch during recovery (one batch per reactor turn).
    static constexpr std::size_t kSyncBatch = 256;

    // Primary side: copies the key's current entry (a value, or a tombstone
    // after DELETE) from the local store to every replica of the key.
    // `done` is called exactly once:
    //   Ok                 every replica confirmed (or the key has no replicas,
    //                      e.g. a single-node cluster)
    //   ReplicationFailed  a replica isn't connected, rejected the write, or
    //                      didn't confirm within the ACK timeout
    // `done` may be called before this function returns.
    void replicate(const std::string& key, DoneFn done);

    // Recovery: sends `peer_id` every entry it should hold. Call whenever the
    // control connection to that peer becomes CONNECTED. Restarts a sync to
    // the same peer that is still running.
    void sync_to(std::uint64_t peer_id);

    // Feed every control message received from a peer here. Messages that
    // aren't about replication are ignored (returns false).
    bool on_message(std::uint64_t peer_id, const protocol::Frame& frame);

    // Replications waiting for ACKs.
    [[nodiscard]] std::size_t pending() const noexcept { return pending_.size(); }

    // Recovery syncs still sending.
    [[nodiscard]] std::size_t syncs_running() const noexcept { return syncs_.size(); }

private:
    struct Pending {
        std::vector<std::uint64_t> waiting;  // replicas that haven't confirmed yet
        DoneFn done;
        TimerQueue::TimerId timer = 0;
        std::string what;                    // for logs, e.g. "PUT student101"
    };

    struct Sync {
        std::vector<std::string> keys;  // what the peer should hold (snapshot)
        std::size_t next = 0;           // index of the next key to send
        std::size_t sent = 0;
        TimerQueue::TimerId timer = 0;  // next batch
    };

    // The REPLICA_PUT / REPLICA_DELETE message for the key's current entry.
    [[nodiscard]] std::vector<std::byte> copy_of(const std::string& key, std::uint64_t request_id,
                                                 std::uint8_t flags) const;
    void send_sync_batch(std::uint64_t peer_id);
    void finish(std::uint64_t id, protocol::Status status);
    void apply(std::uint64_t peer_id, const protocol::Frame& frame);
    void handle_ack(std::uint64_t peer_id, const protocol::Frame& frame);
    void log(const std::string& line);

    cluster::Membership& members_;
    KeyValueStore& store_;
    TimerQueue& timers_;
    PeerSendFn send_;
    std::chrono::milliseconds ack_timeout_;
    LogFn log_;

    std::uint64_t next_id_ = 1;  // request_id of our replication messages
    std::unordered_map<std::uint64_t, Pending> pending_;
    std::unordered_map<std::uint64_t, Sync> syncs_;  // by peer id
};
