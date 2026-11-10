#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

#include "handlers/event_handler.h"
#include "utils/unique_fd.h"

// Lets other threads hand work to the reactor thread. post() is thread-safe;
// the tasks run on the reactor thread, in the order they were posted.
class Mailbox : public EventHandler {
public:
    using Task = std::move_only_function<void()>;

    Mailbox();

    // Thread-safe. Must not be called after the Mailbox is destroyed:
    // stop other threads before the reactor shuts down.
    void post(Task task);

    void handle_event(std::uint32_t events) override;
    [[nodiscard]] int get_fd() const noexcept override { return fd_.get(); }

private:
    void wake() noexcept;

    UniqueFD fd_;
    std::mutex mutex_;
    std::vector<Task> queue_;  // guarded by mutex_
};