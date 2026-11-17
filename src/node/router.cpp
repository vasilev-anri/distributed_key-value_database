#include "node/router.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <optional>
#include <string_view>
#include <utility>

#include "node/replicator.h"
#include "node/request_handler.h"
#include "protocol/codec.h"
#include "utils/address.h"

using namespace protocol;

namespace {

bool is_request(MessageType t) noexcept {
    return t == MessageType::PutRequest || t == MessageType::GetRequest ||
           t == MessageType::DeleteRequest;
}

bool is_response(MessageType t) noexcept {
    return t == MessageType::PutResponse || t == MessageType::GetResponse ||
           t == MessageType::DeleteResponse;
}

bool same_address(const sockaddr_in6& a, const sockaddr_in6& b) noexcept {
    return a.sin6_port == b.sin6_port &&
           std::memcmp(&a.sin6_addr, &b.sin6_addr, sizeof a.sin6_addr) == 0;
}

// The key a request is about, or nullopt if the payload is malformed
// (then the request is handled locally, which answers with an error status).
std::optional<std::string> request_key(const Frame& frame) {
    if (frame.header.type == MessageType::PutRequest) {
        auto kv = decode_key_value(frame.payload);
        if (kv) return std::move(kv->key);
    } else {
        auto k = decode_key(frame.payload);
        if (k) return std::move(k->key);
    }
    return std::nullopt;
}

// Keys are arbitrary bytes: make them safe to print.
std::string printable(std::string_view key) {
    std::string out;
    for (const char c : key.substr(0, 32)) {
        out += std::isprint(static_cast<unsigned char>(c)) ? c : '.';
    }
    if (key.size() > 32) out += "...";
    return out;
}

std::string str(std::string_view sv) { return std::string(sv); }

}  // namespace

Router::Router(cluster::Membership& members, RequestHandler& requests, Replicator& replicator,
               TimerQueue& timers, SendFn send, std::chrono::milliseconds forward_timeout)
    : members_(members),
      requests_(requests),
      replicator_(replicator),
      timers_(timers),
      send_(std::move(send)),
      forward_timeout_(forward_timeout) {}

Router::~Router() {
    // Timer callbacks capture `this`: none may fire after we're gone.
    for (const auto& [key, p] : pending_) timers_.cancel(p.timer);
}

void Router::log(const std::string& line) {
    if (log_) log_(line);
}

bool Router::is_alive(const cluster::NodeEntry& node) {
    if (members_.is_self(node) || !alive_) return true;
    return alive_(node.id);
}

// ── Entry point ─────────────────────────────────────────────────────────

void Router::on_datagram(std::span<const std::byte> data, const sockaddr_in6& from) {
    const auto frame = parse_datagram(data);
    if (!frame) {
        log(to_string(from) + ": dropped invalid datagram (" + str(to_string(frame.error())) + ")");
        return;
    }

    if (is_request(frame->header.type)) {
        handle_request(*frame, from);
    } else if (is_response(frame->header.type)) {
        handle_response(*frame, data, from);
    }
    // Anything else (control messages) doesn't belong on the data channel: ignore.
}

// ── Requests ────────────────────────────────────────────────────────────

void Router::handle_request(const Frame& frame, const sockaddr_in6& from) {
    const auto key = request_key(frame);
    if (!key) {
        handle_locally(frame, from);  // malformed: answered with an error status
        return;
    }

    std::vector<cluster::NodeEntry> replicas = members_.replicas_of(*key);  // primary first
    const cluster::NodeEntry primary = replicas.front();
    const bool forwarded = frame.header.flags & flags::kForwarded;
    const bool is_write = frame.header.type != MessageType::GetRequest;
    std::string what = str(to_string(frame.header.type)) + " " + printable(*key);

    // ── We are the primary: execute ─────────────────────────────────────
    if (members_.is_self(primary)) {
        if (is_write) execute_write(frame, from, what);
        else handle_locally(frame, from);
        return;
    }

    // ── Forwarded to us, but we're not the primary ──────────────────────
    // Never forward twice (loop protection). The one thing we may still do
    // is serve a read from our replica: that's why it was sent here.
    if (forwarded) {
        const bool self_is_replica = std::ranges::any_of(
            replicas, [this](const cluster::NodeEntry& n) { return members_.is_self(n); });
        if (!is_write && self_is_replica) {
            log(what + ": replica read");
            handle_locally(frame, from);
        } else {
            log(what + ": forwarded to us but primary is " + primary.name + " -> UNAVAILABLE");
            reply_status(frame.header, Status::Unavailable, from);
        }
        return;
    }

    // A client retry while the first attempt is still in flight: the pending
    // forward will answer it, so don't forward a second copy.
    if (pending_.contains({frame.header.sender_id, frame.header.request_id})) {
        log(what + ": retry of a request already forwarded, ignored");
        return;
    }

    // ── Writes: only the primary may execute them ───────────────────────
    if (is_write) {
        if (!is_alive(primary)) {
            log(what + ": primary " + primary.name + " is down -> UNAVAILABLE");
            reply_status(frame.header, Status::Unavailable, from);
            return;
        }
        log(what + ": primary is " + primary.name + ", forwarding");
        forward(frame, from, {primary}, std::move(what));
        return;
    }

    // ── Reads: primary first, then the replica ──────────────────────────
    // Nodes known to be down are tried last (not skipped: the liveness
    // information may be a moment out of date).
    std::ranges::stable_partition(replicas, [this](const cluster::NodeEntry& n) { return is_alive(n); });

    if (members_.is_self(replicas.front())) {  // primary down, and we hold the replica
        log(what + ": primary " + primary.name + " is down, reading local replica");
        handle_locally(frame, from);
        return;
    }

    log(what + ": " + (replicas.front().id == primary.id ? "primary is " : "primary down, asking replica ") +
        replicas.front().name + ", forwarding");
    forward(frame, from, std::move(replicas), std::move(what));
}

