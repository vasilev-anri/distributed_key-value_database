// TCPConnection on a socketpair: real sockets and real recv/send, but no
// network and no reactor. handle_event() is called by hand, like epoll would.

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "handlers/tcp_connection.h"
#include "utils/unique_fd.h"

namespace {

struct Fixture {
    UniqueFD other;                        // the "peer" end, used directly
    std::unique_ptr<TCPConnection> conn;   // our end
    std::string received;
    std::optional<std::error_code> closed;
    bool finished_while_delivering = false;

    Fixture() {
        int fds[2];
        if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fds) != 0) throw std::runtime_error("socketpair");
        other = UniqueFD(fds[1]);
        conn = std::make_unique<TCPConnection>(UniqueFD(fds[0]));
        conn->set_on_data([this](std::span<const std::byte> data) {
            received.append(reinterpret_cast<const char*>(data.data()), data.size());
            finished_while_delivering = conn->peer_finished();
            return data.size();
        });
        conn->set_on_closed([this](std::error_code r) { closed = r; });
    }

    void peer_sends(const std::string& s) { ASSERT_EQ(::write(other.get(), s.data(), s.size()), static_cast<ssize_t>(s.size())); }
};

}  // namespace

TEST(TCPConnection, DeliversData) {
    Fixture f;
    f.peer_sends("hello");
    f.conn->handle_event(EPOLLIN);
    EXPECT_EQ(f.received, "hello");
    EXPECT_FALSE(f.conn->peer_finished());
    EXPECT_FALSE(f.closed);
}

TEST(TCPConnection, DataFollowedByFinIsDeliveredThenCloses) {
    Fixture f;
    f.peer_sends("hello");
    ::shutdown(f.other.get(), SHUT_WR);  // the peer leaves right after sending
    f.conn->handle_event(EPOLLIN | EPOLLRDHUP);

    EXPECT_EQ(f.received, "hello");
    EXPECT_TRUE(f.finished_while_delivering);  // known already while handling "hello"
    EXPECT_TRUE(f.conn->peer_finished());
    ASSERT_TRUE(f.closed);
    EXPECT_FALSE(*f.closed);  // a normal close, not an error
}

TEST(TCPConnection, FinWithoutRdhupFlagIsStillNoticed) {
    Fixture f;
    ::shutdown(f.other.get(), SHUT_WR);
    f.conn->handle_event(EPOLLIN);  // recv() returning 0 is enough
    EXPECT_TRUE(f.conn->peer_finished());
    EXPECT_TRUE(f.closed);
}
