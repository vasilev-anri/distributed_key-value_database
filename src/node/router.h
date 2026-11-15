#pragma once

// Routes data-channel traffic in a multi-node cluster with replication.
//
// Every key has a replica set: its primary (the closest node by XOR distance)
// and one replica (the second closest). For every client request it decides:
//
//   PUT / DELETE
//     - we are the primary      -> apply locally, replicate (Replicator), and
//                                  answer only once the replica confirmed;
//     - another node is primary -> forward there and relay its answer;
//     - primary known down      -> UNAVAILABLE right away.
//   GET
//     - we are the primary      -> answer locally;
//     - otherwise               -> forward to the primary; if it doesn't
//                                  answer, ask the replica (that may be us).
//
// Duplicate detection: responses to executed writes are remembered, so a
// client retry gets the same answer instead of the write being applied twice.
//
// Loop protection: a request is forwarded at most once. A node that receives
// a FORWARDED request it can't serve answers UNAVAILABLE.
//
// Like the other node components it knows nothing about sockets: datagrams
// come in through on_datagram() and go out through the injected SendFn.

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <netinet/in.h>

#include "cluster/membership.h"
#include "handlers/timer_queue.h"
#include "node/response_cache.h"
#include "protocol/protocol.h"

class RequestHandler;
class Replicator;

class Router {
public:
    using SendFn = std::move_only_function<bool(std::span<const std::byte> data, const sockaddr_in6& to)>;
    using LivenessFn = std::move_only_function<bool(std::uint64_t node_id)>;
    using LogFn = std::move_only_function<void(const std::string& line)>;

    // How long to wait for a node we forwarded to. Two attempts (primary,
    // then replica) must fit within the client's timeout (500 ms in kvcli).
    static constexpr std::chrono::milliseconds kDefaultForwardTimeout{200};

    Router(cluster::Membership& members, RequestHandler& requests, Replicator& replicator,
           TimerQueue& timers, SendFn send,
           std::chrono::milliseconds forward_timeout = kDefaultForwardTimeout);

    ~Router();
    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    void set_logger(LogFn log) noexcept { log_ = std::move(log); }

    // Tells the router which peers are reachable right now (in the node: the
    // control channel is CONNECTED). Without it, every node is assumed alive.
    void set_liveness(LivenessFn alive) noexcept { alive_ = std::move(alive); }

    // Feed every datagram received on the data channel here.
    void on_datagram(std::span<const std::byte> data, const sockaddr_in6& from);

    // Requests forwarded and still waiting for an answer.
    [[nodiscard]] std::size_t pending_forwards() const noexcept { return pending_.size(); }

    // Writes we executed as primary that are waiting for the replica's ACK.
    [[nodiscard]] std::size_t pending_writes() const noexcept { return replicating_.size(); }

private:
    struct Pending {
        sockaddr_in6 client;                    // who asked us: the answer goes back here
        protocol::Header request;               // original request header (for error replies)
        std::vector<std::byte> message;         // the request as forwarded (FORWARDED flag set)
        std::vector<cluster::NodeEntry> next;   // nodes still to try, in order
        cluster::NodeEntry target;              // node asked right now: only it may answer
        TimerQueue::TimerId timer = 0;
        std::string what;                       // for logs, e.g. "GET student101"
    };

    void handle_request(const protocol::Frame& frame, const sockaddr_in6& from);
    void handle_response(const protocol::Frame& frame, std::span<const std::byte> raw,
                         const sockaddr_in6& from);
    void forward(const protocol::Frame& frame, const sockaddr_in6& client,
                 std::vector<cluster::NodeEntry> targets, std::string what);
    void try_next(const RequestKey& key, Pending p);
    void execute_write(const protocol::Frame& frame, const sockaddr_in6& from, const std::string& what);
    void handle_locally(const protocol::Frame& frame, const sockaddr_in6& from);
    void reply_status(const protocol::Header& request, protocol::Status status, const sockaddr_in6& to);
    bool is_alive(const cluster::NodeEntry& node);
    void log(const std::string& line);

    cluster::Membership& members_;
    RequestHandler& requests_;
    Replicator& replicator_;
    TimerQueue& timers_;
    SendFn send_;
    std::chrono::milliseconds forward_timeout_;
    LivenessFn alive_;
    LogFn log_;

    std::unordered_map<RequestKey, Pending, RequestKeyHash> pending_;  // forwarded, awaiting answer
    std::unordered_set<RequestKey, RequestKeyHash> replicating_;       // executing, awaiting replica
    ResponseCache completed_;                                          // executed writes' responses
};