void Router::handle_locally(const Frame& frame, const sockaddr_in6& from) {
    const auto response = requests_.handle(frame);
    if (!response) return;

    const auto key = request_key(frame);
    log(str(to_string(frame.header.type)) + " " + (key ? printable(*key) : std::string("?")) +
        ": local -> " + str(to_string(response->status)));
    send_(response->message, from);
}

// We are the primary of the key: apply the write, replicate it, and answer
// only when the replica has confirmed.
void Router::execute_write(const Frame& frame, const sockaddr_in6& from, const std::string& what) {
    const RequestKey key{frame.header.sender_id, frame.header.request_id};

    // Duplicate detection: this exact request was already executed (the
    // client didn't get our answer and retried). Answer the same again.
    if (const auto* cached = completed_.find(key)) {
        log(what + ": duplicate of an executed request, resending its response");
        send_(*cached, from);
        return;
    }
    // Still replicating the first attempt: its answer is on the way.
    if (replicating_.contains(key)) {
        log(what + ": retry while replicating, ignored");
        return;
    }

    auto response = requests_.handle(frame);
    if (!response) return;

    // Rejected requests (e.g. KEY_TOO_LARGE) changed nothing: answer now.
    if (response->status != Status::Ok && response->status != Status::NotFound) {
        log(what + ": local -> " + str(to_string(response->status)));
        send_(response->message, from);
        return;
    }

    replicating_.insert(key);
    auto done = [this, key, from, what, header = frame.header,
                 response = std::move(*response)](Status replicated) {
        replicating_.erase(key);
        if (replicated == Status::Ok) {
            completed_.insert(key, response.message);
            log(what + ": local " + str(to_string(response.status)) + ", replica confirmed -> " +
                str(to_string(response.status)));
            send_(response.message, from);
        } else {
            // Not cached: a retry executes the write again, which is safe
            // (PUT and DELETE are idempotent) and may succeed this time.
            log(what + ": replica did not confirm -> REPLICATION_FAILED");
            reply_status(header, Status::ReplicationFailed, from);
        }
    };

    // RequestHandler applied the write to the store (a new value, or a
    // tombstone for DELETE); the replicator copies that entry, version included.
    const auto key_name = request_key(frame);
    replicator_.replicate(*key_name, std::move(done));
}

void Router::forward(const Frame& frame, const sockaddr_in6& client,
                     std::vector<cluster::NodeEntry> targets, std::string what) {
    Header header = frame.header;
    header.flags |= flags::kForwarded;

    Pending p;
    p.client = client;
    p.request = frame.header;
    p.message = encode_message(header, frame.payload);
    p.next = std::move(targets);
    p.what = std::move(what);
    try_next({frame.header.sender_id, frame.header.request_id}, std::move(p));
}

// Sends the request to the next node in p.next and waits for its answer.
// When nobody is left to ask, the client gets UNAVAILABLE.
void Router::try_next(const RequestKey& key, Pending p) {
    while (!p.next.empty()) {
        cluster::NodeEntry target = std::move(p.next.front());
        p.next.erase(p.next.begin());

        if (members_.is_self(target)) {  // we hold a replica: answer from our own store
            log(p.what + ": reading local replica");
            const auto frame = parse_datagram(p.message);  // we built it: always valid
            handle_locally(*frame, p.client);
            return;
        }

        if (!send_(p.message, target.addr)) {
            log(p.what + ": could not send to " + target.name);
            continue;
        }

        p.target = std::move(target);
        // If the node doesn't answer in time, try the next one. The callback
        // captures `this` and the key only; ~Router() cancels pending timers.
        p.timer = timers_.schedule_after(forward_timeout_, [this, key] {
            const auto it = pending_.find(key);
            if (it == pending_.end()) return;
            Pending timed_out = std::move(it->second);
            pending_.erase(it);

            log(timed_out.what + ": no answer from " + timed_out.target.name);
            try_next(key, std::move(timed_out));
        });
        pending_.insert_or_assign(key, std::move(p));
        return;
    }

    log(p.what + ": no node could answer -> UNAVAILABLE");
    reply_status(p.request, Status::Unavailable, p.client);
}

// ── Responses ───────────────────────────────────────────────────────────

void Router::handle_response(const Frame& frame, std::span<const std::byte> raw,
                             const sockaddr_in6& from) {
    const auto it = pending_.find({frame.header.sender_id, frame.header.request_id});
    if (it == pending_.end()) return;  // late (already timed out) or unknown: drop

    // Only the node we're waiting for may answer, and only with the matching type.
    if (!same_address(from, it->second.target.addr) ||
        frame.header.type != response_for(it->second.request.type)) {
        return;
    }

    timers_.cancel(it->second.timer);
    const sockaddr_in6 client = it->second.client;
    pending_.erase(it);

    log("relaying " + str(to_string(frame.header.type)) + " " +
        str(to_string(frame.header.status)) + " to " + to_string(client));
    send_(raw, client);  // unchanged: same request_id and sender_id the client expects
}

void Router::reply_status(const Header& request, Status status, const sockaddr_in6& to) {
    const auto message = encode_message({.type = response_for(request.type),
                                         .status = status,
                                         .request_id = request.request_id,
                                         .sender_id = request.sender_id});
    send_(message, to);
}
