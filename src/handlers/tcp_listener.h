#pragma once

#include <cstdint>
#include <functional>

#include <netinet/in.h>

#include "handlers/event_handler.h"
#include "utils/unique_fd.h"

class TCPListener : public EventHandler {
public:
    // Called once per accepted connection. The socket is already non-blocking.
    // To reject a connection, just let the UniqueFD go out of scope (closes it).
    using AcceptCallback =
        std::move_only_function<void(UniqueFD client, const sockaddr_in6& peer)>;

    // Listens on [::]:port, dual-stack. Port 0 lets the OS choose (see local_port()).
    // Throws std::system_error on failure.
    explicit TCPListener(std::uint16_t port, AcceptCallback on_accept = nullptr);

    void set_on_accept(AcceptCallback cb) noexcept { on_accept_ = std::move(cb); }

    [[nodiscard]] std::uint16_t local_port() const;

    void handle_event(std::uint32_t events) override;
    [[nodiscard]] int get_fd() const noexcept override { return fd_.get(); }

private:
    void accept_all();
    void on_error() noexcept;

    UniqueFD fd_;
    AcceptCallback on_accept_;
};