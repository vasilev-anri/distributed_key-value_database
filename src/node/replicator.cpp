#include "node/replicator.h"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <utility>

#include "cluster/membership.h"
#include "protocol/codec.h"
#include "storage/kv_store.h"

using namespace protocol;

namespace {

std::string printable(std::string_view key) {
    std::string out;
    for (const char c : key.substr(0, 32)) out += std::isprint(static_cast<unsigned char>(c)) ? c : '.';
    if (key.size() > 32) out += "...";
    return out;
}

std::string hex_id(std::uint64_t id) {
    static constexpr char kDigits[] = "0123456789abcdef";
    std::string out(16, '0');
    for (int i = 15; i >= 0; --i, id >>= 4) out[static_cast<std::size_t>(i)] = kDigits[id & 0xf];
    return out;
}

}  // namespace

Replicator::Replicator(cluster::Membership& members, KeyValueStore& store, TimerQueue& timers,
                       PeerSendFn send, std::chrono::milliseconds ack_timeout)
    : members_(members), store_(store), timers_(timers), send_(std::move(send)), ack_timeout_(ack_timeout) {}

Replicator::~Replicator() {
    // Timer callbacks capture `this`: none may fire after we're gone.
    // Pending `done` callbacks are dropped without being called: the node is
    // shutting down and nobody is waiting for an answer anymore.
    for (const auto& [id, p] : pending_) timers_.cancel(p.timer);
    for (const auto& [peer, sync] : syncs_) timers_.cancel(sync.timer);
}

void Replicator::log(const std::string& line) {
    if (log_) log_(line);
}

std::vector<std::byte> Replicator::copy_of(const std::string& key, std::uint64_t request_id,
                                           std::uint8_t flags) const {
    const KeyValueStore::Entry* e = store_.entry(key);
    const Header header{.type = e->deleted ? MessageType::ReplicaDelete : MessageType::ReplicaPut,
                        .flags = flags,
                        .request_id = request_id,
                        .sender_id = members_.self().id};
    if (e->deleted) return encode_message(header, encode_payload(ReplicaDeletePayload{key, e->version}));
    return encode_message(header, encode_payload(ReplicaPutPayload{key, e->value, e->version}));
}

// ── Primary side: replicate one write ───────────────────────────────────

void Replicator::replicate(const std::string& key, DoneFn done) {
    // The replicas: every node of the key's replica set except ourselves.
    std::vector<cluster::NodeEntry> replicas = members_.replicas_of(key);
    std::erase_if(replicas, [this](const cluster::NodeEntry& n) { return members_.is_self(n); });

    if (replicas.empty() || !store_.entry(key)) {  // single-node cluster: nothing to wait for
        done(Status::Ok);
        return;
    }

    const std::uint64_t id = next_id_++;
    const auto message = copy_of(key, id, 0);
    const std::string what = std::string(store_.entry(key)->deleted ? "REPLICA_DELETE " : "REPLICA_PUT ") +
                             printable(key);

    // Registered before sending: an ACK may in principle arrive (and finish
    // this replication) before the send loop below completes.
    Pending& p = pending_[id];
    p.done = std::move(done);
    p.what = what;
    for (const auto& r : replicas) p.waiting.push_back(r.id);
    p.timer = timers_.schedule_after(ack_timeout_, [this, id] {
        const auto it = pending_.find(id);
        if (it == pending_.end()) return;
        it->second.timer = 0;  // this timer is firing: nothing to cancel
        log(it->second.what + ": no ACK within " + std::to_string(ack_timeout_.count()) +
            " ms -> REPLICATION_FAILED");
        finish(id, Status::ReplicationFailed);
    });

    for (const auto& replica : replicas) {
        if (!send_(replica.id, message)) {
            log(what + ": replica " + replica.name + " is not connected -> REPLICATION_FAILED");
            finish(id, Status::ReplicationFailed);
            return;
        }
        if (!pending_.contains(id)) return;  // already finished (synchronous ACK)
    }
}

void Replicator::finish(std::uint64_t id, Status status) {
    const auto it = pending_.find(id);
    if (it == pending_.end()) return;

    // Take it out of the map first: `done` may start new replications.
    Pending p = std::move(it->second);
    pending_.erase(it);
    if (p.timer != 0) timers_.cancel(p.timer);
    p.done(status);
}

// ── Recovery: send a peer everything it should hold ─────────────────────

