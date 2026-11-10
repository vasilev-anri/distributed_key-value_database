// Membership test: start several nodes; each should discover all the others.
//
// Usage:
//   terminal 1:  ./dht_spike 4222 ::1
//   terminal 2:  ./dht_spike 4223 ::1 127.0.0.1 4222

#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <random>
#include <string>

#include <unistd.h>

#include "dht/dht_node.h"
#include "handlers/mailbox.h"
#include "handlers/signal_handler.h"
#include "handlers/timer_queue.h"
#include "reactor/reactor.h"

using namespace std::chrono_literals;

int main(int argc, char** argv) {
    if (argc != 3 && argc != 5) {
        std::fprintf(stderr,
                     "usage: %s <dht_port> <advertised_ip> [bootstrap_host bootstrap_port]\n",
                     argv[0]);
        return 1;
    }
    const auto dht_port = static_cast<std::uint16_t>(std::stoi(argv[1]));

    Reactor reactor;

    // Before OpenDHT starts its threads, so they inherit the blocked signals.
    reactor.register_handler(std::make_unique<SignalHandler>(
        std::initializer_list<int>{SIGINT, SIGTERM},
        [&reactor](int) {
            std::puts("\nshutting down");
            reactor.stop();
        }));

    auto mailbox_handler = std::make_unique<Mailbox>();
    Mailbox* mailbox = mailbox_handler.get();
    reactor.register_handler(std::move(mailbox_handler));

    auto timer_queue = std::make_unique<TimerQueue>();
    TimerQueue* timers = timer_queue.get();
    reactor.register_handler(std::move(timer_queue));

    // ── Our identity (ports are placeholders for now) ───────────────────
    NodeInfo self;
    self.node_id = std::random_device{}() | (static_cast<std::uint64_t>(std::random_device{}()) << 32);
    self.ip = argv[2];
    self.udp_port = static_cast<std::uint16_t>(dht_port + 1000);
    self.tcp_port = static_cast<std::uint16_t>(dht_port + 2000);

    std::printf("node %016lx, reactor thread %d\n", self.node_id, ::gettid());

    // ── DHT ─────────────────────────────────────────────────────────────
    // Declared after `reactor`: destroyed first, and it outlives reactor.run().
    DHTNode dht(*mailbox);

    DHTNode::Config config;
    config.port = dht_port;
    if (argc == 5) {
        config.bootstrap_host = argv[3];
        config.bootstrap_port = argv[4];
    }

    dht.start(config);
    dht.discover(
        [](const NodeInfo& peer) {
            std::printf("FOUND node %016lx at %s (udp %u, tcp %u) [thread %d]\n",
                        peer.node_id, peer.ip.c_str(), peer.udp_port, peer.tcp_port, ::gettid());
        },
        [](const NodeInfo& peer) {
            std::printf("GONE  node %016lx\n", peer.node_id);
        });
    dht.announce(self);

    timers->schedule_every(5s, [&dht] {
        std::printf("dht: %zu known nodes\n", dht.good_nodes());
    });

    reactor.run();

    dht.shutdown();
    std::puts("stopped cleanly");
}