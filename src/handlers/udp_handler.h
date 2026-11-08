#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

#include <netinet/in.h>

#include "handlers/event_handler.h"
#include "utils/unique_fd.h"

class UDPHandler : public EventHandler {
public:
    // Called once per received datagram. `data` points into an internal buffer
    // and is only valid during the call — copy it if you need it later.
    using DatagramCallback =
        std::move_only_function<void(std::span<const std::byte> data, const sockaddr_in6& peer)>;

    // Binds to [::]:port, dual-stack. Port 0 lets the OS choose (see local_port()).
    // Throws std::system_error if the socket cannot be created or bound.
    explicit UDPHandler(std::uint16_t port, DatagramCallback on_datagram = nullptr);

    void set_on_datagram(DatagramCallback cb) noexcept { on_datagram_ = std::move(cb); }

    // Sends one datagram. Returns false if it could not be sent (it is dropped).
    bool send_to(std::span<const std::byte> data, const sockaddr_in6& peer) noexcept;

    [[nodiscard]] std::uint16_t local_port() const;

    void handle_event(std::uint32_t events) override;
    [[nodiscard]] int get_fd() const noexcept override { return fd_.get(); }

private:
    void on_readable();
    void on_error() noexcept;

    UniqueFD fd_;
    DatagramCallback on_datagram_;
    std::array<std::byte, 65536> buf_{};  // fits the largest possible UDP datagram
};