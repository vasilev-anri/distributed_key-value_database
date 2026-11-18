// k_v_db — a key-value database node.
//
// Usage:
//   k_v_db [--listen <endpoint>] [--peer <endpoint>]...
//          [--dht <port> [--bootstrap <endpoint>] [--cluster <name>]]
//
//   --listen     this node's address, as other nodes and clients reach it
//                (default [::1]:9000). The node listens on that port on all
//                interfaces: UDP for data, TCP for the control channel (same
//                port number). The full address also determines the node's id.
//   --peer       another node of the cluster, known in advance (repeat for each).
//   --dht        discover the other nodes automatically through OpenDHT, which
//                uses this UDP port. Nodes found this way join at runtime.
//   --bootstrap  an existing node's DHT endpoint to join through (omit on the
//                first node).
//   --cluster    nodes with the same cluster name find each other (default kvdb).
//
//   endpoint:  [::1]:9001  or  127.0.0.1:9001
//
// Example, a 3-node cluster with discovery (no --peer lists needed):
//   k_v_db --listen [::1]:9001 --dht 4001
//   k_v_db --listen [::1]:9002 --dht 4002 --bootstrap [::1]:4001
//   k_v_db --listen [::1]:9003 --dht 4003 --bootstrap [::1]:4001
//
// The same cluster with a static list:
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

#include <arpa/inet.h>
#include <netinet/in.h>

#include "cluster/membership.h"
#include "cluster/peer_manager.h"
#include "dht/dht_node.h"
#include "handlers/mailbox.h"
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
    std::optional<std::uint16_t> dht_port;  // set: discovery through OpenDHT
    std::string bootstrap_host;             // empty: first node of the DHT
    std::string bootstrap_port;
    std::string cluster = "kvdb";
};

int usage(const char* prog) {
    std::fprintf(stderr,
                 "usage: %s [--listen <endpoint>] [--peer <endpoint>]...\n"
                 "          [--dht <port> [--bootstrap <endpoint>] [--cluster <name>]]\n"
                 "  endpoint: [::1]:9001 or 127.0.0.1:9001\n",
                 prog);
    return 2;
}

std::optional<std::uint16_t> parse_port(std::string_view text) {
    if (text.empty() || text.size() > 5) return std::nullopt;
    unsigned port = 0;
    for (const char c : text) {
        if (c < '0' || c > '9') return std::nullopt;
        port = port * 10 + static_cast<unsigned>(c - '0');
    }
    if (port == 0 || port > 65535) return std::nullopt;
    return static_cast<std::uint16_t>(port);
}

// The IP part of an address as text, e.g. "::1" or "::ffff:127.0.0.1".
std::string ip_of(const sockaddr_in6& addr) {
    char buf[INET6_ADDRSTRLEN];
    ::inet_ntop(AF_INET6, &addr.sin6_addr, buf, sizeof buf);
    return buf;
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
        } else if (arg == "--dht" && i + 1 < argc) {
            opts.dht_port = parse_port(argv[++i]);
            if (!opts.dht_port) {
                std::fprintf(stderr, "invalid DHT port: %s\n", argv[i]);
                return std::nullopt;
            }
        } else if (arg == "--bootstrap" && i + 1 < argc) {
            const auto endpoint = parse_endpoint(argv[++i]);
            if (!endpoint) {
                std::fprintf(stderr, "invalid endpoint: %s\n", argv[i]);
                return std::nullopt;
            }
            opts.bootstrap_host = ip_of(*endpoint);
            opts.bootstrap_port = std::to_string(ntohs(endpoint->sin6_port));
        } else if (arg == "--cluster" && i + 1 < argc) {
            opts.cluster = argv[++i];
        } else {
            return std::nullopt;
        }
    }
    if (!opts.dht_port && (!opts.bootstrap_host.empty() || opts.cluster != "kvdb")) {
        std::fprintf(stderr, "--bootstrap and --cluster need --dht\n");
        return std::nullopt;
    }
    return opts;
}

}  // namespace

// Builds the node and runs it until Ctrl+C / SIGTERM. Setup failures (e.g.
// the port is already taken) throw std::system_error.
int run(const Options& opts) {
    // ── Cluster membership: the static --peer list; discovery adds more ──
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

    // ── Joining at runtime ──────────────────────────────────────────────
    // A node we didn't know yet: add it to the membership (ownership changes
    // at once) and to the control channel. When its connection comes up, the
    // sync in on_state() sends it the keys it is now responsible for.
    auto join = [&members, &peers](const sockaddr_in6& addr) {
        cluster::NodeEntry node = cluster::make_node(addr);
        if (members.find(node.id)) return;  // already a member
        const std::uint64_t id = node.id;
        const std::string name = node.name;
        members.add(std::move(node));
        std::printf("discovery: %s joined, cluster now has %zu nodes\n", name.c_str(),
                    members.nodes().size());
        peers.add_peer(id);
    };

    // ── Discovery through OpenDHT (optional) ────────────────────────────
    // Declared after everything `join` uses, so it is destroyed first: its
    // destructor stops OpenDHT's threads, after which no task can be posted.
    std::unique_ptr<DHTNode> dht;
    if (opts.dht_port) {
        auto mailbox = std::make_unique<Mailbox>();
        Mailbox& mb = *mailbox;
        reactor.register_handler(std::move(mailbox));

        dht = std::make_unique<DHTNode>(mb);
        dht->start({.port = *opts.dht_port,
                    .bootstrap_host = opts.bootstrap_host,
                    .bootstrap_port = opts.bootstrap_port,
                    .cluster_name = opts.cluster});

        const std::uint16_t port = ntohs(opts.listen.sin6_port);
        dht->announce({.node_id = members.self().id, .ip = ip_of(opts.listen), .udp_port = port, .tcp_port = port});

        // Both callbacks run on the reactor thread (DHTNode posts them to the Mailbox).
        dht->discover(
            [&join](const NodeInfo& info) {
                const auto addr = make_address(info.ip, info.udp_port);
                if (!addr) {
                    std::printf("discovery: ignored invalid announcement (ip \"%s\")\n", info.ip.c_str());
                    return;
                }
                // Never trust the network: the id must be the one its address gives.
                if (cluster::make_node(*addr).id != info.node_id) {
                    std::printf("discovery: ignored %s: announced id doesn't match its address\n",
                                to_string(*addr).c_str());
                    return;
                }
                join(*addr);
            },
            [](const NodeInfo& info) {
                // Nodes are never removed: a node that is gone shows as down
                // (heartbeats) and gets its keys back when it returns.
                std::printf("discovery: announcement of [%s]:%u expired\n", info.ip.c_str(),
                            static_cast<unsigned>(info.udp_port));
            });

        std::printf("discovery: OpenDHT on UDP port %u, cluster \"%s\", %s\n",
                    static_cast<unsigned>(*opts.dht_port), opts.cluster.c_str(),
                    opts.bootstrap_host.empty()
                        ? "first node (others bootstrap from us)"
                        : ("bootstrapping from [" + opts.bootstrap_host + "]:" + opts.bootstrap_port).c_str());
    }

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
