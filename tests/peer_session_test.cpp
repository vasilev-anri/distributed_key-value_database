// Two PeerSessions connected back to back in memory: what one sends, the other
// receives. Tests the handshake and its failure cases without any sockets.

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <system_error>
#include <vector>

#include "cluster/peer_session.h"
#include "protocol/codec.h"

using namespace protocol;
using cluster::PeerSession;

namespace {

constexpr std::uint64_t kLowId = 100;   // connects (lower id)
constexpr std::uint64_t kHighId = 200;  // accepts

// Everything one side observed.
struct Side {
    std::vector<std::byte> outbox;            // bytes it sent
    std::optional<std::error_code> closed;    // set if it closed
    std::optional<std::uint64_t> established;
    std::vector<MessageType> messages;        // control messages after the handshake
};

PeerSession::Callbacks callbacks(Side& side, bool accept = true) {
    PeerSession::Callbacks cb;
    cb.send = [&side](std::span<const std::byte> d) {
        side.outbox.insert(side.outbox.end(), d.begin(), d.end());
        return true;
    };
    cb.close = [&side](std::error_code r) { side.closed = r; };
    cb.established = [&side](std::uint64_t id) { side.established = id; };
    cb.message = [&side](std::uint64_t, const Frame& f) { side.messages.push_back(f.header.type); };
    cb.accept_peer = [accept](std::uint64_t id) { return accept && id == kLowId; };
    return cb;
}

// Delivers everything `from` sent to `to` (like TCP would), keeping any
// incomplete remainder, exactly as TCPConnection does.
void pump(Side& from, PeerSession& to) {
    const std::size_t used = to.on_data(from.outbox);
    from.outbox.erase(from.outbox.begin(), from.outbox.begin() + static_cast<std::ptrdiff_t>(used));
}

std::vector<std::byte> message(MessageType type, std::uint64_t sender = 0) {
    return encode_message({.type = type, .sender_id = sender});
}

}  // namespace

TEST(PeerSession, HandshakeEstablishesBothSides) {
    Side low, high;
    auto out = PeerSession::outbound({kLowId, 9001, 9001}, kHighId, callbacks(low));
    auto in = PeerSession::inbound({kHighId, 9002, 9002}, callbacks(high));

    out.on_connected();               // low sends HELLO
    EXPECT_EQ(out.state(), PeerSession::State::AwaitingHelloAck);
    pump(low, in);                    // high receives HELLO, answers HELLO_ACK
    pump(high, out);                  // low receives HELLO_ACK

    EXPECT_TRUE(out.established());
    EXPECT_TRUE(in.established());
    EXPECT_EQ(low.established, kHighId);
    EXPECT_EQ(high.established, kLowId);
    EXPECT_EQ(in.peer_id(), kLowId);  // learned from HELLO
    EXPECT_FALSE(low.closed);
    EXPECT_FALSE(high.closed);
}

TEST(PeerSession, MessagesAfterHandshakeAreDelivered) {
    Side low, high;
    auto out = PeerSession::outbound({kLowId, 9001, 9001}, kHighId, callbacks(low));
    auto in = PeerSession::inbound({kHighId, 9002, 9002}, callbacks(high));
    out.on_connected();
    pump(low, in);
    pump(high, out);

    EXPECT_TRUE(out.send(message(MessageType::ReplicaDelete, kLowId)));
    EXPECT_TRUE(out.send(message(MessageType::ReplicaAck, kLowId)));
    pump(low, in);  // two messages in one chunk

    EXPECT_EQ(high.messages, (std::vector<MessageType>{MessageType::ReplicaDelete, MessageType::ReplicaAck}));
}

TEST(PeerSession, HelloArrivingByteByByteStillWorks) {
    Side high;
    auto in = PeerSession::inbound({kHighId, 9002, 9002}, callbacks(high));
    const auto hello = encode_message({.type = MessageType::Hello, .sender_id = kLowId},
                                      encode_payload(HelloPayload{9001, 9001}));

    std::vector<std::byte> buffer;
    for (const std::byte b : hello) {  // TCP may deliver any split
        buffer.push_back(b);
        const std::size_t used = in.on_data(buffer);
        buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(used));
    }
    EXPECT_TRUE(in.established());
    EXPECT_TRUE(buffer.empty());
}

