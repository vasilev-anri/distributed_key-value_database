#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "protocol/protocol.h"

class KeyValueStore;

// The database logic of one node: applies a decoded client request to the
// store and builds the response. It knows nothing about sockets, so the same
// logic serves any transport (and can be tested without a network).
class RequestHandler {
public:
    struct Response {
        protocol::Status status;         // for logging
        std::vector<std::byte> message;  // complete encoded response, ready to send
    };

    explicit RequestHandler(KeyValueStore& store) noexcept : store_(store) {}

    // Handles one request. Returns std::nullopt if `request` is not a client
    // request (PUT / GET / DELETE): responses and control messages that arrive
    // on the data channel are ignored, not answered.
    [[nodiscard]] std::optional<Response> handle(const protocol::Frame& request);

private:
    KeyValueStore& store_;
};
