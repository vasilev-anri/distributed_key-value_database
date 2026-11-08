#include <cstdio>
#include <memory>

#include "reactor/reactor.h"
#include "handlers/udp_handler.h"

int main() {
    Reactor reactor;

    auto udp = std::make_unique<UDPHandler>(9000);
    UDPHandler* udp_ptr = udp.get();  // non-owning; the reactor will own it

    udp->set_on_datagram([udp_ptr](std::span<const std::byte> data, const sockaddr_in6& peer) {
        std::printf("received %zu bytes\n", data.size());
        udp_ptr->send_to(data, peer);  // echo back
    });

    reactor.register_handler(std::move(udp));

    std::printf("UDP echo listening on port %u\n", udp_ptr->local_port());
    reactor.run();
}