TEST(PeerSession, CannotSendBeforeHandshake) {
    Side low;
    auto out = PeerSession::outbound({kLowId, 9001, 9001}, kHighId, callbacks(low));
    out.on_connected();
    const std::size_t hello_size = low.outbox.size();
    EXPECT_FALSE(out.send(message(MessageType::Ping)));
    EXPECT_EQ(low.outbox.size(), hello_size);  // nothing extra went out
}

TEST(PeerSession, UnknownPeerIsRejected) {
    Side high;
    auto in = PeerSession::inbound({kHighId, 9002, 9002}, callbacks(high, /*accept=*/false));
    in.on_data(encode_message({.type = MessageType::Hello, .sender_id = kLowId},
                              encode_payload(HelloPayload{9001, 9001})));

    ASSERT_TRUE(high.closed);
    EXPECT_EQ(*high.closed, std::make_error_code(std::errc::permission_denied));
    EXPECT_FALSE(high.established);
    EXPECT_TRUE(high.outbox.empty());  // no HELLO_ACK for strangers
}

TEST(PeerSession, PeerClaimingOurOwnIdIsRejected) {
    Side high;
    auto in = PeerSession::inbound({kHighId, 9002, 9002}, callbacks(high));
    in.on_data(encode_message({.type = MessageType::Hello, .sender_id = kHighId},
                              encode_payload(HelloPayload{9002, 9002})));
    EXPECT_TRUE(high.closed);
}

TEST(PeerSession, FirstMessageMustBeHello) {
    Side high;
    auto in = PeerSession::inbound({kHighId, 9002, 9002}, callbacks(high));
    in.on_data(message(MessageType::Ping, kLowId));
    ASSERT_TRUE(high.closed);
    EXPECT_EQ(*high.closed, std::make_error_code(std::errc::protocol_error));
}

TEST(PeerSession, WrongNodeAnsweringIsRejected) {
    Side low;
    auto out = PeerSession::outbound({kLowId, 9001, 9001}, kHighId, callbacks(low));
    out.on_connected();
    out.on_data(message(MessageType::HelloAck, /*sender=*/999));  // not who we dialed
    EXPECT_TRUE(low.closed);
    EXPECT_FALSE(low.established);
}

TEST(PeerSession, GarbageClosesTheConnection) {
    Side high;
    auto in = PeerSession::inbound({kHighId, 9002, 9002}, callbacks(high));
    const std::vector<std::byte> garbage{std::byte{'G'}, std::byte{'E'}, std::byte{'T'}};
    in.on_data(garbage);
    EXPECT_TRUE(high.closed);
}

TEST(PeerSession, SecondHelloIsAProtocolError) {
    Side low, high;
    auto out = PeerSession::outbound({kLowId, 9001, 9001}, kHighId, callbacks(low));
    auto in = PeerSession::inbound({kHighId, 9002, 9002}, callbacks(high));
    out.on_connected();
    pump(low, in);
    pump(high, out);

    in.on_data(encode_message({.type = MessageType::Hello, .sender_id = kLowId},
                              encode_payload(HelloPayload{9001, 9001})));
    EXPECT_TRUE(high.closed);
}

TEST(PeerSession, NothingIsProcessedAfterClose) {
    Side high;
    auto in = PeerSession::inbound({kHighId, 9002, 9002}, callbacks(high));
    // Garbage followed by a valid HELLO in the same chunk: the HELLO must be ignored.
    std::vector<std::byte> data = message(MessageType::Ping, kLowId);
    const auto hello = encode_message({.type = MessageType::Hello, .sender_id = kLowId},
                                      encode_payload(HelloPayload{9001, 9001}));
    data.insert(data.end(), hello.begin(), hello.end());
    in.on_data(data);

    EXPECT_TRUE(high.closed);
    EXPECT_FALSE(high.established);
}

// ── Heartbeats ──────────────────────────────────────────────────────────

