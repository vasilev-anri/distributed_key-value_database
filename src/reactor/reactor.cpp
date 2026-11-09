#include "reactor/reactor.h"

#include <array>
#include <cerrno>
#include <stdexcept>

#include <sys/epoll.h>

#include "utils/error_utils.h"

namespace {

epoll_event make_epoll_event(EventHandler* handler, bool want_write) noexcept {
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    if (want_write) ev.events |= EPOLLOUT;
    ev.data.ptr = handler;  // epoll hands this pointer back with every event
    return ev;
}

}  // namespace

Reactor::Reactor() : epfd_(::epoll_create1(EPOLL_CLOEXEC)) {
    if (!epfd_) throw_errno(errno, "epoll_create1");
}

EventHandler* Reactor::register_handler(std::unique_ptr<EventHandler> handler,
                                        bool want_write) {
    if (!handler) throw std::invalid_argument("register_handler: null handler");

    EventHandler* raw = handler.get();
    auto [it, inserted] = handlers_.emplace(raw, std::move(handler));

    epoll_event ev = make_epoll_event(raw, want_write);
    if (::epoll_ctl(epfd_.get(), EPOLL_CTL_ADD, raw->get_fd(), &ev) < 0) {
        const int err = errno;  // save before erase(): destroying the handler closes its fd
        handlers_.erase(it);
        throw_errno(err, "epoll_ctl(ADD)");
    }
    return raw;
}

void Reactor::modify_handler(EventHandler* handler, bool want_write) {
    if (!handlers_.contains(handler)) return;

    epoll_event ev = make_epoll_event(handler, want_write);
    if (::epoll_ctl(epfd_.get(), EPOLL_CTL_MOD, handler->get_fd(), &ev) < 0)
        throw_errno(errno, "epoll_ctl(MOD)");
}

void Reactor::unregister_handler(EventHandler* handler) {
    auto it = handlers_.find(handler);
    if (it == handlers_.end()) return;

    ::epoll_ctl(epfd_.get(), EPOLL_CTL_DEL, handler->get_fd(), nullptr);
    pending_destroy_.push_back(std::move(it->second));  // alive until the batch ends
    handlers_.erase(it);
}

void Reactor::run() {
    running_ = true;
    std::array<epoll_event, 64> events{};

    while (running_) {
        const int n = ::epoll_wait(epfd_.get(), events.data(),
                                   static_cast<int>(events.size()), -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            throw_errno(errno, "epoll_wait");
        }

        for (int i = 0; i < n; ++i) {
            auto* handler = static_cast<EventHandler*>(events[i].data.ptr);
            if (!handlers_.contains(handler)) continue;  // unregistered earlier in this batch
            handler->handle_event(events[i].events);
        }

        pending_destroy_.clear();  // removed handlers die here, closing their fds
    }
}