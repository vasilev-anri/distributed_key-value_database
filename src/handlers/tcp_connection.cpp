#include "handlers/tcp_connection.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <stdexcept>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/socket.h>

namespace {

constexpr std::size_t kReadChunk = 16 * 1024;
constexpr std::size_t kCompactThreshold = 64 * 1024;

}  // namespace

TCPConnection::TCPConnection(UniqueFD fd) : fd_(std::move(fd)) {
    if (!fd_) throw std::invalid_argument("TCPConnection: invalid socket");

    // Send small messages (heartbeats, acks) immediately instead of batching them.
    // Failure only affects latency, so it's not fatal.
    int one = 1;
    ::setsockopt(fd_.get(), IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
}

void TCPConnection::handle_event(std::uint32_t events) {
    if (!open_) return;

    if (events & EPOLLERR) {
        report_socket_error();
        close();
        return;
    }

    if (events & EPOLLOUT) flush();

    // Peer hang-ups are handled by reading: recv() returns any remaining
    // data first, then 0 — so the last messages are never lost.
    if (open_ && (events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP))) on_readable();
}

void TCPConnection::on_readable() {
    std::array<std::byte, kReadChunk> chunk;

    while (open_ && !read_closed_) {
        const ssize_t n = ::recv(fd_.get(), chunk.data(), chunk.size(), 0);

        if (n > 0) {
            in_buf_.insert(in_buf_.end(), chunk.begin(), chunk.begin() + n);
            deliver_input();
            if (open_ && in_buf_.size() > kMaxInputBuffer) {
                std::fprintf(stderr, "tcp: input limit exceeded, closing connection\n");
                close();
            }
            continue;
        }

        if (n == 0) {  // peer finished sending
            read_closed_ = true;
            if (pending_output() == 0) close();  // nothing left to send
            return;                               // otherwise flush() closes when done
        }

        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return;  // drained

        std::fprintf(stderr, "tcp recv: %s\n", std::strerror(errno));
        close();
    }
}

void TCPConnection::deliver_input() {
    if (!on_data_) {  // nobody listening: discard
        in_buf_.clear();
        return;
    }

    const std::size_t consumed = on_data_(std::span<const std::byte>(in_buf_));
    if (!open_) return;  // the callback closed the connection

    in_buf_.erase(in_buf_.begin(),
                  in_buf_.begin() + static_cast<std::ptrdiff_t>(std::min(consumed, in_buf_.size())));
}

bool TCPConnection::send(std::span<const std::byte> data) {
    if (!open_) return false;
    if (data.empty()) return true;

    if (pending_output() + data.size() > kMaxOutputBuffer) {
        std::fprintf(stderr, "tcp: output limit exceeded (peer too slow), closing connection\n");
        close();
        return false;
    }

    out_buf_.insert(out_buf_.end(), data.begin(), data.end());
    flush();
    return open_;
}

void TCPConnection::flush() {
    while (out_offset_ < out_buf_.size()) {
        const ssize_t n = ::send(fd_.get(), out_buf_.data() + out_offset_,
                                 out_buf_.size() - out_offset_, MSG_NOSIGNAL);
        if (n > 0) {
            out_offset_ += static_cast<std::size_t>(n);
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;  // kernel buffer full

        std::fprintf(stderr, "tcp send: %s\n", n < 0 ? std::strerror(errno) : "no progress");
        close();
        return;
    }

    if (pending_output() == 0) {
        out_buf_.clear();
        out_offset_ = 0;
        set_want_write(false);
        if (read_closed_) close();  // half-closed peer: everything sent, done
        return;
    }

    // Partially sent. Drop the already-sent prefix once it's large, so the
    // buffer doesn't grow forever under steady traffic.
    if (out_offset_ >= kCompactThreshold && out_offset_ * 2 >= out_buf_.size()) {
        out_buf_.erase(out_buf_.begin(), out_buf_.begin() + static_cast<std::ptrdiff_t>(out_offset_));
        out_offset_ = 0;
    }
    set_want_write(true);
}

void TCPConnection::set_want_write(bool want) {
    if (want == want_write_) return;  // only notify on real changes
    want_write_ = want;
    if (on_want_write_) on_want_write_(want);
}

void TCPConnection::close() {
    if (!open_) return;
    open_ = false;

    // Tell the peer right away. The descriptor itself is closed when the
    // handler is destroyed (EventHandler rule 2).
    ::shutdown(fd_.get(), SHUT_RDWR);

    if (on_closed_) on_closed_();
}

void TCPConnection::report_socket_error() noexcept {
    int err = 0;
    socklen_t len = sizeof err;
    if (::getsockopt(fd_.get(), SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err != 0)
        std::fprintf(stderr, "tcp socket error: %s\n", std::strerror(err));
}