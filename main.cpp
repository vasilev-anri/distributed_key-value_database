// k_v_db — a key-value database node.
//
// Usage:
//   k_v_db [--listen <endpoint>] [--peer <endpoint>]...
//
//   --listen   this node's address, as other nodes and clients reach it
//              (default [::1]:9000). The node listens on that port on all
//              interfaces; the full address also determines the node's id.
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
#include <vector>

#include <netinet/in.h>

#include "cluster/membership.h"
#include "handlers/signal_handler.h"
#include "handlers/timer_queue.h"
#include "handlers/udp_handler.h"
#include "node/request_handler.h"
#include "node/router.h"
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

int main(int argc, char** argv) {
    const auto opts = parse_args(argc, argv);
    if (!opts) return usage(argv[0]);

    // ── Cluster membership (static list for now) ────────────────────────
    cluster::Membership members(cluster::make_node(opts->listen));
    try {
        for (const sockaddr_in6& peer : opts->peers) members.add(cluster::make_node(peer));
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 2;
    }

    // Declared before the reactor, so they outlive every handler callback
    // that refers to them (locals are destroyed in reverse order).
    KeyValueStore store;
    RequestHandler requests(store);

    Reactor reactor;

    // ── Clean shutdown on Ctrl+C / SIGTERM ──────────────────────────────
    reactor.register_handler(std::make_unique<SignalHandler>(
        std::initializer_list<int>{SIGINT, SIGTERM},
        [&reactor](int signo) {
            std::printf("\nsignal %d (%s): shutting down\n", signo, ::strsignal(signo));
            reactor.stop();
        }));

    // ── Timers (forward timeouts) ───────────────────────────────────────
    auto timer_queue = std::make_unique<TimerQueue>();
    TimerQueue* timers = timer_queue.get();
    reactor.register_handler(std::move(timer_queue));

    // ── UDP data channel ────────────────────────────────────────────────
    auto udp = std::make_unique<UDPHandler>(ntohs(opts->listen.sin6_port));
    UDPHandler* udp_ptr = udp.get();

    // Declared after the reactor: destroyed first, while the TimerQueue it
    // cancels its timers on still exists.
    Router router(members, requests, *timers,
                  [udp_ptr](std::span<const std::byte> data, const sockaddr_in6& to) {
                      return udp_ptr->send_to(data, to);
                  });
    router.set_logger([](const std::string& line) { std::printf("%s\n", line.c_str()); });

    udp->set_on_datagram([&router](std::span<const std::byte> data, const sockaddr_in6& from) {
        router.on_datagram(data, from);
    });
    reactor.register_handler(std::move(udp));

    // ── Startup summary ─────────────────────────────────────────────────
    std::printf("k_v_db node %s (id %016lx)\n", members.self().name.c_str(),
                static_cast<unsigned long>(members.self().id));
    std::printf("cluster of %zu node(s):\n", members.nodes().size());
    for (const cluster::NodeEntry& node : members.nodes()) {
        std::printf("  %016lx  %s%s\n", static_cast<unsigned long>(node.id), node.name.c_str(),
                    members.is_self(node) ? "  (self)" : "");
    }

    reactor.run();
    std::printf("stopped cleanly (%zu keys stored on this node)\n", store.size());
}
