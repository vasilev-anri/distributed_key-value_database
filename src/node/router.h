#pragma once

// Routes data-channel traffic in a multi-node cluster.
//
// For every client request (PUT / GET / DELETE) it decides:
//   - we own the key            -> handle it locally (RequestHandler) and reply;
//   - another node owns it      -> forward it there, remember who asked, and
//                                  relay the owner's response back;
//   - it was already forwarded  -> never forward twice: reply UNAVAILABLE.
//
// Like the other node components it knows nothing about sockets: datagrams
// come in through on_datagram() and go out through the injected SendFn. That
// keeps it testable with a simulated network.

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <unordered_map>

#include <netinet/in.h>

#include "handlers/timer_queue.h"
#include "protocol/protocol.h"

class RequestHandler;
namespace cluster { class Membership; }

class Router {
public:
    using SendFn = std::move_only_function<bool(std::span<const std::byte> data, const sockaddr_in6& to)>;
    using LogFn = std::move_only_function<void(const std::string& line)>;

    // How long to wait for the owner before answering the client UNAVAILABLE.
    // Shorter than the client's own timeout, so the client gets an answer.
    static constexpr std::chrono::milliseconds kDefaultForwardTimeout{300};

    Router(cluster::Membership& members, RequestHandler& requests, TimerQueue& timers, SendFn send,
           std::chrono::milliseconds forward_timeout = kDefaultForwardTimeout);

    ~Router();
    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    void set_logger(LogFn log) noexcept { log_ = std::move(log); }

    // Feed every datagram received on the data channel here.
    void on_datagram(std::span<const std::byte> data, const sockaddr_in6& from);

    // Requests forwarded and still waiting for the owner's response.
    [[nodiscard]] std::size_t pending_forwards() const noexcept { return pending_.size(); }

private:
    // A request is identified by (sender_id, request_id), see docs/protocol.md.
    struct RequestKey {
        std::uint64_t sender_id;
        std::uint64_t request_id;
        bool operator==(const RequestKey&) const = default;
    };
    struct RequestKeyHash {
        std::size_t operator()(const RequestKey& k) const noexcept {
            return std::hash<std::uint64_t>{}(k.sender_id * 0x9e3779b97f4a7c15ull ^ k.request_id);
        }
    };
    struct Pending {
        sockaddr_in6 client;   // who asked us: the response goes back here
        sockaddr_in6 owner;    // who we forwarded to: only it may answer
        protocol::MessageType type;
        TimerQueue::TimerId timer = 0;
    };

    void handle_request(const protocol::Frame& frame, const sockaddr_in6& from);
    void handle_response(const protocol::Frame& frame, std::span<const std::byte> raw,
                         const sockaddr_in6& from);
    void forward(const protocol::Frame& frame, const sockaddr_in6& client,
                 const sockaddr_in6& owner, const std::string& owner_name);
    void handle_locally(const protocol::Frame& frame, const sockaddr_in6& from);
    void reply_status(const protocol::Header& request, protocol::Status status, const sockaddr_in6& to);
    void log(const std::string& line);

    cluster::Membership& members_;
    RequestHandler& requests_;
    TimerQueue& timers_;
    SendFn send_;
    std::chrono::milliseconds forward_timeout_;
    LogFn log_;

    std::unordered_map<RequestKey, Pending, RequestKeyHash> pending_;
};
