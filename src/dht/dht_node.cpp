#include "dht/dht_node.h"

#include <exception>
#include <memory>

#include <sys/socket.h>

#include "handlers/mailbox.h"

namespace {

// Private network id: our nodes only talk to each other, never to the public DHT.
constexpr dht::NetId kNetworkId = 0x6b76'6462;  // "kvdb"

}  // namespace

DHTNode::DHTNode(Mailbox& mailbox) : mailbox_(mailbox) {}

DHTNode::~DHTNode() { shutdown(); }

void DHTNode::start(const Config& config) {
    cluster_key_ = dht::InfoHash::get("kvdb-cluster/" + config.cluster_name);

    // Empty identity: we don't sign or encrypt values, and generating an RSA
    // identity would only slow down startup.
    runner_.run(config.port, {}, true, kNetworkId);
    running_ = true;

    if (!config.bootstrap_host.empty())
        runner_.bootstrap(config.bootstrap_host, config.bootstrap_port);
}

void DHTNode::announce(const NodeInfo& self) {
    self_id_ = self.node_id;

    // permanent = true: OpenDHT re-publishes the value before it expires,
    // for as long as this node is running.
    runner_.put(cluster_key_, dht::Value(dht::packMsg(self)),
                dht::DoneCallbackSimple{},  // no completion callback needed
                dht::time_point::max(), true);
}

void DHTNode::discover(PeerCallback on_found, PeerCallback on_gone) {
    on_found_ = std::move(on_found);
    on_gone_ = std::move(on_gone);

    runner_.listen(cluster_key_,
        [this](const std::vector<std::shared_ptr<dht::Value>>& values, bool expired) {
            // OpenDHT thread: decode and copy, never touch engine state here.
            std::vector<NodeInfo> peers;
            peers.reserve(values.size());
            for (const auto& value : values) {
                try {
                    peers.push_back(dht::unpackMsg<NodeInfo>(value->data));
                } catch (const std::exception&) {
                    // Not a NodeInfo (malformed or foreign value): skip it.
                }
            }

            if (!peers.empty()) {
                mailbox_.post([this, peers = std::move(peers), expired] {
                    deliver(peers, expired);  // reactor thread
                });
            }
            return true;  // keep listening
        });
}

void DHTNode::deliver(const std::vector<NodeInfo>& peers, bool expired) {
    for (const NodeInfo& peer : peers) {
        if (peer.node_id == self_id_) continue;  // our own announcement
        PeerCallback& cb = expired ? on_gone_ : on_found_;
        if (cb) cb(peer);
    }
}

std::size_t DHTNode::good_nodes() const {
    return runner_.getNodesStats(AF_INET).good_nodes + runner_.getNodesStats(AF_INET6).good_nodes;
}

void DHTNode::shutdown() {
    if (!running_) return;
    running_ = false;
    runner_.join();  // stops OpenDHT's threads; no more callbacks after this
}