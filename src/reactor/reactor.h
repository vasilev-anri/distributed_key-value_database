#pragma once
#include <memory>

#include "handlers/event_handler.h"
#include "utils/unique_fd.h"


class Reactor {
public:
    Reactor();
    void register_handler(std::unique_ptr<EventHandler> handler, bool want_write = false);
    void unregister_handler(int fd);
    void modify_handler(int fd, bool want_write);
    void handle_events();
private:
    UniqueFD epfd_;
};
