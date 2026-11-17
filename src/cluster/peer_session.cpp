#include "cluster/peer_session.h"

#include <utility>

#include "protocol/codec.h"

using namespace protocol;

namespace cluster {

namespace {

std::error_code protocol_error() { return std::make_error_code(std::errc::protocol_error); }

}  // namespace

PeerSession::PeerSession(Role role, Identity self, std::uint64_t peer_id, State initial,
                         Callbacks callbacks)
    : role_(role), self_(self), peer_id_(peer_id), state_(initial), cb_(std::move(callbacks)) {}

PeerSession PeerSession::outbound(Identity self, std::uint64_t expected_peer, Callbacks callbacks) {
    return PeerSession(Role::Outbound, self, expected_peer, State::Connecting, std::move(callbacks));
}

PeerSession PeerSession::inbound(Identity self, Callbacks callbacks) {
    return PeerSession(Role::Inbound, self, 0, State::AwaitingHello, std::move(callbacks));
}

void PeerSession::on_connected() {
    if (role_ != Role::Outbound || state_ != State::Connecting) return;

    state_ = State::AwaitingHelloAck;
    const auto hello = encode_message({.type = MessageType::Hello, .sender_id = self_.node_id},
                                      encode_payload(HelloPayload{self_.udp_port, self_.tcp_port}));
    if (!cb_.send(hello)) close(std::make_error_code(std::errc::broken_pipe));
}

std::size_t PeerSession::on_data(std::span<const std::byte> data) {
    std::size_t consumed = 0;

    while (state_ != State::Closed) {
        const auto parsed = try_parse_message(data.subspan(consumed));
        if (!parsed) {  // invalid stream: we can't find the next message boundary
            close(protocol_error());
            break;
        }
        if (!*parsed) break;  // incomplete message: wait for more bytes

        const Frame& frame = **parsed;
        consumed += frame.size;
        handle(frame);  // may close the session
    }
    return consumed;
}

void PeerSession::handle(const Frame& frame) {
    const MessageType type = frame.header.type;

    switch (state_) {
        case State::AwaitingHello: {  // inbound: the peer must introduce itself first
            if (type != MessageType::Hello || !decode_hello(frame.payload)) {
                close(protocol_error());
                return;
            }
            const std::uint64_t claimed = frame.header.sender_id;
            if (claimed == self_.node_id || !cb_.accept_peer || !cb_.accept_peer(claimed)) {
                close(std::make_error_code(std::errc::permission_denied));
                return;
            }

            peer_id_ = claimed;
            state_ = State::Established;
            const auto ack = encode_message({.type = MessageType::HelloAck,
                                             .request_id = frame.header.request_id,
                                             .sender_id = self_.node_id});
            if (!cb_.send(ack)) {
                close(std::make_error_code(std::errc::broken_pipe));
                return;
            }
            if (cb_.established) cb_.established(peer_id_);
            return;
        }

        case State::AwaitingHelloAck: {  // outbound: the right node must answer
            if (type != MessageType::HelloAck || frame.header.sender_id != peer_id_) {
                close(protocol_error());
                return;
            }
            state_ = State::Established;
            if (cb_.established) cb_.established(peer_id_);
            return;
        }

        case State::Established: {
            if (type == MessageType::Hello || type == MessageType::HelloAck) {
                close(protocol_error());  // the handshake happens once
                return;
            }
            if (type == MessageType::Ping || type == MessageType::Pong) {
                handle_heartbeat(frame);
                return;
            }
            if (cb_.message) cb_.message(peer_id_, frame);
            return;
        }

        case State::Connecting:  // no data before the connection is up
        case State::Closed:
            return;
    }
}

// ── Heartbeats ──────────────────────────────────────────────────────────

void PeerSession::heartbeat() {
    if (state_ != State::Established) return;

    // Each call is one interval. If even the oldest of the last
    // kMaxMissedPongs PINGs is still unanswered, the peer is gone.
    if (unanswered_ >= kMaxMissedPongs) {
        close(std::make_error_code(std::errc::timed_out));
        return;
    }

    const auto ping = encode_message({.type = MessageType::Ping,
                                      .request_id = ++last_ping_id_,
                                      .sender_id = self_.node_id});
    ++unanswered_;
    if (!cb_.send(ping)) close(std::make_error_code(std::errc::broken_pipe));
}

void PeerSession::handle_heartbeat(const Frame& frame) {
    if (frame.header.type == MessageType::Ping) {
        // Answer with the same request_id, so the peer knows which PING it was.
        const auto pong = encode_message({.type = MessageType::Pong,
                                          .request_id = frame.header.request_id,
                                          .sender_id = self_.node_id});
        if (!cb_.send(pong)) close(std::make_error_code(std::errc::broken_pipe));
        return;
    }

    // PONG: an answer to any PING we actually sent proves the peer is alive
    // (PONGs to older PINGs may arrive late; that's fine). A PONG for a PING
    // we never sent is ignored.
    const std::uint64_t id = frame.header.request_id;
    if (id != 0 && id <= last_ping_id_) unanswered_ = 0;
}

bool PeerSession::send(std::span<const std::byte> message) {
    if (state_ != State::Established) return false;
    return cb_.send(message);
}

void PeerSession::close(std::error_code reason) {
    if (state_ == State::Closed) return;
    state_ = State::Closed;
    if (cb_.close) cb_.close(reason);
}

}  // namespace cluster
