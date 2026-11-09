#include "handlers/udp_handler.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <sys/epoll.h>
#include <sys/socket.h>

#include "utils/error_utils.h"

UDPHandler::UDPHandler(std::uint16_t port, DatagramCallback on_datagram)
    : fd_(::socket(AF_INET6, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)),
      on_datagram_(std::move(on_datagram)) {
    if (!fd_) throw_errno("socket");

    int v6only = 0;  // dual-stack: IPv4 clients appear as ::ffff:a.b.c.d
    if (::setsockopt(fd_.get(), IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof v6only) < 0)
        throw_errno("setsockopt(IPV6_V6ONLY)");

    sockaddr_in6 addr{};
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    addr.sin6_addr = in6addr_any;

    if (::bind(fd_.get(), reinterpret_cast<const sockaddr*>(&addr), sizeof addr) < 0)
        throw_errno("bind");
}

std::uint16_t UDPHandler::local_port() const {
    sockaddr_in6 addr{};
    socklen_t len = sizeof addr;
    if (::getsockname(fd_.get(), reinterpret_cast<sockaddr*>(&addr), &len) < 0)
        throw_errno("getsockname");
    return ntohs(addr.sin6_port);
}

void UDPHandler::handle_event(std::uint32_t events) {
    if (events & EPOLLERR) on_error();
    if (events & EPOLLIN) on_readable();
}

void UDPHandler::on_readable() {
    for (;;) {
        sockaddr_in6 peer{};
        socklen_t peer_len = sizeof peer;

        const ssize_t n = ::recvfrom(fd_.get(), buf_.data(), buf_.size(), 0,
                                     reinterpret_cast<sockaddr*>(&peer), &peer_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;  // fully drained
            if (errno == EINTR) continue;
            std::perror("recvfrom");
            return;
        }

        if (on_datagram_) {
            on_datagram_(std::span<const std::byte>(buf_.data(), static_cast<std::size_t>(n)),
                         peer);
        }
    }
}

bool UDPHandler::send_to(std::span<const std::byte> data, const sockaddr_in6& peer) noexcept {
    for (;;) {
        const ssize_t n = ::sendto(fd_.get(), data.data(), data.size(), 0,
                                   reinterpret_cast<const sockaddr*>(&peer), sizeof peer);
        if (n >= 0) return true;
        if (errno == EINTR) continue;

        // EAGAIN = send buffer full. Either way the datagram is dropped,
        // which is acceptable for UDP: the client's retry covers it.
        if (errno != EAGAIN && errno != EWOULDBLOCK) std::perror("sendto");
        return false;
    }
}

void UDPHandler::on_error() noexcept {
    // Reading SO_ERROR clears the pending error so it doesn't fire again.
    int err = 0;
    socklen_t len = sizeof err;
    if (::getsockopt(fd_.get(), SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err != 0)
        std::fprintf(stderr, "udp socket error: %s\n", std::strerror(err));
}