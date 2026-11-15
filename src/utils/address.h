#pragma once

#include <cstdint>
#include <cstring>
#include <optional>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>

// Parses "::1" or "127.0.0.1" into an IPv6 socket address.
// IPv4 becomes an IPv4-mapped address (::ffff:a.b.c.d), so one type covers both.
inline std::optional<sockaddr_in6> make_address(const std::string& ip, std::uint16_t port) {
    sockaddr_in6 addr{};
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);

    if (::inet_pton(AF_INET6, ip.c_str(), &addr.sin6_addr) == 1) return addr;

    in_addr v4{};
    if (::inet_pton(AF_INET, ip.c_str(), &v4) == 1) {
        addr.sin6_addr.s6_addr[10] = 0xff;
        addr.sin6_addr.s6_addr[11] = 0xff;
        std::memcpy(&addr.sin6_addr.s6_addr[12], &v4, sizeof v4);
        return addr;
    }
    return std::nullopt;
}

// Parses an endpoint: "[::1]:9001" (IPv6, brackets required) or
// "127.0.0.1:9001" (IPv4). Returns std::nullopt if it's not valid.
inline std::optional<sockaddr_in6> parse_endpoint(const std::string& text) {
    std::string host;
    std::string port_text;

    if (!text.empty() && text.front() == '[') {
        const auto close = text.find(']');
        if (close == std::string::npos || close + 1 >= text.size() || text[close + 1] != ':')
            return std::nullopt;
        host = text.substr(1, close - 1);
        port_text = text.substr(close + 2);
    } else {
        const auto colon = text.rfind(':');
        if (colon == std::string::npos || text.find(':') != colon) return std::nullopt;  // bare IPv6 needs []
        host = text.substr(0, colon);
        port_text = text.substr(colon + 1);
    }

    if (port_text.empty() || port_text.size() > 5) return std::nullopt;
    unsigned port = 0;
    for (const char c : port_text) {
        if (c < '0' || c > '9') return std::nullopt;
        port = port * 10 + static_cast<unsigned>(c - '0');
    }
    if (port == 0 || port > 65535) return std::nullopt;

    return make_address(host, static_cast<std::uint16_t>(port));
}

// Formats an address as "[ip]:port" for logs.
inline std::string to_string(const sockaddr_in6& addr) {
    char buf[INET6_ADDRSTRLEN];
    ::inet_ntop(AF_INET6, &addr.sin6_addr, buf, sizeof buf);
    return "[" + std::string(buf) + "]:" + std::to_string(ntohs(addr.sin6_port));
}
