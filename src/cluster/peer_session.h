#pragma once

// The control-channel protocol of ONE TCP connection between two nodes.
//
//   outbound (we connected):   send HELLO        -> wait for HELLO_ACK -> established
//   inbound  (they connected): wait for HELLO    -> send HELLO_ACK     -> established
//
// After the handshake:
//   - heartbeats are handled here: PING is answered with PONG, and the owner
//     calls heartbeat() at a fixed interval to send PINGs. A peer that misses
//     kMaxMissedPongs PONGs in a row is considered dead: the session closes
//     with errc::timed_out. This catches a peer that is frozen or overloaded
//     but whose TCP connection is still open;
//   - every other control message is handed to the owner together with the
//     peer's node id.
// Any protocol violation closes the connection: on TCP, after one bad
// message the stream can't be trusted.
//
// It never touches a socket: bytes come in through on_data() and go out
// through the `send` callback, so it can be tested without a network.

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <system_error>

#include "protocol/protocol.h"

namespace cluster {

class PeerSession {
public:
    enum class Role { Outbound, Inbound };
    enum class State { Connecting, AwaitingHello, AwaitingHelloAck, Established, Closed };

    // Heartbeat intervals without a PONG before the peer is declared dead.
    static constexpr int kMaxMissedPongs = 4;

    // What we announce about ourselves in HELLO.
    struct Identity {
        std::uint64_t node_id = 0;
        std::uint16_t udp_port = 0;
        std::uint16_t tcp_port = 0;
    };

    struct Callbacks {
        // Sends bytes on the connection. Returns false if it couldn't.
        std::move_only_function<bool(std::span<const std::byte>)> send;
        // Closes the connection. Called at most once.
        std::move_only_function<void(std::error_code)> close;
        // The handshake completed; `peer_id` is now known and verified.
        std::move_only_function<void(std::uint64_t peer_id)> established;
        // A control message after the handshake (REPLICA_PUT, ...).
        // PING and PONG are handled by the session itself.
        std::move_only_function<void(std::uint64_t peer_id, const protocol::Frame&)> message;
        // Inbound only: may this node id connect to us?
        std::move_only_function<bool(std::uint64_t peer_id)> accept_peer;
    };

    // We connect to `expected_peer`: it must be the one answering.
    static PeerSession outbound(Identity self, std::uint64_t expected_peer, Callbacks callbacks);
    // Someone connected to us: we learn who from their HELLO.
    static PeerSession inbound(Identity self, Callbacks callbacks);

    // Outbound: the TCP connection is up, start the handshake.
    void on_connected();

    // All unconsumed bytes received so far. Returns how many were consumed
    // (complete messages). Fits TCPConnection's on_data contract directly.
    std::size_t on_data(std::span<const std::byte> data);

    // Sends a complete encoded message. Only after the handshake.
    bool send(std::span<const std::byte> message);

    // Call once per heartbeat interval. Established sessions send a PING; if
    // the previous kMaxMissedPongs PINGs all went unanswered, the session
    // closes with errc::timed_out instead. Does nothing before the handshake.
    void heartbeat();

    // Closes because of `reason` (protocol error, timeout, ...).
    void close(std::error_code reason);

    // The connection was closed from outside: just record it.
    void on_closed() noexcept { state_ = State::Closed; }

    [[nodiscard]] Role role() const noexcept { return role_; }
    [[nodiscard]] State state() const noexcept { return state_; }
    [[nodiscard]] bool established() const noexcept { return state_ == State::Established; }
    // 0 until known: inbound sessions learn it from HELLO.
    [[nodiscard]] std::uint64_t peer_id() const noexcept { return peer_id_; }
    // PINGs sent since the last PONG arrived (the last one may still be on its way).
    [[nodiscard]] int unanswered_pings() const noexcept { return unanswered_; }

private:
    PeerSession(Role role, Identity self, std::uint64_t peer_id, State initial, Callbacks callbacks);

    void handle(const protocol::Frame& frame);
    void handle_heartbeat(const protocol::Frame& frame);

    Role role_;
    Identity self_;
    std::uint64_t peer_id_;
    State state_;
    Callbacks cb_;

    std::uint64_t last_ping_id_ = 0;  // request_id of the last PING we sent
    int unanswered_ = 0;              // PINGs sent since the last PONG
};

}  // namespace cluster
