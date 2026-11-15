#include "node/request_handler.h"

#include <span>
#include <utility>

#include "protocol/codec.h"
#include "storage/kv_store.h"

using namespace protocol;

namespace {

// A payload that fails to decode becomes a status the client understands.
Status to_status(DecodeError error) noexcept {
    switch (error) {
        case DecodeError::KeyTooLarge: return Status::KeyTooLarge;
        case DecodeError::ValueTooLarge: return Status::ValueTooLarge;
        default: return Status::InvalidRequest;
    }
}

// Every response copies request_id and sender_id from the request, so the
// client can match it to the request it sent.
RequestHandler::Response respond(const Header& request, Status status,
                                 std::span<const std::byte> payload = {}) {
    return {status, encode_message({.type = response_for(request.type),
                                    .status = status,
                                    .request_id = request.request_id,
                                    .sender_id = request.sender_id},
                                   payload)};
}

}  // namespace

std::optional<RequestHandler::Response> RequestHandler::handle(const Frame& request) {
    const Header& header = request.header;

    switch (header.type) {
        case MessageType::PutRequest: {
            auto put = decode_key_value(request.payload);
            if (!put) return respond(header, to_status(put.error()));

            store_.put(std::move(put->key), std::move(put->value));
            return respond(header, Status::Ok);
        }

        case MessageType::GetRequest: {
            const auto get = decode_key(request.payload);
            if (!get) return respond(header, to_status(get.error()));

            auto value = store_.get(get->key);
            if (!value) return respond(header, Status::NotFound);
            return respond(header, Status::Ok, encode_payload(ValuePayload{std::move(*value)}));
        }

        case MessageType::DeleteRequest: {
            const auto del = decode_key(request.payload);
            if (!del) return respond(header, to_status(del.error()));

            return respond(header, store_.remove(del->key) ? Status::Ok : Status::NotFound);
        }

        default:
            return std::nullopt;  // not a client request: ignore, don't answer
    }
}
