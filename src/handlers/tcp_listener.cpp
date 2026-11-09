#include "handlers/tcp_listener.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <sys/epoll.h>
#include <sys/socket.h>

#include "utils/error_utils.h"

namespace {

void set_int_option(int fd, int level, int name, int value, const char* what) {
    if (::setsockopt(fd, level, name, &value, sizeof value) < 0) throw_errno(what);
}

}  // namespace

TCPListener::TCPListener(std::uint16_t port, AcceptCallback on_accept)
    : fd_(::socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)),
      on_accept_(std::move(on_accept)) {
    if (!fd_) throw_errno("socket");

    // Allow immediate restart while old connections sit in TIME_WAIT.
    set_int_option(fd_.get(), SOL_SOCKET, SO_REUSEADDR, 1, "setsockopt(SO_REUSEADDR)");
    // Dual-stack: IPv4 clients appear as ::ffff:a.b.c.d
    set_int_option(fd_.get(), IPPROTO_IPV6, IPV6_V6ONLY, 0, "setsockopt(IPV6_V6ONLY)");

    sockaddr_in6 addr{};
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    addr.sin6_addr = in6addr_any;

    if (::bind(fd_.get(), reinterpret_cast<const sockaddr*>(&addr), sizeof addr) < 0)
        throw_errno("bind");
    if (::listen(fd_.get(), SOMAXCONN) < 0)
        throw_errno("listen");
}

std::uint16_t TCPListener::local_port() const {
    sockaddr_in6 addr{};
    socklen_t len = sizeof addr;
    if (::getsockname(fd_.get(), reinterpret_cast<sockaddr*>(&addr), &len) < 0)
        throw_errno("getsockname");
    return ntohs(addr.sin6_port);
}

void TCPListener::handle_event(std::uint32_t events) {
    if (events & EPOLLERR) on_error();
    if (events & EPOLLIN) accept_all();
}

void TCPListener::accept_all() {
    for (;;) {
        sockaddr_in6 peer{};
        socklen_t peer_len = sizeof peer;

        UniqueFD client(::accept4(fd_.get(), reinterpret_cast<sockaddr*>(&peer), &peer_len,
                                  SOCK_NONBLOCK | SOCK_CLOEXEC));
        if (!client) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;   // backlog drained
            if (errno == EINTR || errno == ECONNABORTED) continue;  // try the next one
            std::perror("accept4");  // e.g. EMFILE: out of file descriptors
            return;
        }

        if (on_accept_) on_accept_(std::move(client), peer);
        // No callback, or the callback didn't take it: the connection is closed here.
    }
}

void TCPListener::on_error() noexcept {
    int err = 0;
    socklen_t len = sizeof err;
    if (::getsockopt(fd_.get(), SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err != 0)
        std::fprintf(stderr, "tcp listener error: %s\n", std::strerror(err));
}