void Replicator::sync_to(std::uint64_t peer_id) {
    // A previous sync to this peer (its connection dropped and came back):
    // start over, the peer may have lost everything again.
    if (const auto old = syncs_.find(peer_id); old != syncs_.end()) {
        timers_.cancel(old->second.timer);
        syncs_.erase(old);
    }

    // Snapshot of the keys only: each entry is read when it is sent, so a
    // write that happens during the sync is sent in its newest version.
    Sync sync;
    store_.for_each([&](const std::string& key, const KeyValueStore::Entry&) {
        for (const auto& n : members_.replicas_of(key)) {
            if (n.id == peer_id) {
                sync.keys.push_back(key);
                break;
            }
        }
    });
    if (sync.keys.empty()) return;

    syncs_.emplace(peer_id, std::move(sync));
    send_sync_batch(peer_id);
}

// One batch per reactor turn, so a large sync doesn't block the node.
void Replicator::send_sync_batch(std::uint64_t peer_id) {
    const auto it = syncs_.find(peer_id);
    if (it == syncs_.end()) return;
    Sync& sync = it->second;
    sync.timer = 0;

    const std::size_t end = std::min(sync.next + kSyncBatch, sync.keys.size());
    for (; sync.next < end; ++sync.next) {
        // request_id 0 + SYNC flag: the peer applies it and sends no ACK.
        if (!send_(peer_id, copy_of(sync.keys[sync.next], 0, flags::kSync))) {
            log("recovery: connection to " + hex_id(peer_id) + " lost after sending " +
                std::to_string(sync.sent) + " entries, will resend on reconnect");
            syncs_.erase(it);
            return;
        }
        ++sync.sent;
    }

    if (sync.next < sync.keys.size()) {
        sync.timer = timers_.schedule_after(std::chrono::milliseconds(0),
                                            [this, peer_id] { send_sync_batch(peer_id); });
        return;
    }

    log("recovery: sent " + std::to_string(sync.sent) + " entries to " + hex_id(peer_id));
    syncs_.erase(it);
}

// ── Messages from peers ─────────────────────────────────────────────────

bool Replicator::on_message(std::uint64_t peer_id, const Frame& frame) {
    switch (frame.header.type) {
        case MessageType::ReplicaPut:
        case MessageType::ReplicaDelete:
            apply(peer_id, frame);
            return true;
        case MessageType::ReplicaAck:
            handle_ack(peer_id, frame);
            return true;
        default:
            return false;
    }
}

// Replica side (and recovery): store the copy if it is newer, then confirm.
void Replicator::apply(std::uint64_t peer_id, const Frame& frame) {
    const bool sync = frame.header.flags & flags::kSync;

    std::string key;
    KeyValueStore::Entry entry;
    Status status = Status::Ok;

    if (frame.header.type == MessageType::ReplicaPut) {
        auto p = decode_replica_put(frame.payload);
        if (p) {
            key = std::move(p->key);
            entry = {std::move(p->value), p->version, false};
        } else {
            status = Status::InvalidRequest;
        }
    } else {
        auto p = decode_replica_delete(frame.payload);
        if (p) {
            key = std::move(p->key);
            entry = {{}, p->version, true};
        } else {
            status = Status::InvalidRequest;
        }
    }

    // Never trust the network: only store keys this node is responsible for.
    if (status == Status::Ok) {
        const auto set = members_.replicas_of(key);
        const bool responsible = std::ranges::any_of(set, [this](const auto& n) { return members_.is_self(n); });
        if (!responsible) status = Status::InvalidRequest;
    }

    bool applied = false;
    if (status == Status::Ok) applied = store_.apply(key, std::move(entry));

    const std::string what = std::string(to_string(frame.header.type)) + " " + printable(key);
    if (sync) {
        // Recovery traffic can be large: log only what actually changed.
        if (applied) log("recovery: " + what + " from " + hex_id(peer_id) + " applied");
        return;  // no ACK for sync messages
    }

    log(what + " from " + hex_id(peer_id) + ": " + std::string(to_string(status)) +
        (status == Status::Ok && !applied ? " (already up to date)" : ""));
    const auto ack = encode_message({.type = MessageType::ReplicaAck,
                                     .status = status,
                                     .request_id = frame.header.request_id,
                                     .sender_id = members_.self().id});
    send_(peer_id, ack);  // if the link just dropped, the primary times out
}

// Primary side: one replica confirmed (or rejected).
void Replicator::handle_ack(std::uint64_t peer_id, const Frame& frame) {
    const std::uint64_t id = frame.header.request_id;
    const auto it = pending_.find(id);
    if (it == pending_.end()) return;  // late ACK: that replication already finished

    auto& waiting = it->second.waiting;
    const auto w = std::ranges::find(waiting, peer_id);
    if (w == waiting.end()) return;  // not a replica we're waiting for: ignore

    if (frame.header.status != Status::Ok) {
        log(it->second.what + ": replica " + hex_id(peer_id) + " answered " +
            std::string(to_string(frame.header.status)) + " -> REPLICATION_FAILED");
        finish(id, Status::ReplicationFailed);
        return;
    }

    waiting.erase(w);
    if (waiting.empty()) finish(id, Status::Ok);
}
