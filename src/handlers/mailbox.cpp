#include "handlers/mailbox.h"

#include <cerrno>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "utils/error_utils.h"

Mailbox::Mailbox() : fd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)) {
    if (!fd_) throw_errno("eventfd");
}

void Mailbox::post(Task task) {
    bool was_empty = false;
    {
        std::lock_guard lock(mutex_);
        was_empty = queue_.empty();
        queue_.push_back(std::move(task));
    }
    // Only the first task needs to wake the reactor; later ones are picked
    // up in the same drain.
    if (was_empty) wake();
}

void Mailbox::wake() noexcept {
    const std::uint64_t one = 1;
    while (::write(fd_.get(), &one, sizeof one) < 0 && errno == EINTR) {}
}

void Mailbox::handle_event(std::uint32_t events) {
    if (!(events & EPOLLIN)) return;

    // Reset the eventfd counter first, then take the queue.
    std::uint64_t count = 0;
    while (::read(fd_.get(), &count, sizeof count) < 0 && errno == EINTR) {}

    std::vector<Task> tasks;
    {
        std::lock_guard lock(mutex_);
        tasks.swap(queue_);
    }

    // Run outside the lock: tasks may post again without deadlocking.
    for (auto& task : tasks) task();
}