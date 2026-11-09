#include "handlers/timer_queue.h"

#include <cerrno>

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "utils/error_utils.h"

namespace {

// steady_clock is CLOCK_MONOTONIC on Linux, so its time points can be given
// to the timerfd directly as absolute times.
timespec to_timespec(TimerQueue::Clock::time_point tp) {
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
    if (ns <= 0) ns = 1;  // an all-zero value would disarm the timer instead
    return {static_cast<time_t>(ns / 1'000'000'000), static_cast<long>(ns % 1'000'000'000)};
}

}  // namespace

TimerQueue::TimerQueue()
    : fd_(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)) {
    if (!fd_) throw_errno("timerfd_create");
}

TimerQueue::TimerId TimerQueue::schedule_after(Clock::duration delay, Callback cb) {
    return schedule(delay, Clock::duration::zero(), std::move(cb));
}

TimerQueue::TimerId TimerQueue::schedule_every(Clock::duration interval, Callback cb) {
    if (interval <= Clock::duration::zero()) interval = std::chrono::milliseconds(1);
    return schedule(interval, interval, std::move(cb));
}

TimerQueue::TimerId TimerQueue::schedule(Clock::duration delay, Clock::duration interval,
                                         Callback cb) {
    const TimerId id = next_id_++;
    const Clock::time_point deadline = Clock::now() + delay;

    entries_.emplace(id, Entry{std::move(cb), interval});
    heap_.push({deadline, id});

    // Re-arm only if this timer is now the earliest. During dispatch,
    // run_due() re-arms once at the end instead.
    if (!dispatching_ && deadline < armed_for_) arm();
    return id;
}

bool TimerQueue::cancel(TimerId id) noexcept {
    // Lazy: the heap entry stays and is skipped when it comes up.
    return entries_.erase(id) > 0;
}

void TimerQueue::handle_event(std::uint32_t events) {
    if (!(events & EPOLLIN)) return;

    // Reading the expiration counter resets the timerfd's readiness.
    std::uint64_t expirations = 0;
    while (::read(fd_.get(), &expirations, sizeof expirations) < 0 && errno == EINTR) {}

    run_due();
}

void TimerQueue::run_due() {
    dispatching_ = true;
    const Clock::time_point now = Clock::now();

    // 1. Collect everything due *now*. Timers scheduled by callbacks below go
    //    into the heap and run in a later round, so a callback that keeps
    //    scheduling zero-delay timers can't trap us in an endless loop.
    std::vector<TimerId> due;
    while (!heap_.empty() && heap_.top().deadline <= now) {
        const Due item = heap_.top();
        heap_.pop();

        auto it = entries_.find(item.id);
        if (it == entries_.end()) continue;  // cancelled

        if (it->second.interval > Clock::duration::zero()) {
            Clock::time_point next = item.deadline + it->second.interval;  // no drift
            if (next <= now) next = now + it->second.interval;            // fell behind: no burst
            heap_.push({next, item.id});
        }
        due.push_back(item.id);
    }

    // 2. Run them. The callback is moved out before it runs, so a timer that
    //    cancels itself doesn't destroy the function that's executing.
    for (const TimerId id : due) {
        auto it = entries_.find(id);
        if (it == entries_.end()) continue;  // cancelled by an earlier callback

        const bool repeating = it->second.interval > Clock::duration::zero();
        Callback cb = std::move(it->second.callback);
        if (!repeating) entries_.erase(it);

        cb();

        if (repeating) {  // give the callback back, unless it was cancelled meanwhile
            if (auto again = entries_.find(id); again != entries_.end())
                again->second.callback = std::move(cb);
        }
    }

    dispatching_ = false;
    arm();
}

void TimerQueue::arm() {
    // Drop cancelled timers sitting at the top so we don't wake up for them.
    while (!heap_.empty() && !entries_.contains(heap_.top().id)) heap_.pop();

    itimerspec spec{};  // all zero = disarmed
    if (heap_.empty()) {
        armed_for_ = Clock::time_point::max();
    } else {
        armed_for_ = heap_.top().deadline;
        spec.it_value = to_timespec(armed_for_);
    }

    if (::timerfd_settime(fd_.get(), TFD_TIMER_ABSTIME, &spec, nullptr) < 0)
        throw_errno("timerfd_settime");
}