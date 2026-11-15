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
}

void Replicator::log(const std::string& line) {
    if (log_) log_(line);
}

// ── Primary side ────────────────────────────────────────────────────────

void Replicator::replicate_put(const std::string& key, const std::string& value, DoneFn done) {
    replicate(key, encode_payload(KeyValuePayload{key, value}), MessageType::ReplicaPut, std::move(done));
}

void Replicator::replicate_delete(const std::string& key, DoneFn done) {
    replicate(key, encode_payload(KeyPayload{key}), MessageType::ReplicaDelete, std::move(done));
}

void Replicator::replicate(const std::string& key, const std::vector<std::byte>& payload,
                           MessageType type, DoneFn done) {
    // The replicas: every node of the key's replica set except ourselves.
    std::vector<cluster::NodeEntry> replicas = members_.replicas_of(key);
    std::erase_if(replicas, [this](const cluster::NodeEntry& n) { return members_.is_self(n); });

    if (replicas.empty()) {  // single-node cluster: nothing to wait for
        done(Status::Ok);
        return;
    }

    const std::uint64_t id = next_id_++;
    const auto message = encode_message({.type = type, .request_id = id, .sender_id = members_.self().id},
                                        payload);

    // Registered before sending: an ACK may in principle arrive (and finish
    // this replication) before the send loop below completes.
    Pending& p = pending_[id];
    p.done = std::move(done);
    p.what = std::string(to_string(type)) + " " + printable(key);
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
            log(std::string(to_string(type)) + " " + printable(key) + ": replica " + replica.name +
                " is not connected -> REPLICATION_FAILED");
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

// Replica side: apply the primary's write, then confirm.
void Replicator::apply(std::uint64_t peer_id, const Frame& frame) {
    Status status = Status::Ok;
    std::string what = std::string(to_string(frame.header.type));

    auto is_replica = [this](const std::string& key) {
        for (const auto& n : members_.replicas_of(key))
            if (members_.is_self(n)) return true;
        return false;
    };

    if (frame.header.type == MessageType::ReplicaPut) {
        auto kv = decode_key_value(frame.payload);
        if (!kv) {
            status = Status::InvalidRequest;
        } else {
            what += " " + printable(kv->key);
            // Never trust the network: only store keys this node is responsible for.
            if (!is_replica(kv->key)) status = Status::InvalidRequest;
            else store_.put(std::move(kv->key), std::move(kv->value));
        }
    } else {
        auto k = decode_key(frame.payload);
        if (!k) {
            status = Status::InvalidRequest;
        } else {
            what += " " + printable(k->key);
            if (!is_replica(k->key)) status = Status::InvalidRequest;
            else store_.remove(k->key);  // deleting a missing key is fine: the result is the same
        }
    }

    log(what + " from " + hex_id(peer_id) + ": " + std::string(to_string(status)));
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
