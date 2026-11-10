#pragma once

#include <cstdint>
#include <string>

#include <msgpack.hpp>

// What a node announces about itself in the DHT.
struct NodeInfo {
    std::uint64_t node_id = 0;
    std::string ip;              // address other nodes use to reach us, e.g. "::1"
    std::uint16_t udp_port = 0;  // data channel
    std::uint16_t tcp_port = 0;  // control channel

    MSGPACK_DEFINE(node_id, ip, udp_port, tcp_port)
};