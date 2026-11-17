// k_v_db — a key-value database node.
//
// Usage:
//   k_v_db [--listen <endpoint>] [--peer <endpoint>]...
//
//   --listen   this node's address, as other nodes and clients reach it
//              (default [::1]:9000). The node listens on that port on all
//              interfaces: UDP for data, TCP for the control channel (same
//              port number). The full address also determines the node's id.
//   --peer     another node of the cluster (repeat for each one). Every node
//              must be started with the same set of nodes.
//
//   endpoint:  [::1]:9001  or  127.0.0.1:9001
//
// Example, a 3-node cluster on one machine:
//   k_v_db --listen [::1]:9001 --peer [::1]:9002 --peer [::1]:9003
//   k_v_db --listen [::1]:9002 --peer [::1]:9001 --peer [::1]:9003
//   k_v_db --listen [::1]:9003 --peer [::1]:9001 --peer [::1]:9002

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <netinet/in.h>

#include "cluster/membership.h"
#include "cluster/peer_manager.h"
#include "handlers/signal_handler.h"
#include "handlers/tcp_listener.h"
#include "handlers/timer_queue.h"
#include "handlers/udp_handler.h"
#include "node/replicator.h"
#include "node/request_handler.h"
#include "node/status_service.h"
#include "node/router.h"
#include "protocol/codec.h"
#include "protocol/protocol.h"
#include "reactor/reactor.h"
#include "storage/kv_store.h"
#include "utils/address.h"

namespace {

struct Options {
    sockaddr_in6 listen{};
    std::vector<sockaddr_in6> peers;
};

int usage(const char* prog) {
    std::fprintf(stderr,
                 "usage: %s [--listen <endpoint>] [--peer <endpoint>]...\n"
                 "  endpoint: [::1]:9001 or 127.0.0.1:9001\n",
                 prog);
    return 2;
}

// Returns std::nullopt (after printing why) if the arguments are invalid.
std::optional<Options> parse_args(int argc, char** argv) {
    Options opts;
    opts.listen = *make_address("::1", 9000);

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if ((arg == "--listen" || arg == "--peer") && i + 1 < argc) {
            const auto endpoint = parse_endpoint(argv[++i]);
            if (!endpoint) {
                std::fprintf(stderr, "invalid endpoint: %s\n", argv[i]);
                return std::nullopt;
            }
            if (arg == "--listen") opts.listen = *endpoint;
            else opts.peers.push_back(*endpoint);
        } else {
            return std::nullopt;
        }
    }
    return opts;
}

}  // namespace

