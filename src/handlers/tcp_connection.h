#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <system_error>
#include <vector>

#include <netinet/in.h>

#include "handlers/event_handler.h"
#include "utils/unique_fd.h"

class TCPConnection : public EventHandler {
public:
    enum class State { Connecting, Open, Closed };

    // Receives all buffered input that hasn't been consumed yet. Must return how
    // many bytes it consumed (complete messages); the rest is kept and passed
    // again, together with newer data, on the next call.
    using DataCallback = std::move_only_function<std::size_t(std::span<const std::byte> data)>;

    // Asks the owner to enable/disable writable events for this connection.
    using WantWriteCallback = std::move_only_function<void(bool want_write)>;

    // Outbound only: called once when the connection is established.
    using ConnectedCallback = std::move_only_function<void()>;

    // Called exactly once when the connection ends. An empty error code means a
    // normal close; otherwise it says why (refused, reset, limit exceeded, ...).
    // A failed connect is reported here too, without on_connected ever firing.
    using ClosedCallback = std::move_only_function<void(std::error_code reason)>;

    static constexpr std::size_t kMaxInputBuffer = 1 << 20;   // 1 MiB unconsumed input
    static constexpr std::size_t kMaxOutputBuffer = 8 << 20;  // 8 MiB unsent output

    // Inbound: takes ownership of an already-connected, non-blocking socket.
    explicit TCPConnection(UniqueFD fd);

    // Outbound: starts a non-blocking connect. The connection begins in the
    // Connecting state and already wants writable events — register it with
    // `wants_write()`. Throws std::system_error if the connect fails immediately.
    static std::unique_ptr<TCPConnection> connect_to(const sockaddr_in6& addr);

    void set_on_data(DataCallback cb) noexcept { on_data_ = std::move(cb); }
    void set_on_want_write(WantWriteCallback cb) noexcept { on_want_write_ = std::move(cb); }
    void set_on_connected(ConnectedCallback cb) noexcept { on_connected_ = std::move(cb); }
    void set_on_closed(ClosedCallback cb) noexcept { on_closed_ = std::move(cb); }

    // Queues data. Written immediately if open; if still connecting, it's sent
    // as soon as the connection is up. Returns false if the connection is closed
    // or the output limit is exceeded (then it closes).
    bool send(std::span<const std::byte> data);

    // Closes the connection normally. Pending output is discarded.
    void close() { close_with({}); }

    [[nodiscard]] State state() const noexcept { return state_; }
    [[nodiscard]] bool is_open() const noexcept { return state_ == State::Open; }
    [[nodiscard]] bool wants_write() const noexcept { return want_write_; }

    void handle_event(std::uint32_t events) override;
    [[nodiscard]] int get_fd() const noexcept override { return fd_.get(); }

private:
    TCPConnection(UniqueFD fd, State initial);

    void finish_connect(std::uint32_t events);
    void on_readable();
    void deliver_input();
    void flush();
    void set_want_write(bool want);
    void close_with(std::error_code reason);
    [[nodiscard]] std::error_code socket_error() const noexcept;

    [[nodiscard]] std::size_t pending_output() const noexcept {
        return out_buf_.size() - out_offset_;
    }

    UniqueFD fd_;
    State state_;
    bool read_closed_ = false;  // peer finished sending; close once output is flushed
    bool want_write_ = false;

    std::vector<std::byte> in_buf_;
    std::vector<std::byte> out_buf_;
    std::size_t out_offset_ = 0;

    DataCallback on_data_;
    WantWriteCallback on_want_write_;
    ConnectedCallback on_connected_;
    ClosedCallback on_closed_;
};