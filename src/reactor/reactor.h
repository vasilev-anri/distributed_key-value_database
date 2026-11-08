#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "handlers/event_handler.h"
#include "utils/unique_fd.h"

class Reactor {
public:
    Reactor();

    Reactor(const Reactor&) = delete;
    Reactor& operator=(const Reactor&) = delete;

    // Takes ownership. Returns a non-owning pointer that identifies the handler.
    EventHandler* register_handler(std::unique_ptr<EventHandler> handler,
                                   bool want_write = false);

    void modify_handler(EventHandler* handler, bool want_write);

    // Stops events immediately; destroys the handler after the current batch,
    // so it is safe to call from inside the handler's own callback.
    void unregister_handler(EventHandler* handler);

    void run();
    void stop() noexcept { running_ = false; }

private:
    UniqueFD epfd_;  // the reactor's own epoll instance
    bool running_ = false;
    std::unordered_map<EventHandler*, std::unique_ptr<EventHandler>> handlers_;
    std::vector<std::unique_ptr<EventHandler>> pending_destroy_;
};