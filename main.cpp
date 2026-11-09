#include <cstddef>
#include <cstdio>
#include <memory>
#include <span>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "handlers/tcp_connection.h"
#include "handlers/tcp_listener.h"
#include "handlers/udp_handler.h"
#include "reactor/reactor.h"

int main() {
    Reactor reactor;

    // ── UDP echo on port 9000 ───────────────────────────────────────────
    auto udp = std::make_unique<UDPHandler>(9000);
    UDPHandler* udp_ptr = udp.get();  // non-owning; the reactor will own it

    udp->set_on_datagram([udp_ptr](std::span<const std::byte> data, const sockaddr_in6& peer) {
        std::printf("udp: received %zu bytes\n", data.size());
        udp_ptr->send_to(data, peer);  // echo back
    });

    reactor.register_handler(std::move(udp));

    // ── TCP echo on port 9001 ───────────────────────────────────────────
    auto listener = std::make_unique<TCPListener>(9001,
        [&reactor](UniqueFD client, const sockaddr_in6& peer) {
            char addr[INET6_ADDRSTRLEN];
            ::inet_ntop(AF_INET6, &peer.sin6_addr, addr, sizeof addr);
            std::printf("tcp: accepted [%s]:%u\n", addr, ntohs(peer.sin6_port));

            auto conn = std::make_unique<TCPConnection>(std::move(client));
            TCPConnection* c = conn.get();  // non-owning; the reactor will own it

            c->set_on_data([c](std::span<const std::byte> data) {
                c->send(data);       // echo back
                return data.size();  // consumed everything
            });

            c->set_on_want_write([&reactor, c](bool want) {
                reactor.modify_handler(c, want);
            });

            c->set_on_closed([&reactor, c] {
                std::puts("tcp: connection closed");
                reactor.unregister_handler(c);
            });

            reactor.register_handler(std::move(conn), c->wants_write());
        });
    TCPListener* listener_ptr = listener.get();

    reactor.register_handler(std::move(listener));

    std::printf("UDP echo listening on port %u\n", udp_ptr->local_port());
    std::printf("TCP echo listening on port %u\n", listener_ptr->local_port());
    reactor.run();
}