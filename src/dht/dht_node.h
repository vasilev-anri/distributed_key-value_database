#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <opendht.h>

#include "dht/node_info.h"

class Mailbox;

// Cluster membership through OpenDHT: every node announces its NodeInfo under a
// shared cluster key and listens on that key to learn about the others.
//
// OpenDHT runs its own threads. All callbacks of this class are delivered
// through the Mailbox, so they always run on the reactor thread.
// A DHTNode must outlive the reactor loop (tasks it posts refer to it).
class DHTNode {
public:
    struct Config {
        std::uint16_t port = 4222;            // OpenDHT's own UDP port
        std::string bootstrap_host;           // empty: first node, nothing to join
        std::string bootstrap_port = "4222";
        std::string cluster_name = "kvdb";    // nodes with the same name form one cluster
    };

    using PeerCallback = std::move_only_function<void(const NodeInfo& peer)>;

    explicit DHTNode(Mailbox& mailbox);
    ~DHTNode();

    DHTNode(const DHTNode&) = delete;
    DHTNode& operator=(const DHTNode&) = delete;

    // Starts OpenDHT's threads. Create the SignalHandler before calling this.
    void start(const Config& config);

    // Publishes our info; OpenDHT keeps re-announcing it while we run.
    void announce(const NodeInfo& self);

    // on_found: a node announced itself. on_gone: its announcement expired.
    // Our own announcement is never reported.
    void discover(PeerCallback on_found, PeerCallback on_gone);

    // Number of DHT nodes we currently know (for logs / debugging).
    [[nodiscard]] std::size_t good_nodes() const;

    // Stops OpenDHT's threads. Safe to call more than once.
    void shutdown();

private:
    void deliver(const std::vector<NodeInfo>& peers, bool expired);

    Mailbox& mailbox_;
    dht::DhtRunner runner_;
    dht::InfoHash cluster_key_;
    std::uint64_t self_id_ = 0;
    bool running_ = false;

    PeerCallback on_found_;  // used on the reactor thread only
    PeerCallback on_gone_;   // used on the reactor thread only
};