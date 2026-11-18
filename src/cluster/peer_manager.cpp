#include "cluster/peer_manager.h"

#include <algorithm>
#include <exception>
#include <system_error>
#include <utility>

#include "cluster/membership.h"
#include "handlers/tcp_connection.h"
#include "reactor/reactor.h"
#include "utils/address.h"

namespace cluster {

const char* to_string(PeerManager::PeerState state) noexcept {
    switch (state) {
        case PeerManager::PeerState::Disconnected: return "DISCONNECTED";
        case PeerManager::PeerState::Connecting: return "CONNECTING";
        case PeerManager::PeerState::Connected: return "CONNECTED";
    }
    return "UNKNOWN";
}

PeerManager::PeerManager(Membership& members, Reactor& reactor, TimerQueue& timers)
    : members_(members), reactor_(reactor), timers_(timers) {
    const NodeEntry& self = members_.self();
    const std::uint16_t port = ntohs(self.addr.sin6_port);
    identity_ = {self.id, port, port};  // TCP control port = UDP data port

    for (const NodeEntry& node : members_.nodes()) {
        if (members_.is_self(node)) continue;
        Peer peer;
        peer.id = node.id;
        peer.we_connect = self.id < node.id;  // the lower id connects
        peers_.emplace(node.id, peer);
    }
}

PeerManager::~PeerManager() {
    // Every pending timer captures `this` (or a Link): none may fire later.
    for (const auto& [id, peer] : peers_) timers_.cancel(peer.reconnect_timer);
    for (const auto& link : links_) timers_.cancel(link->handshake_timer);
    timers_.cancel(gc_timer_);
    timers_.cancel(heartbeat_timer_);
}

void PeerManager::log(const std::string& line) {
    if (log_) log_(line);
}

const NodeEntry* PeerManager::node(std::uint64_t id) const {
    for (const NodeEntry& n : members_.nodes())
        if (n.id == id) return &n;
    return nullptr;
}

// ── Starting connections ────────────────────────────────────────────────

void PeerManager::start() {
    started_ = true;
    for (auto& [id, peer] : peers_) {
        if (peer.we_connect) connect(id);
    }
    if (heartbeat_timer_ == 0)
        heartbeat_timer_ = timers_.schedule_every(kHeartbeatInterval, [this] { send_heartbeats(); });
}

bool PeerManager::add_peer(std::uint64_t node_id) {
    if (node_id == members_.self().id || peers_.contains(node_id) || !node(node_id)) return false;

    Peer peer;
    peer.id = node_id;
    peer.we_connect = members_.self().id < node_id;  // the lower id connects
    peers_.emplace(node_id, peer);

    if (started_ && peer.we_connect) connect(node_id);
    return true;
}

// One heartbeat round over every connection. A session that has missed too
// many PONGs closes itself, which ends up in on_link_closed() and removes it
// from links_ -- so iterate over a copy of the pointers, not links_ itself.
// (Closed Links stay alive in the graveyard until after this event.)
void PeerManager::send_heartbeats() {
    std::vector<Link*> links;
    links.reserve(links_.size());
    for (const auto& link : links_) links.push_back(link.get());

    for (Link* link : links) link->session->heartbeat();
}

void PeerManager::connect(std::uint64_t peer_id) {
    Peer& peer = peers_.at(peer_id);
    peer.reconnect_timer = 0;
    const NodeEntry& target = *node(peer_id);

    std::unique_ptr<TCPConnection> conn;
    try {
        conn = TCPConnection::connect_to(target.addr);
    } catch (const std::system_error& e) {
        if (peer.backoff == kMinBackoff)
            log("control: cannot reach " + target.name + " (" + e.code().message() + "), will keep retrying");
        set_state(peer, PeerState::Connecting);
        schedule_reconnect(peer);
        return;
    }

    Link& link = add_link(std::move(conn), target.name, peer_id);
    peer.link = &link;
    set_state(peer, PeerState::Connecting);
}

void PeerManager::accept(UniqueFD fd, const sockaddr_in6& from) {
    add_link(std::make_unique<TCPConnection>(std::move(fd)), to_string(from), std::nullopt);
}

PeerManager::Link& PeerManager::add_link(std::unique_ptr<TCPConnection> conn, std::string remote,
                                         std::optional<std::uint64_t> expected_peer) {
    links_.push_back(std::make_unique<Link>());
    Link* link = links_.back().get();
    link->remote = std::move(remote);
    link->conn = conn.get();

    link->session.emplace(expected_peer
                              ? PeerSession::outbound(identity_, *expected_peer, callbacks_for(link))
                              : PeerSession::inbound(identity_, callbacks_for(link)));

    TCPConnection* c = conn.get();
    c->set_on_connected([link] { link->session->on_connected(); });
    c->set_on_data([link](std::span<const std::byte> data) { return link->session->on_data(data); });
    c->set_on_want_write([this, c](bool want) { reactor_.modify_handler(c, want); });
    c->set_on_closed([this, link](std::error_code reason) { on_link_closed(link, reason); });

    // The handshake (and, for outbound links, the TCP connect) must finish in time.
    link->handshake_timer = timers_.schedule_after(kHandshakeTimeout, [link] {
        link->handshake_timer = 0;
        link->conn->close(std::make_error_code(std::errc::timed_out));
    });

    reactor_.register_handler(std::move(conn), c->wants_write());
    return *link;
}

PeerSession::Callbacks PeerManager::callbacks_for(Link* link) {
    PeerSession::Callbacks cb;
    cb.send = [link](std::span<const std::byte> data) { return link->conn->send(data); };
    cb.close = [link](std::error_code reason) { link->conn->close(reason); };
    cb.established = [this, link](std::uint64_t peer_id) { on_established(link, peer_id); };
    cb.message = [this](std::uint64_t peer_id, const protocol::Frame& frame) {
        if (on_message_) on_message_(peer_id, frame);
    };
    // Only known nodes with a LOWER id than ours connect to us.
    cb.accept_peer = [this](std::uint64_t peer_id) {
        const auto it = peers_.find(peer_id);
        return it != peers_.end() && !it->second.we_connect;
    };
    return cb;
}

// ── Handshake done ──────────────────────────────────────────────────────

void PeerManager::on_established(Link* link, std::uint64_t peer_id) {
    timers_.cancel(link->handshake_timer);
    link->handshake_timer = 0;

    // A stale connection: the peer sent HELLO but closed its side before we
    // even answered. That happens when we were frozen or overloaded: the
    // kernel kept accepting the peer's reconnect attempts into the listen
    // backlog, and each one timed out on the peer's side. Accepting it would
    // replace a good connection with a dead one, so drop it quietly.
    if (link->session->role() == PeerSession::Role::Inbound && link->conn->peer_finished()) {
        log("control: dropped stale connection from " + node(peer_id)->name +
            " (the peer had already given up on it)");
        link->conn->close(std::make_error_code(std::errc::connection_aborted));
        return;
    }

    Peer& peer = peers_.at(peer_id);

    // The peer reconnected while we still had an old connection to it (e.g. it
    // restarted before we noticed): the new connection replaces the old one.
    // Point peer.link at the new one FIRST, so closing the old one below
    // isn't mistaken for "the peer disconnected".
    Link* old = peer.link;
    peer.link = link;
    if (old && old != link) old->conn->close(std::make_error_code(std::errc::connection_aborted));

    peer.backoff = kMinBackoff;
    set_state(peer, PeerState::Connected);
}

// ── Connection gone ─────────────────────────────────────────────────────

void PeerManager::on_link_closed(Link* link, std::error_code reason) {
    link->session->on_closed();
    timers_.cancel(link->handshake_timer);
    link->handshake_timer = 0;
    reactor_.unregister_handler(link->conn);  // destroyed by the reactor after this batch

    const std::uint64_t peer_id = link->session->peer_id();
    const auto it = peers_.find(peer_id);
    if (it != peers_.end() && it->second.link == link) {
        Peer& peer = it->second;
        peer.link = nullptr;
        std::string why = reason ? reason.message() : "closed by peer";
        if (peer.state == PeerState::Connected && reason == std::errc::timed_out) {
            why = "no heartbeat for " + std::to_string(kFailureTimeout.count()) + " ms";
        }

        if (peer.state == PeerState::Connected) {
            log("control: lost connection to " + node(peer_id)->name + ": " + why);
        } else if (peer.backoff == kMinBackoff) {  // first failed attempt: say it once
            log("control: cannot reach " + node(peer_id)->name + " (" + why + "), will keep retrying");
        }

        if (peer.we_connect) {
            set_state(peer, PeerState::Connecting);  // still trying
            schedule_reconnect(peer);
        } else {
            set_state(peer, PeerState::Disconnected);  // the peer will reconnect to us
        }
    } else if (peer_id == 0 && reason) {
        log("control: rejected connection from " + link->remote + ": " + reason.message());
    }

    // The Link may still be on the call stack (e.g. its session closed itself
    // while parsing), so it isn't destroyed here but after the current event.
    const auto pos = std::find_if(links_.begin(), links_.end(),
                                  [link](const auto& p) { return p.get() == link; });
    if (pos != links_.end()) {
        graveyard_.push_back(std::move(*pos));
        links_.erase(pos);
    }
    if (gc_timer_ == 0) {
        gc_timer_ = timers_.schedule_after(std::chrono::milliseconds(0), [this] {
            gc_timer_ = 0;
            collect_garbage();
        });
    }
}

void PeerManager::collect_garbage() { graveyard_.clear(); }

void PeerManager::schedule_reconnect(Peer& peer) {
    if (peer.reconnect_timer != 0) return;
    const std::chrono::milliseconds delay = peer.backoff;
    peer.backoff = std::min(peer.backoff * 2, kMaxBackoff);

    const std::uint64_t id = peer.id;
    peer.reconnect_timer = timers_.schedule_after(delay, [this, id] { connect(id); });
}

void PeerManager::set_state(Peer& peer, PeerState state) {
    if (peer.state == state) return;
    peer.state = state;
    if (on_state_) on_state_(*node(peer.id), state);
}

// ── Queries / sending ───────────────────────────────────────────────────

bool PeerManager::send(std::uint64_t peer_id, std::span<const std::byte> message) {
    const auto it = peers_.find(peer_id);
    if (it == peers_.end() || it->second.state != PeerState::Connected || !it->second.link)
        return false;
    return it->second.link->session->send(message);
}

PeerManager::PeerState PeerManager::state(std::uint64_t peer_id) const {
    const auto it = peers_.find(peer_id);
    return it == peers_.end() ? PeerState::Disconnected : it->second.state;
}

std::size_t PeerManager::connected_count() const noexcept {
    std::size_t n = 0;
    for (const auto& [id, peer] : peers_) n += peer.state == PeerState::Connected;
    return n;
}

}  // namespace cluster
