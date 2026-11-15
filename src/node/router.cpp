#include "node/router.h"

#include <cctype>
#include <cstring>
#include <optional>
#include <string_view>
#include <utility>

#include "cluster/membership.h"
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

Router::Router(cluster::Membership& members, RequestHandler& requests, TimerQueue& timers,
               SendFn send, std::chrono::milliseconds forward_timeout)
    : members_(members),
      requests_(requests),
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

    const cluster::NodeEntry& owner = members_.owner_of(*key);
    if (members_.is_self(owner)) {
        handle_locally(frame, from);
        return;
    }

    const std::string what = str(to_string(frame.header.type)) + " " + printable(*key);

    // Loop protection: a forwarded request is never forwarded again. If we
    // aren't the owner either, our views of the cluster disagree right now.
    if (frame.header.flags & flags::kForwarded) {
        log(what + ": forwarded to us but owner is " + owner.name + " -> UNAVAILABLE");
        reply_status(frame.header, Status::Unavailable, from);
        return;
    }

    // A client retry while the first attempt is still in flight: the pending
    // forward will answer it, so don't forward a second copy.
    if (pending_.contains({frame.header.sender_id, frame.header.request_id})) {
        log(what + ": retry of a request already forwarded, ignored");
        return;
    }

    log(what + ": owner is " + owner.name + ", forwarding");
    forward(frame, from, owner.addr, owner.name);
}

void Router::handle_locally(const Frame& frame, const sockaddr_in6& from) {
    const auto response = requests_.handle(frame);
    if (!response) return;

    const auto key = request_key(frame);
    log(str(to_string(frame.header.type)) + " " + (key ? printable(*key) : std::string("?")) +
        ": local -> " + str(to_string(response->status)));
    send_(response->message, from);
}

void Router::forward(const Frame& frame, const sockaddr_in6& client, const sockaddr_in6& owner,
                     const std::string& owner_name) {
    Header header = frame.header;
    header.flags |= flags::kForwarded;
    const auto message = encode_message(header, frame.payload);

    const RequestKey key{header.sender_id, header.request_id};

    if (!send_(message, owner)) {
        log("could not send to " + owner_name + " -> UNAVAILABLE");
        reply_status(frame.header, Status::Unavailable, client);
        return;
    }

    // If the owner doesn't answer in time, tell the client instead of
    // leaving it waiting. The callback captures `this` and the key only;
    // ~Router() cancels any timer still pending.
    const TimerQueue::TimerId timer =
        timers_.schedule_after(forward_timeout_, [this, key, owner_name] {
            const auto it = pending_.find(key);
            if (it == pending_.end()) return;

            Header request{};
            request.type = it->second.type;
            request.sender_id = key.sender_id;
            request.request_id = key.request_id;
            const sockaddr_in6 client_addr = it->second.client;
            pending_.erase(it);

            log("no answer from " + owner_name + " -> UNAVAILABLE");
            reply_status(request, Status::Unavailable, client_addr);
        });

    pending_.emplace(key, Pending{client, owner, frame.header.type, timer});
}

// ── Responses ───────────────────────────────────────────────────────────

void Router::handle_response(const Frame& frame, std::span<const std::byte> raw,
                             const sockaddr_in6& from) {
    const auto it = pending_.find({frame.header.sender_id, frame.header.request_id});
    if (it == pending_.end()) return;  // late (already timed out) or unknown: drop

    // Only the node we forwarded to may answer, and only with the matching type.
    if (!same_address(from, it->second.owner) ||
        frame.header.type != response_for(it->second.type)) {
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
