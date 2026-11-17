// kvcli — command-line client for k_v_db.
//
// Usage:
//   kvcli <host> <port> put <key> <value>
//   kvcli <host> <port> get <key>
//   kvcli <host> <port> delete <key>
//   kvcli <host> <port> status          how this node sees the cluster
//   kvcli <host> <port> locate <key>    which nodes hold the key
//
// Exit codes: 0 = OK, 1 = the node answered with an error status (e.g. NOT_FOUND),
//             2 = usage error or no answer from the node.

#include <array>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "protocol/codec.h"
#include "utils/address.h"
#include "utils/unique_fd.h"

using namespace protocol;

namespace {

constexpr int kAttempts = 3;
constexpr int kTimeoutMs = 500;

int usage(const char* prog) {
    std::fprintf(stderr,
                 "usage:\n"
                 "  %s <host> <port> put <key> <value>\n"
                 "  %s <host> <port> get <key>\n"
                 "  %s <host> <port> delete <key>\n"
                 "  %s <host> <port> status\n"
                 "  %s <host> <port> locate <key>\n",
                 prog, prog, prog, prog, prog);
    return 2;
}

std::optional<std::uint16_t> parse_port(std::string_view text) {
    std::uint16_t port = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), port);
    if (ec != std::errc{} || end != text.data() + text.size()) return std::nullopt;
    return port;
}

void print(std::string_view text) {
    std::fwrite(text.data(), 1, text.size(), stdout);  // values may contain any bytes
    std::fputc('\n', stdout);
}

std::string hex_id(std::uint64_t id) {
    char buf[17];
    std::snprintf(buf, sizeof buf, "%016llx", static_cast<unsigned long long>(id));
    return buf;
}

void print_node(const char* role, const NodeStatus& n) {
    std::printf("  %-8s %s  %-24s %s\n", role, hex_id(n.id).c_str(), n.address.c_str(),
                std::string(to_string(n.state)).c_str());
}

int malformed() {
    std::fprintf(stderr, "error: malformed response from node\n");
    return 2;
}

// Prints the node's answer and returns the exit code.
int print_result(MessageType request_type, const Frame& response) {
    if (response.header.status != Status::Ok) {
        print(to_string(response.header.status));
        return 1;
    }
    if (request_type == MessageType::StatusRequest) {
        const auto st = decode_status(response.payload);
        if (!st) return malformed();
        std::printf("node %s: %u keys stored, replication factor %u\n", hex_id(st->node_id).c_str(),
                    st->key_count, static_cast<unsigned>(st->replication_factor));
        for (const NodeStatus& n : st->nodes) print_node("", n);
        return 0;
    }
    if (request_type == MessageType::LocateRequest) {
        const auto loc = decode_locate(response.payload);
        if (!loc) return malformed();
        std::printf("key hash %s\n", hex_id(loc->key_hash).c_str());
        for (std::size_t i = 0; i < loc->nodes.size(); ++i)
            print_node(i == 0 ? "primary" : "replica", loc->nodes[i]);
        return 0;
    }
    if (request_type == MessageType::GetRequest) {
        const auto value = decode_value(response.payload);
        if (!value) {
            std::fprintf(stderr, "error: malformed response from node\n");
            return 2;
        }
        print(value->value);
    } else {
        print("OK");
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 4) return usage(argv[0]);

    const auto port = parse_port(argv[2]);
    if (!port) return usage(argv[0]);

    const auto addr = make_address(argv[1], *port);
    if (!addr) {
        std::fprintf(stderr, "error: invalid address %s\n", argv[1]);
        return 2;
    }
    const std::string node = to_string(*addr);

    // ── Build the request payload ───────────────────────────────────────
    const std::string_view command = argv[3];
    MessageType type{};
    std::vector<std::byte> payload;
    try {
        if (command == "put" && argc == 6) {
            type = MessageType::PutRequest;
            payload = encode_payload(KeyValuePayload{argv[4], argv[5]});
        } else if (command == "get" && argc == 5) {
            type = MessageType::GetRequest;
            payload = encode_payload(KeyPayload{argv[4]});
        } else if (command == "delete" && argc == 5) {
            type = MessageType::DeleteRequest;
            payload = encode_payload(KeyPayload{argv[4]});
        } else if (command == "status" && argc == 4) {
            type = MessageType::StatusRequest;  // no payload
        } else if (command == "locate" && argc == 5) {
            type = MessageType::LocateRequest;
            payload = encode_payload(KeyPayload{argv[4]});
        } else {
            return usage(argv[0]);
        }
    } catch (const std::length_error&) {
        std::fprintf(stderr, "error: key must be at most %zu bytes, value at most %zu bytes\n",
                     kMaxKeySize, kMaxValueSize);
        return 2;
    }

    // A random sender_id identifies this client; together with request_id it
    // identifies the request. Retries reuse both, so the node sees one request.
    std::mt19937_64 rng(std::random_device{}());
    const std::uint64_t sender_id = rng();
    const std::uint64_t request_id = rng();
    const auto request = encode_message(
        {.type = type, .request_id = request_id, .sender_id = sender_id}, payload);

    // ── Socket ──────────────────────────────────────────────────────────
    UniqueFD sock(::socket(AF_INET6, SOCK_DGRAM | SOCK_CLOEXEC, 0));
    if (!sock) {
        std::perror("socket");
        return 2;
    }

    int v6only = 0;  // allow IPv4 nodes (::ffff:a.b.c.d)
    ::setsockopt(sock.get(), IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof v6only);

    timeval timeout{.tv_sec = 0, .tv_usec = kTimeoutMs * 1000};
    ::setsockopt(sock.get(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

    // connect() on UDP sends nothing: it fixes the destination and makes the
    // kernel report "port unreachable" to us as ECONNREFUSED.
    if (::connect(sock.get(), reinterpret_cast<const sockaddr*>(&*addr), sizeof *addr) < 0) {
        std::perror("connect");
        return 2;
    }

    // ── Send, wait, retry with the same request ─────────────────────────
    std::array<std::byte, kMaxDatagramSize> buf;

    for (int attempt = 1; attempt <= kAttempts; ++attempt) {
        if (::send(sock.get(), request.data(), request.size(), 0) < 0) {
            std::perror("send");
            return 2;
        }

        for (;;) {
            const ssize_t n = ::recv(sock.get(), buf.data(), buf.size(), 0);
            if (n < 0) {
                if (errno == EINTR) continue;
                if (errno == EAGAIN || errno == EWOULDBLOCK) break;  // timed out: retry
                if (errno == ECONNREFUSED) {
                    std::fprintf(stderr, "error: no node listening at %s\n", node.c_str());
                    return 2;
                }
                std::perror("recv");
                return 2;
            }

            // Ignore anything that isn't the answer to *this* request,
            // e.g. a late reply to an earlier attempt of another client run.
            const auto frame = parse_datagram(std::span(buf).first(static_cast<std::size_t>(n)));
            if (!frame) continue;
            if (frame->header.type != response_for(type)) continue;
            if (frame->header.request_id != request_id || frame->header.sender_id != sender_id) continue;

            return print_result(type, *frame);
        }

        if (attempt < kAttempts)
            std::fprintf(stderr, "no answer within %d ms, retrying (%d/%d)\n",
                         kTimeoutMs, attempt + 1, kAttempts);
    }

    std::fprintf(stderr, "error: no answer from %s after %d attempts\n", node.c_str(), kAttempts);
    return 2;
}
