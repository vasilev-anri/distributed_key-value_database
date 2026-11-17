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

    using DataCallback = std::move_only_function<std::size_t(std::span<const std::byte> data)>;
    using WantWriteCallback = std::move_only_function<void(bool want_write)>;
    using ConnectedCallback = std::move_only_function<void()>;
    using ClosedCallback = std::move_only_function<void(std::error_code reason)>;

    static constexpr std::size_t kMaxInputBuffer = 1 << 20;
    static constexpr std::size_t kMaxOutputBuffer = 8 << 20;

    explicit TCPConnection(UniqueFD fd);
    static std::unique_ptr<TCPConnection> connect_to(const sockaddr_in6& addr);

    void set_on_data(DataCallback cb) noexcept { on_data_ = std::move(cb); }
    void set_on_want_write(WantWriteCallback cb) noexcept { on_want_write_ = std::move(cb); }
    void set_on_connected(ConnectedCallback cb) noexcept { on_connected_ = std::move(cb); }
    void set_on_closed(ClosedCallback cb) noexcept { on_closed_ = std::move(cb); }

    bool send(std::span<const std::byte> data);
    void close(std::error_code reason = {}) { close_with(reason); }

    [[nodiscard]] State state() const noexcept { return state_; }
    [[nodiscard]] bool is_open() const noexcept { return state_ == State::Open; }
    [[nodiscard]] bool wants_write() const noexcept { return want_write_; }
    // The peer has closed its sending side (FIN received): no more data will
    // come. Already true while the data it sent before the FIN is delivered.
    [[nodiscard]] bool peer_finished() const noexcept { return peer_finished_; }

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
    [[nodiscard]] std::size_t pending_output() const noexcept { return out_buf_.size() - out_offset_; }

    UniqueFD fd_;
    State state_;
    bool read_closed_ = false;
    bool peer_finished_ = false;
    bool want_write_ = false;
    std::vector<std::byte> in_buf_;
    std::vector<std::byte> out_buf_;
    std::size_t out_offset_ = 0;
    DataCallback on_data_;
    WantWriteCallback on_want_write_;
    ConnectedCallback on_connected_;
    ClosedCallback on_closed_;
};
