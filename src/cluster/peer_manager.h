#pragma once

// The TCP control channel of a node: one connection to every other node,
// a handshake on each, and a live table of which peers are connected.
//
// Who connects to whom: the node with the LOWER id connects, the other only
// accepts. So every pair of nodes has exactly one connection, never two.
//
// When a connection we initiated drops or fails, we reconnect with backoff
// (0.25 s, 0.5 s, 1 s ... up to 4 s). When one the peer initiated drops,
// we simply wait for the peer to reconnect.
//
// Failure detection: every kHeartbeatInterval each established connection
// sends a PING (see PeerSession). A peer that stops answering for
// kHeartbeatInterval * PeerSession::kMaxMissedPongs (1 s) is disconnected,
// so CONNECTED always means "connected AND answering".
//
// This is wiring code: it creates TCPConnections and registers them with the
// reactor. The protocol itself lives in PeerSession.

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include <netinet/in.h>

#include "cluster/peer_session.h"
#include "handlers/timer_queue.h"
#include "protocol/protocol.h"
#include "utils/unique_fd.h"

class Reactor;
class TCPConnection;

namespace cluster {

class Membership;
struct NodeEntry;

class PeerManager {
public:
    enum class PeerState { Disconnected, Connecting, Connected };

    using StateCallback = std::move_only_function<void(const NodeEntry& peer, PeerState state)>;
    using MessageCallback = std::move_only_function<void(std::uint64_t peer_id, const protocol::Frame&)>;
    using LogFn = std::move_only_function<void(const std::string& line)>;

    static constexpr std::chrono::milliseconds kHandshakeTimeout{3000};
    static constexpr std::chrono::milliseconds kMinBackoff{250};
    static constexpr std::chrono::milliseconds kMaxBackoff{4000};
    static constexpr std::chrono::milliseconds kHeartbeatInterval{250};
    static constexpr std::chrono::milliseconds kFailureTimeout =
        kHeartbeatInterval * PeerSession::kMaxMissedPongs;

    PeerManager(Membership& members, Reactor& reactor, TimerQueue& timers);
    ~PeerManager();

    PeerManager(const PeerManager&) = delete;
    PeerManager& operator=(const PeerManager&) = delete;

    void set_on_state(StateCallback cb) noexcept { on_state_ = std::move(cb); }
    void set_on_message(MessageCallback cb) noexcept { on_message_ = std::move(cb); }
    void set_logger(LogFn log) noexcept { log_ = std::move(log); }

    // Starts connecting to every peer we are responsible for connecting to.
    void start();

    // Hand every connection accepted by the TCPListener here.
    void accept(UniqueFD fd, const sockaddr_in6& from);

    // Sends a complete encoded control message. False if not connected.
    bool send(std::uint64_t peer_id, std::span<const std::byte> message);

    [[nodiscard]] PeerState state(std::uint64_t peer_id) const;
    [[nodiscard]] std::size_t connected_count() const noexcept;

private:
    // One TCP connection and its protocol state. Lives on the heap at a
    // stable address: callbacks capture a pointer to it.
    struct Link {
        std::optional<PeerSession> session;       // set right after the Link exists
        TCPConnection* conn = nullptr;            // owned by the reactor
        TimerQueue::TimerId handshake_timer = 0;
        std::string remote;                       // for logs
    };

    // Our view of one peer: the membership table row.
    struct Peer {
        std::uint64_t id = 0;
        PeerState state = PeerState::Disconnected;
        bool we_connect = false;                  // we have the lower id
        Link* link = nullptr;                     // the current connection, if any
        TimerQueue::TimerId reconnect_timer = 0;
        std::chrono::milliseconds backoff = kMinBackoff;
    };

    void connect(std::uint64_t peer_id);
    Link& add_link(std::unique_ptr<TCPConnection> conn, std::string remote,
                   std::optional<std::uint64_t> expected_peer);
    PeerSession::Callbacks callbacks_for(Link* link);
    void on_established(Link* link, std::uint64_t peer_id);
    void on_link_closed(Link* link, std::error_code reason);
    void schedule_reconnect(Peer& peer);
    void set_state(Peer& peer, PeerState state);
    void collect_garbage();
    void send_heartbeats();
    [[nodiscard]] const NodeEntry* node(std::uint64_t id) const;
    void log(const std::string& line);

    Membership& members_;
    Reactor& reactor_;
    TimerQueue& timers_;
    PeerSession::Identity identity_;

    std::unordered_map<std::uint64_t, Peer> peers_;
    std::vector<std::unique_ptr<Link>> links_;    // every live connection
    std::vector<std::unique_ptr<Link>> graveyard_;  // closed, destroyed after the current event
    TimerQueue::TimerId gc_timer_ = 0;
    TimerQueue::TimerId heartbeat_timer_ = 0;

    StateCallback on_state_;
    MessageCallback on_message_;
    LogFn log_;
};

[[nodiscard]] const char* to_string(PeerManager::PeerState state) noexcept;

}  // namespace cluster
