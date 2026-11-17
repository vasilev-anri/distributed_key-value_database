#include "handlers/tcp_connection.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <stdexcept>

#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include "utils/error_utils.h"

namespace {
constexpr std::size_t kReadChunk = 16 * 1024;
constexpr std::size_t kCompactThreshold = 64 * 1024;
std::error_code errno_code(int err) { return {err, std::system_category()}; }
}  // namespace

TCPConnection::TCPConnection(UniqueFD fd) : TCPConnection(std::move(fd), State::Open) {}

TCPConnection::TCPConnection(UniqueFD fd, State initial) : fd_(std::move(fd)), state_(initial) {
    if (!fd_) throw std::invalid_argument("TCPConnection: invalid socket");
    int one = 1;
    ::setsockopt(fd_.get(), IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
}

std::unique_ptr<TCPConnection> TCPConnection::connect_to(const sockaddr_in6& addr) {
    UniqueFD fd(::socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0));
    if (!fd) throw_errno("socket");
    int v6only = 0;
    ::setsockopt(fd.get(), IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof v6only);
    std::unique_ptr<TCPConnection> conn(new TCPConnection(std::move(fd), State::Connecting));
    if (::connect(conn->fd_.get(), reinterpret_cast<const sockaddr*>(&addr), sizeof addr) < 0 &&
        errno != EINPROGRESS) {
        throw_errno("connect");
    }
    conn->want_write_ = true;
    return conn;
}

void TCPConnection::handle_event(std::uint32_t events) {
    switch (state_) {
        case State::Closed: return;
        case State::Connecting: finish_connect(events); return;
        case State::Open: break;
    }
    if (events & EPOLLERR) { close_with(socket_error()); return; }
    // epoll reports the peer's FIN together with any data still unread, so
    // whoever handles that data can tell it came from a peer that has left.
    if (events & (EPOLLRDHUP | EPOLLHUP)) peer_finished_ = true;
    if (events & EPOLLOUT) flush();
    if (state_ == State::Open && (events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP))) on_readable();
}

void TCPConnection::finish_connect(std::uint32_t events) {
    if (!(events & (EPOLLOUT | EPOLLERR | EPOLLHUP))) return;
    if (const std::error_code err = socket_error()) { close_with(err); return; }
    state_ = State::Open;
    if (on_connected_) on_connected_();
    if (state_ != State::Open) return;
    flush();
}

void TCPConnection::on_readable() {
    std::array<std::byte, kReadChunk> chunk;
    while (state_ == State::Open && !read_closed_) {
        const ssize_t n = ::recv(fd_.get(), chunk.data(), chunk.size(), 0);
        if (n > 0) {
            in_buf_.insert(in_buf_.end(), chunk.begin(), chunk.begin() + n);
            deliver_input();
            if (state_ == State::Open && in_buf_.size() > kMaxInputBuffer)
                close_with(std::make_error_code(std::errc::message_size));
            continue;
        }
        if (n == 0) { read_closed_ = true; peer_finished_ = true; if (pending_output() == 0) close_with({}); return; }
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return;
        close_with(errno_code(errno));
    }
}

void TCPConnection::deliver_input() {
    if (!on_data_) { in_buf_.clear(); return; }
    const std::size_t consumed = on_data_(std::span<const std::byte>(in_buf_));
    if (state_ != State::Open) return;
    in_buf_.erase(in_buf_.begin(),
                  in_buf_.begin() + static_cast<std::ptrdiff_t>(std::min(consumed, in_buf_.size())));
}

bool TCPConnection::send(std::span<const std::byte> data) {
    if (state_ == State::Closed) return false;
    if (data.empty()) return true;
    if (pending_output() + data.size() > kMaxOutputBuffer) {
        close_with(std::make_error_code(std::errc::no_buffer_space));
        return false;
    }
    out_buf_.insert(out_buf_.end(), data.begin(), data.end());
    if (state_ == State::Open) flush();
    return state_ != State::Closed;
}

void TCPConnection::flush() {
    while (out_offset_ < out_buf_.size()) {
        const ssize_t n = ::send(fd_.get(), out_buf_.data() + out_offset_, out_buf_.size() - out_offset_, MSG_NOSIGNAL);
        if (n > 0) { out_offset_ += static_cast<std::size_t>(n); continue; }
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        close_with(n < 0 ? errno_code(errno) : std::make_error_code(std::errc::io_error));
        return;
    }
    if (pending_output() == 0) {
        out_buf_.clear(); out_offset_ = 0; set_want_write(false);
        if (read_closed_) close_with({});
        return;
    }
    if (out_offset_ >= kCompactThreshold && out_offset_ * 2 >= out_buf_.size()) {
        out_buf_.erase(out_buf_.begin(), out_buf_.begin() + static_cast<std::ptrdiff_t>(out_offset_));
        out_offset_ = 0;
    }
    set_want_write(true);
}

void TCPConnection::set_want_write(bool want) {
    if (want == want_write_) return;
    want_write_ = want;
    if (on_want_write_) on_want_write_(want);
}

void TCPConnection::close_with(std::error_code reason) {
    if (state_ == State::Closed) return;
    state_ = State::Closed;
    ::shutdown(fd_.get(), SHUT_RDWR);
    if (on_closed_) on_closed_(reason);
}

std::error_code TCPConnection::socket_error() const noexcept {
    int err = 0; socklen_t len = sizeof err;
    if (::getsockopt(fd_.get(), SOL_SOCKET, SO_ERROR, &err, &len) < 0) err = errno;
    return err ? errno_code(err) : std::error_code{};
}
