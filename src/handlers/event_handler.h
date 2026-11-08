#pragma once
#include <cstdint>

class EventHandler {
public:
    virtual ~EventHandler() = default;
    virtual void handle_event(uint32_t events) = 0;
    virtual int get_fd() = 0;
};
