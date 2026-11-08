#pragma once

#include <cstdint>

// Rules for implementations:
//  1. get_fd() must return the same descriptor for as long as the handler
//     is registered with a reactor.
//  2. A handler must not close or replace its own descriptor while
//     registered — the descriptor is closed when the handler is destroyed.
class EventHandler {
public:
    virtual ~EventHandler() = default;

    // Edge-triggered: implementations must handle until EAGAIN.
    virtual void handle_event(std::uint32_t events) = 0;

    [[nodiscard]] virtual int get_fd() const noexcept = 0;
};