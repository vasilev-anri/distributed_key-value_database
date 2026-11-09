#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

#include "handlers/event_handler.h"
#include "utils/unique_fd.h"

// Many timers on a single timerfd: the timerfd is always armed for the earliest
// deadline, and every timer that is due runs when it fires.
class TimerQueue : public EventHandler {
public:
    using Clock = std::chrono::steady_clock;
    using Callback = std::move_only_function<void()>;
    using TimerId = std::uint64_t;  // 0 is never a valid id

    TimerQueue();

    // Runs `cb` once after `delay`.
    TimerId schedule_after(Clock::duration delay, Callback cb);

    // Runs `cb` every `interval`; the first run is after one interval.
    TimerId schedule_every(Clock::duration interval, Callback cb);

    // Cancels a timer. Safe to call with an id that already fired or was
    // cancelled, and from inside any timer callback (including its own).
    bool cancel(TimerId id) noexcept;

    [[nodiscard]] std::size_t active() const noexcept { return entries_.size(); }

    void handle_event(std::uint32_t events) override;
    [[nodiscard]] int get_fd() const noexcept override { return fd_.get(); }

private:
    struct Entry {
        Callback callback;
        Clock::duration interval;  // zero = one-shot
    };

    struct Due {
        Clock::time_point deadline;
        TimerId id;
        bool operator>(const Due& other) const noexcept {
            return deadline != other.deadline ? deadline > other.deadline : id > other.id;
        }
    };

    TimerId schedule(Clock::duration delay, Clock::duration interval, Callback cb);
    void run_due();
    void arm();

    UniqueFD fd_;
    TimerId next_id_ = 1;
    bool dispatching_ = false;
    Clock::time_point armed_for_ = Clock::time_point::max();

    std::unordered_map<TimerId, Entry> entries_;  // live timers
    std::priority_queue<Due, std::vector<Due>, std::greater<>> heap_;  // earliest first
};