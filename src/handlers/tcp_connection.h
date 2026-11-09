#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

#include "handlers/event_handler.h"
#include "utils/unique_fd.h"

class TCPConnection : public EventHandler {
public:
    // Receives all buffered input that hasn't been consumed yet. Must return how
    // many bytes it consumed (complete messages); the rest is kept and passed
    // again, together with newer data, on the next call.
    using DataCallback = std::move_only_function<std::size_t(std::span<const std::byte> data)>;

    // Asks the owner to enable/disable writable events for this connection.
    using WantWriteCallback = std::move_only_function<void(bool want_write)>;

    // Called exactly once when the connection is closed (by either side).
    using ClosedCallback = std::move_only_function<void()>;

    static constexpr std::size_t kMaxInputBuffer = 1 << 20;   // 1 MiB unconsumed input
    static constexpr std::size_t kMaxOutputBuffer = 8 << 20;  // 8 MiB unsent output

    // Takes ownership of an already-connected, non-blocking socket.
    explicit TCPConnection(UniqueFD fd);

    void set_on_data(DataCallback cb) noexcept { on_data_ = std::move(cb); }
    void set_on_want_write(WantWriteCallback cb) noexcept { on_want_write_ = std::move(cb); }
    void set_on_closed(ClosedCallback cb) noexcept { on_closed_ = std::move(cb); }

    // Queues data and tries to write it immediately. Returns false if the
    // connection is closed, or if the output limit is exceeded (then it closes).
    bool send(std::span<const std::byte> data);

    // Closes the connection. Pending output is discarded; on_closed is called once.
    void close();

    [[nodiscard]] bool is_open() const noexcept { return open_; }
    [[nodiscard]] bool wants_write() const noexcept { return want_write_; }

    void handle_event(std::uint32_t events) override;
    [[nodiscard]] int get_fd() const noexcept override { return fd_.get(); }

private:
    void on_readable();
    void deliver_input();
    void flush();
    void set_want_write(bool want);
    void report_socket_error() noexcept;

    [[nodiscard]] std::size_t pending_output() const noexcept {
        return out_buf_.size() - out_offset_;
    }

    UniqueFD fd_;
    bool open_ = true;
    bool read_closed_ = false;  // peer finished sending; close once output is flushed
    bool want_write_ = false;

    std::vector<std::byte> in_buf_;
    std::vector<std::byte> out_buf_;
    std::size_t out_offset_ = 0;

    DataCallback on_data_;
    WantWriteCallback on_want_write_;
    ClosedCallback on_closed_;
};