namespace {

// Two sessions with the handshake done, and their sides cleared.
struct Pair {
    Side low, high;
    PeerSession out = PeerSession::outbound({kLowId, 9001, 9001}, kHighId, callbacks(low));
    PeerSession in = PeerSession::inbound({kHighId, 9002, 9002}, callbacks(high));

    Pair() {
        out.on_connected();
        pump(low, in);
        pump(high, out);
        low.outbox.clear();
        high.outbox.clear();
    }
};

}  // namespace

TEST(Heartbeat, PingIsAnsweredWithPongAndResetsTheCount) {
    Pair p;
    p.out.heartbeat();  // low sends PING
    EXPECT_EQ(p.out.unanswered_pings(), 1);

    pump(p.low, p.in);   // high answers PONG
    pump(p.high, p.out); // low receives it
    EXPECT_EQ(p.out.unanswered_pings(), 0);

    // Neither side saw PING / PONG as an ordinary message.
    EXPECT_TRUE(p.low.messages.empty());
    EXPECT_TRUE(p.high.messages.empty());
}

TEST(Heartbeat, PongCarriesThePingsRequestId) {
    Pair p;
    p.out.heartbeat();
    p.out.heartbeat();
    pump(p.low, p.in);

    // Two PONGs, request ids 1 and 2, from high's node id.
    std::span<const std::byte> rest = p.high.outbox;
    for (std::uint64_t expected : {1u, 2u}) {
        const auto frame = try_parse_message(rest);
        ASSERT_TRUE(frame && *frame);
        EXPECT_EQ((*frame)->header.type, MessageType::Pong);
        EXPECT_EQ((*frame)->header.request_id, expected);
        EXPECT_EQ((*frame)->header.sender_id, kHighId);
        rest = rest.subspan((*frame)->size);
    }
}

TEST(Heartbeat, SilentPeerIsDeclaredDead) {
    Pair p;  // high never gets to answer: we don't pump
    for (int i = 0; i < PeerSession::kMaxMissedPongs; ++i) {
        p.out.heartbeat();
        EXPECT_FALSE(p.low.closed) << "closed too early, after " << i + 1 << " pings";
    }
    p.out.heartbeat();  // the oldest unanswered PING is now kMaxMissedPongs intervals old

    ASSERT_TRUE(p.low.closed);
    EXPECT_EQ(*p.low.closed, std::make_error_code(std::errc::timed_out));
}

TEST(Heartbeat, LatePongStillCountsAsAlive) {
    Pair p;
    p.out.heartbeat();
    p.out.heartbeat();
    p.out.heartbeat();  // 3 unanswered...
    pump(p.low, p.in);  // ...high answers all three at once (e.g. after a short stall)
    pump(p.high, p.out);

    EXPECT_EQ(p.out.unanswered_pings(), 0);
    for (int i = 0; i < PeerSession::kMaxMissedPongs; ++i) p.out.heartbeat();
    EXPECT_FALSE(p.low.closed);
}

TEST(Heartbeat, PongForAPingNeverSentIsIgnored) {
    Pair p;
    p.out.heartbeat();
    p.out.on_data(encode_message({.type = MessageType::Pong, .request_id = 99, .sender_id = kHighId}));
    EXPECT_EQ(p.out.unanswered_pings(), 1);
}

TEST(Heartbeat, NothingIsSentBeforeTheHandshake) {
    Side low;
    auto out = PeerSession::outbound({kLowId, 9001, 9001}, kHighId, callbacks(low));
    out.heartbeat();
    EXPECT_TRUE(low.outbox.empty());
    for (int i = 0; i < 10; ++i) out.heartbeat();
    EXPECT_FALSE(low.closed);  // the handshake has its own timeout
}

TEST(Heartbeat, BothSidesCanMonitorEachOther) {
    Pair p;
    for (int round = 0; round < 10; ++round) {
        p.out.heartbeat();
        p.in.heartbeat();
        pump(p.low, p.in);
        pump(p.high, p.out);
    }
    EXPECT_FALSE(p.low.closed);
    EXPECT_FALSE(p.high.closed);
    EXPECT_EQ(p.out.unanswered_pings(), 0);
    EXPECT_EQ(p.in.unanswered_pings(), 0);
}
