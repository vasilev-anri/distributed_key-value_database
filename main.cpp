#include <chrono>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

#include <netinet/in.h>

#include "handlers/tcp_connection.h"
#include "handlers/tcp_listener.h"
#include "handlers/timer_queue.h"
#include "handlers/udp_handler.h"
#include "reactor/reactor.h"
#include "utils/address.h"

using namespace std::chrono_literals;

namespace {

std::span<const std::byte> as_bytes(std::string_view s) {
    return std::as_bytes(std::span(s.data(), s.size()));
}

std::string to_text(std::span<const std::byte> data) {
    return {reinterpret_cast<const char*>(data.data()), data.size()};
}

// Test: connect out (with a timeout), send one message, print the echo, close.
void start_outbound_test(Reactor& reactor, TimerQueue& timers, const std::string& ip,
                         std::uint16_t port, TimerQueue::Clock::duration timeout) {
    const auto addr = make_address(ip, port);
    if (!addr) {
        std::fprintf(stderr, "invalid address %s\n", ip.c_str());
        return;
    }
    const std::string peer = to_string(*addr);
    std::printf("out: connecting to %s\n", peer.c_str());

    std::unique_ptr<TCPConnection> conn;
    try {
        conn = TCPConnection::connect_to(*addr);
    } catch (const std::system_error& e) {
        std::printf("out: %s failed immediately: %s\n", peer.c_str(), e.code().message().c_str());
        return;
    }
    TCPConnection* c = conn.get();

    // Queued while still connecting; sent once the connection is up.
    c->send(as_bytes("hello from connector\n"));

    // Give up if the connect hasn't finished in time.
    const TimerQueue::TimerId timeout_id = timers.schedule_after(timeout, [c] {
        c->close(std::make_error_code(std::errc::timed_out));
    });

    c->set_on_connected([&timers, timeout_id, peer] {
        timers.cancel(timeout_id);
        std::printf("out: connected to %s\n", peer.c_str());
    });

    c->set_on_data([c](std::span<const std::byte> data) {
        std::printf("out: echo received: %s", to_text(data).c_str());
        c->close();  // test done
        return data.size();
    });

    c->set_on_want_write([&reactor, c](bool want) { reactor.modify_handler(c, want); });

    c->set_on_closed([&reactor, &timers, c, peer, timeout_id](std::error_code reason) {
        timers.cancel(timeout_id);  // never let the timeout fire on a dead connection
        if (reason) std::printf("out: %s closed: %s\n", peer.c_str(), reason.message().c_str());
        else        std::printf("out: %s closed normally\n", peer.c_str());
        reactor.unregister_handler(c);
    });

    reactor.register_handler(std::move(conn), c->wants_write());
}

}  // namespace

int main() {
    Reactor reactor;

    // ── Timers ──────────────────────────────────────────────────────────
    auto timer_queue = std::make_unique<TimerQueue>();
    TimerQueue* timers = timer_queue.get();
    reactor.register_handler(std::move(timer_queue));

    // ── UDP echo on port 9000 ───────────────────────────────────────────
    auto udp = std::make_unique<UDPHandler>(9000);
    UDPHandler* udp_ptr = udp.get();

    udp->set_on_datagram([udp_ptr](std::span<const std::byte> data, const sockaddr_in6& peer) {
        std::printf("udp: %zu bytes from %s\n", data.size(), to_string(peer).c_str());
        udp_ptr->send_to(data, peer);
    });

    reactor.register_handler(std::move(udp));

    // ── TCP echo on port 9001 ───────────────────────────────────────────
    auto listener = std::make_unique<TCPListener>(9001,
        [&reactor](UniqueFD client, const sockaddr_in6& peer) {
            const std::string name = to_string(peer);
            std::printf("tcp: accepted %s\n", name.c_str());

            auto conn = std::make_unique<TCPConnection>(std::move(client));
            TCPConnection* c = conn.get();

            c->set_on_data([c](std::span<const std::byte> data) {
                c->send(data);  // echo
                return data.size();
            });

            c->set_on_want_write([&reactor, c](bool want) { reactor.modify_handler(c, want); });

            c->set_on_closed([&reactor, c, name](std::error_code reason) {
                if (reason) std::printf("tcp: %s closed: %s\n", name.c_str(), reason.message().c_str());
                else        std::printf("tcp: %s closed normally\n", name.c_str());
                reactor.unregister_handler(c);
            });

            reactor.register_handler(std::move(conn), c->wants_write());
        });
    TCPListener* listener_ptr = listener.get();

    reactor.register_handler(std::move(listener));

    std::printf("UDP echo listening on port %u\n", udp_ptr->local_port());
    std::printf("TCP echo listening on port %u\n", listener_ptr->local_port());

    // ── Heartbeat demo: tick every second, stop after 3 ─────────────────
    TimerQueue::TimerId heartbeat = 0;
    heartbeat = timers->schedule_every(1s, [timers, &heartbeat, ticks = 0]() mutable {
        ++ticks;
        std::printf("timer: tick %d\n", ticks);
        if (ticks == 3) timers->cancel(heartbeat);  // a timer cancelling itself
    });

    // ── Outbound tests ──────────────────────────────────────────────────
    start_outbound_test(reactor, *timers, "::1", 9001, 3s);           // succeeds
    start_outbound_test(reactor, *timers, "::1", 9999, 3s);           // refused
    start_outbound_test(reactor, *timers, "10.255.255.1", 9001, 3s);  // no answer: times out

    reactor.run();
}