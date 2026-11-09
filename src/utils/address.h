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

// Formats an address as "[ip]:port" for logs.
inline std::string to_string(const sockaddr_in6& addr) {
    char buf[INET6_ADDRSTRLEN];
    ::inet_ntop(AF_INET6, &addr.sin6_addr, buf, sizeof buf);
    return "[" + std::string(buf) + "]:" + std::to_string(ntohs(addr.sin6_port));
}