// k_v_db — a key-value database node (single node for now, not distributed yet).
//
// Usage: k_v_db [udp_port]        (default port: 9000)

#include <charconv>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include <netinet/in.h>

#include "handlers/signal_handler.h"
#include "handlers/udp_handler.h"
#include "node/request_handler.h"
#include "protocol/codec.h"
#include "reactor/reactor.h"
#include "storage/kv_store.h"
#include "utils/address.h"

namespace {

constexpr std::uint16_t kDefaultPort = 9000;

std::optional<std::uint16_t> parse_port(std::string_view text) {
    std::uint16_t port = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), port);
    if (ec != std::errc{} || end != text.data() + text.size()) return std::nullopt;
    return port;
}

}  // namespace

int main(int argc, char** argv) {
    std::uint16_t port = kDefaultPort;
    if (argc > 2) {
        std::fprintf(stderr, "usage: %s [udp_port]\n", argv[0]);
        return 2;
    }
    if (argc == 2) {
        const auto parsed = parse_port(argv[1]);
        if (!parsed) {
            std::fprintf(stderr, "invalid port: %s\n", argv[1]);
            return 2;
        }
        port = *parsed;
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

    // ── UDP data channel: datagram -> codec -> request handler -> reply ─
    auto udp = std::make_unique<UDPHandler>(port);
    UDPHandler* udp_ptr = udp.get();

    udp->set_on_datagram([udp_ptr, &requests](std::span<const std::byte> data,
                                              const sockaddr_in6& peer) {
        const auto frame = protocol::parse_datagram(data);
        if (!frame) {
            const std::string_view why = protocol::to_string(frame.error());
            std::printf("%s: dropped invalid datagram (%.*s)\n", to_string(peer).c_str(),
                        static_cast<int>(why.size()), why.data());
            return;
        }

        auto response = requests.handle(*frame);
        if (!response) return;  // not a client request: ignored

        const std::string_view type = protocol::to_string(frame->header.type);
        const std::string_view status = protocol::to_string(response->status);
        std::printf("%s: %.*s -> %.*s\n", to_string(peer).c_str(),
                    static_cast<int>(type.size()), type.data(),
                    static_cast<int>(status.size()), status.data());

        udp_ptr->send_to(response->message, peer);
    });

    reactor.register_handler(std::move(udp));

    std::printf("k_v_db node listening on UDP port %u\n", udp_ptr->local_port());
    reactor.run();
    std::printf("stopped cleanly (%zu keys in store)\n", store.size());
}