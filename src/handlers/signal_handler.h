#pragma once

#include <cstdint>
#include <functional>
#include <initializer_list>

#include "handlers/event_handler.h"
#include "utils/unique_fd.h"

// Delivers signals (e.g. SIGINT, SIGTERM) as reactor events instead of
// interrupting the program at an arbitrary point.
//
// IMPORTANT: create it in main() BEFORE starting any other thread. It blocks
// the signals for the calling thread; threads started later inherit that, so
// the signals can only arrive through this handler.
class SignalHandler : public EventHandler {
public:
    using Callback = std::move_only_function<void(int signo)>;

    SignalHandler(std::initializer_list<int> signals, Callback on_signal);

    void handle_event(std::uint32_t events) override;
    [[nodiscard]] int get_fd() const noexcept override { return fd_.get(); }

private:
    UniqueFD fd_;
    Callback on_signal_;
};