// Builds the node and runs it until Ctrl+C / SIGTERM. Setup failures (e.g.
// the port is already taken) throw std::system_error.
int run(const Options& opts) {
    // ── Cluster membership (static list for now) ────────────────────────
    cluster::Membership members(cluster::make_node(opts.listen));
    try {
        for (const sockaddr_in6& peer : opts.peers) members.add(cluster::make_node(peer));
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 2;
    }

    // Declared before the reactor, so they outlive every handler callback
    // that refers to them (locals are destroyed in reverse order).
    KeyValueStore store;
    RequestHandler requests(store);

    Reactor reactor;

    auto log_line = [](const std::string& line) { std::printf("%s\n", line.c_str()); };

    // ── Clean shutdown on Ctrl+C / SIGTERM ──────────────────────────────
    reactor.register_handler(std::make_unique<SignalHandler>(
        std::initializer_list<int>{SIGINT, SIGTERM},
        [&reactor](int signo) {
            std::printf("\nsignal %d (%s): shutting down\n", signo, ::strsignal(signo));
            reactor.stop();
        }));

    // ── Timers (forward / replication timeouts, reconnects) ────────────
    auto timer_queue = std::make_unique<TimerQueue>();
    TimerQueue* timers = timer_queue.get();
    reactor.register_handler(std::move(timer_queue));

    // The order of the next declarations matters. They are destroyed in
    // reverse order, so: Router (uses Replicator) goes first, then Replicator
    // (sends through PeerManager), then PeerManager; all of them before the
    // Reactor, which owns the TimerQueue they cancel their timers on.

    // ── TCP control channel (same port number as UDP) ───────────────────
    cluster::PeerManager peers(members, reactor, *timers);
    peers.set_logger(log_line);

    // ── Replication (over the control channel) ──────────────────────────
    Replicator replicator(members, store, *timers,
                          [&peers](std::uint64_t peer_id, std::span<const std::byte> message) {
                              return peers.send(peer_id, message);
                          });
    replicator.set_logger(log_line);

    // Every change of a peer's state is printed. When a peer becomes
    // CONNECTED -- at startup, or after it restarted or was unreachable --
    // we send it every entry it is responsible for (recovery). The peer does
    // the same towards us, so both sides end up with the newest copies.
    peers.set_on_state([&peers, &members, &replicator](const cluster::NodeEntry& peer,
                                                       cluster::PeerManager::PeerState state) {
        std::printf("peer %s: %s  (%zu/%zu peers connected)\n", peer.name.c_str(),
                    cluster::to_string(state), peers.connected_count(),
                    members.nodes().size() - 1);
        if (state == cluster::PeerManager::PeerState::Connected) replicator.sync_to(peer.id);
    });

    peers.set_on_message([&replicator](std::uint64_t peer_id, const protocol::Frame& frame) {
        if (replicator.on_message(peer_id, frame)) return;
        // PING / PONG never get here: PeerSession answers them itself.
        const std::string_view type = protocol::to_string(frame.header.type);
        std::printf("control: %.*s from %016lx (not handled yet)\n",
                    static_cast<int>(type.size()), type.data(), static_cast<unsigned long>(peer_id));
    });

    // ── UDP data channel ────────────────────────────────────────────────
    auto udp = std::make_unique<UDPHandler>(ntohs(opts.listen.sin6_port));
    UDPHandler* udp_ptr = udp.get();

    Router router(members, requests, replicator, *timers,
                  [udp_ptr](std::span<const std::byte> data, const sockaddr_in6& to) {
                      return udp_ptr->send_to(data, to);
                  });
    router.set_logger(log_line);
    // A peer counts as alive while its control connection is up -- which,
    // thanks to heartbeats, also means it answered within the last second.
    router.set_liveness([&peers](std::uint64_t node_id) {
        return peers.state(node_id) == cluster::PeerManager::PeerState::Connected;
    });

    // ── STATUS / LOCATE (cluster information for the UI and kvcli) ──────
    StatusService status(members, store, [&peers](std::uint64_t peer_id) {
        switch (peers.state(peer_id)) {
            case cluster::PeerManager::PeerState::Connected: return protocol::NodeState::Connected;
            case cluster::PeerManager::PeerState::Connecting: return protocol::NodeState::Connecting;
            case cluster::PeerManager::PeerState::Disconnected: break;
        }
        return protocol::NodeState::Disconnected;
    });

    // Every datagram: STATUS / LOCATE are answered right here, everything
    // else (PUT / GET / DELETE and their responses) goes to the Router.
    udp->set_on_datagram([&router, &status, udp_ptr](std::span<const std::byte> data,
                                                     const sockaddr_in6& from) {
        if (const auto frame = protocol::parse_datagram(data)) {
            if (const auto reply = status.handle(*frame)) {
                udp_ptr->send_to(*reply, from);
                return;
            }
        }
        router.on_datagram(data, from);  // also reports invalid datagrams
    });
    reactor.register_handler(std::move(udp));

    reactor.register_handler(std::make_unique<TCPListener>(
        ntohs(opts.listen.sin6_port),
        [&peers](UniqueFD client, const sockaddr_in6& from) { peers.accept(std::move(client), from); }));

    // ── Startup summary ─────────────────────────────────────────────────
    std::printf("k_v_db node %s (id %016lx)\n", members.self().name.c_str(),
                static_cast<unsigned long>(members.self().id));
    std::printf("cluster of %zu node(s):\n", members.nodes().size());
    for (const cluster::NodeEntry& node : members.nodes()) {
        std::printf("  %016lx  %s%s\n", static_cast<unsigned long>(node.id), node.name.c_str(),
                    members.is_self(node) ? "  (self)" : "");
    }
    std::printf("replication factor %zu, heartbeat every %lld ms, peer down after %lld ms\n",
                cluster::kReplicationFactor,
                static_cast<long long>(cluster::PeerManager::kHeartbeatInterval.count()),
                static_cast<long long>(cluster::PeerManager::kFailureTimeout.count()));

    peers.start();  // connect to the peers we're responsible for
    reactor.run();
    std::printf("stopped cleanly (%zu keys stored on this node)\n", store.size());
    return 0;
}

int main(int argc, char** argv) {
    const auto opts = parse_args(argc, argv);
    if (!opts) return usage(argv[0]);

    try {
        return run(*opts);
    } catch (const std::system_error& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        if (e.code() == std::errc::address_in_use)
            std::fprintf(stderr, "  port %u is taken: is another node already running? (pgrep -af k_v_db)\n",
                         static_cast<unsigned>(ntohs(opts->listen.sin6_port)));
        return 1;
    }
}
