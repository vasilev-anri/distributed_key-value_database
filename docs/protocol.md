# Wire Protocol — Version 1

This document defines every message exchanged between clients and nodes
(UDP data channel) and between nodes (TCP control channel).
Implementation: `src/protocol/` (`protocol.h`, `codec.h`, `codec.cpp`).

---

## 1. General rules

- **Byte order:** all integers are unsigned and **big-endian** (network byte order).
- **One format, two transports:** every message is a fixed **28-byte header**
  followed by a payload of `length` bytes. The same format is used over UDP and TCP.
- **UDP:** one datagram carries **exactly one** message. A datagram is at most
  **1232 bytes** (IPv6 minimum MTU 1280 − 40 IPv6 − 8 UDP), so it is never
  fragmented. Truncated datagrams or datagrams with trailing bytes are invalid.
- **TCP:** messages follow each other in the byte stream. The header's `length`
  field tells where a message ends (length-prefixed framing).
- **Invalid input:** a node **drops** an invalid UDP datagram and **closes** a TCP
  connection that sends an invalid message. Nothing invalid is ever processed.

---

## 2. Header (28 bytes)

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-------------------------------+---------------+---------------+
|          magic = 0x4B56       |  version = 1  |     type      |
+---------------+---------------+-------------------------------+
|     flags     |    status     |           reserved            |
+---------------+---------------+-------------------------------+
|                                                               |
+                          request_id                           +
|                                                               |
+---------------------------------------------------------------+
|                                                               |
+                           sender_id                           +
|                                                               |
+---------------------------------------------------------------+
|                            length                             |
+---------------------------------------------------------------+
|                     payload (length bytes)                    |
```

| Offset | Size | Field | Meaning |
|---:|---:|---|---|
| 0 | 2 | `magic` | Always `0x4B56` (`"KV"`). Anything else is rejected immediately. |
| 2 | 1 | `version` | Protocol version, currently `1`. |
| 3 | 1 | `type` | Message type (section 3). Unknown types are rejected. |
| 4 | 1 | `flags` | Bit 0 = `FORWARDED` (section 5), bit 1 = `SYNC` (section 6, recovery). Other bits are reserved: sent as 0, ignored on receipt. |
| 5 | 1 | `status` | Result code in responses and acks (section 4); `0` in requests. |
| 6 | 2 | `reserved` | Sent as 0, ignored on receipt (room for future versions). |
| 8 | 8 | `request_id` | Chosen by the original sender; unique per sender. |
| 16 | 8 | `sender_id` | Id of the original sender (client id or node id). |
| 24 | 4 | `length` | Payload length in bytes, at most **65 536**. |

---

## 3. Message types

### UDP data channel — client ↔ node, node → node (forwarding)

A response's type is the request's type with the high bit set (`type | 0x80`).

| Type | Name | Payload | Notes |
|---|---|---|---|
| `0x01` | `PUT` | key, value | Store or overwrite a value. |
| `0x02` | `GET` | key | Read a value. |
| `0x03` | `DELETE` | key | Remove a key. |
| `0x04` | `STATUS` | — | How the node sees the cluster (section 3a). |
| `0x05` | `LOCATE` | key | Which nodes hold a key (section 3a). |
| `0x81` | `PUT_RESPONSE` | — | `status` = result. |
| `0x82` | `GET_RESPONSE` | value *(only if `status = OK`)* | |
| `0x83` | `DELETE_RESPONSE` | — | `status` = result. |
| `0x84` | `STATUS_RESPONSE` | status info | Always `status = OK`. |
| `0x85` | `LOCATE_RESPONSE` | locate info *(only if `status = OK`)* | `KEY_TOO_LARGE` / `INVALID_REQUEST` for a bad key. |

### TCP control channel — node ↔ node

| Type | Name | Payload | Notes |
|---|---|---|---|
| `0x10` | `HELLO` | udp_port, tcp_port | First message on every connection; `sender_id` = node id. |
| `0x11` | `HELLO_ACK` | — | Reply to `HELLO`; `sender_id` = node id. |
| `0x12` | `PING` | — | Heartbeat. |
| `0x13` | `PONG` | — | Reply to `PING`, same `request_id`. |
| `0x20` | `REPLICA_PUT` | key, value, version | A copy of a write: apply it if `version` is newer. |
| `0x21` | `REPLICA_DELETE` | key, version | A tombstone: apply it if `version` is newer. |
| `0x22` | `REPLICA_ACK` | — | Replica → primary, same `request_id`; `status` = result. Never sent for `SYNC` messages. |

### Payload encoding

| Element | Encoding | Limit |
|---|---|---|
| key | `u16` length + bytes | 128 bytes |
| value | `u32` length + bytes | 1024 bytes |
| udp_port, tcp_port | `u16` each | — |
| version | `u64` | — |

Keys and values are arbitrary bytes (not necessarily text). A payload must be
consumed exactly: missing or leftover bytes make the message invalid.

The limits guarantee that the largest `PUT`
(28 + 2 + 128 + 4 + 1024 = **1186 bytes**) fits in one datagram.

### 3a. Cluster information — `STATUS` and `LOCATE`

For the UI and for `kvcli`. Both are answered **by the node that receives
them and never forwarded**: each node reports its own view of the cluster.
To draw the whole cluster, ask every node; nodes may briefly disagree (for
example, right after a failure one node may already see a peer as down while
another still sees it as connected).

**Node entry** (used in both responses):

| Field | Encoding | Meaning |
|---|---|---|
| id | `u64` | Node id (`key_hash` of its canonical address). |
| address | `u8` length + bytes, ≤ 64 | Canonical address, e.g. `[::1]:9001`. |
| state | `u8` | `0` SELF (the answering node), `1` CONNECTED (control link up and answering heartbeats), `2` CONNECTING (trying to reconnect), `3` DISCONNECTED (down, waiting for it to reconnect). Other values are invalid. |

A node list is a `u8` count (at most **16**) followed by that many entries.
With these limits a response always fits in one datagram
(28 + 14 + 16 × 74 = 1226 bytes).

**`STATUS_RESPONSE` payload:**

| Field | Encoding | Meaning |
|---|---|---|
| node_id | `u64` | The answering node. |
| key_count | `u32` | Entries stored on it: keys it is primary for plus replica copies. |
| replication_factor | `u8` | Copies per key (2). |
| nodes | node list | Every node, the answering node first (state SELF). |

**`LOCATE_RESPONSE` payload** (request payload = key):

| Field | Encoding | Meaning |
|---|---|---|
| key_hash | `u64` | The key's position in the id space (FNV-1a 64 + SplitMix64 mix, see `src/cluster/hash.h`). |
| nodes | node list | The key's replica set, primary first, then the replica. |

The XOR distance from a key to a node is `key_hash XOR node_id`; the primary
is the node with the smallest distance, the replica the second smallest.

---

## 4. Status codes

| Code | Name | Meaning |
|---:|---|---|
| 0 | `OK` | Success. |
| 1 | `NOT_FOUND` | `GET` / `DELETE` of a key that doesn't exist. |
| 2 | `INVALID_REQUEST` | The request was well-formed but not acceptable. |
| 3 | `KEY_TOO_LARGE` | Key exceeds 128 bytes. |
| 4 | `VALUE_TOO_LARGE` | Value exceeds 1024 bytes. |
| 5 | `REPLICATION_FAILED` | The replica did not confirm the write in time. |
| 6 | `UNAVAILABLE` | The responsible node can't be reached right now; retry later. |
| 7 | `INTERNAL_ERROR` | Unexpected failure on the node. |

---

## 5. Request ids, retries and forwarding

**Request identity.** A request is identified by the pair
`(sender_id, request_id)`. Clients pick a random 64-bit `sender_id` at startup
and increment `request_id` for each new request.

**Responses** copy `request_id` and `sender_id` from the request, so the
client can match each response to its request.

**Retries.** If a client gets no response before its timeout, it resends the
**same request with the same `request_id`**. The primary remembers the responses
to the last 4096 writes it executed, keyed by `(sender_id, request_id)`, and
answers a retry with the stored response instead of applying the write twice
(duplicate detection). A retry that arrives while the first attempt is still
being replicated is ignored: the first attempt's answer is on its way. Writes
that ended in `REPLICATION_FAILED` are not remembered, so a retry executes them
again (safe: `PUT` and `DELETE` are idempotent).

**Forwarding.** If the node that receives a request is not the key's owner, it
forwards the request unchanged — same `sender_id` and `request_id` — to the
owner, with the `FORWARDED` flag set, and relays the owner's response back to
the client. A node **never forwards a message that already has `FORWARDED`
set**: if it is not the owner either (for example, because membership changed
a moment ago), it answers `UNAVAILABLE` and the client retries. This prevents
forwarding loops between nodes that briefly disagree about ownership.
The one exception: a forwarded `GET` that reaches the key's **replica** is
answered from the replica's copy (see section 6, replication rules).

---

## 6. Message flows

### PUT through a non-owner node (replication factor 2)

```
Client            Node A (entry)        Node C (primary)       Node D (replica)
  |  PUT  (UDP)        |                      |                      |
  |------------------->|  PUT, FORWARDED (UDP)|                      |
  |                    |--------------------->|  REPLICA_PUT (TCP)   |
  |                    |                      |--------------------->|
  |                    |                      |  REPLICA_ACK OK      |
  |                    |                      |<---------------------|
  |                    |  PUT_RESPONSE OK     |                      |
  |  PUT_RESPONSE OK   |<---------------------|                      |
  |<-------------------|                      |                      |
```

A `PUT` succeeds only when both the primary and the replica have the value.

### Replication rules

- **Replica set:** the `kReplicationFactor = 2` nodes closest to the key by XOR
  distance. The closest is the **primary**, the second is the **replica**. All
  nodes compute the same set from the same node list.
- **Writes** (`PUT`, `DELETE`) are executed only by the primary: it applies the
  write, sends `REPLICA_PUT` / `REPLICA_DELETE` over the control channel, and
  answers the client only after `REPLICA_ACK OK`. `DELETE` is replicated even if
  the key was `NOT_FOUND` on the primary, so both copies end up identical.
- **Replica side:** applies the write and answers `REPLICA_ACK` with the same
  `request_id` (the primary's own counter; `sender_id` = node id). It answers
  `INVALID_REQUEST` and stores nothing if it isn't in the key's replica set.
- **Replication failure:** if the replica isn't connected, rejects the write, or
  doesn't ACK within **100 ms**, the client gets `REPLICATION_FAILED`. The
  primary keeps the value it already applied; the client should retry.
- **Reads** go to the primary. If it doesn't answer within **200 ms** (the
  forward timeout), the entry node asks the replica. Nodes whose control
  connection is down are asked last, so a read for a key whose primary is known
  to be down goes straight to the replica, or is answered locally if the entry
  node is the replica.
- **Writes to a primary known to be down** get `UNAVAILABLE` immediately.
- **Versions:** every entry has a 64-bit version, assigned by the primary when
  it executes the write: `max(last + 1, unix_ms << 16)` (a hybrid logical
  clock: always increasing, also across restarts, and never below a version
  the node has seen). A node applies a `REPLICA_PUT` / `REPLICA_DELETE` only
  if its version is newer than the entry it has; an older or equal one
  changes nothing (it still gets `REPLICA_ACK OK`). So copies can arrive
  twice, late, or out of order.
- **Tombstones:** `DELETE` doesn't erase the key, it stores a tombstone
  (deleted, with the delete's version). `GET` treats it as `NOT_FOUND`. The
  tombstone is what stops an older copy of the key, held by a node that
  missed the delete, from bringing it back during recovery.
- **Timeouts:** 100 ms (replica ACK) < 200 ms (forward) and 2 × 200 ms < 500 ms
  (the client's timeout), so every node in the chain answers before the one
  waiting for it gives up.

### Recovery

When a control connection becomes established (at startup, after a node
restarted, or after it was unreachable), **both** nodes send each other every
entry, tombstones included, for which the other node is in the key's replica
set: `REPLICA_PUT` / `REPLICA_DELETE` with `request_id = 0` and the `SYNC`
flag set. The receiver applies each one if it is newer, and sends no
`REPLICA_ACK`. Entries are sent in batches of 256 per event-loop turn.

Results: a node that restarted empty gets its keys back from the surviving
copies within milliseconds of reconnecting; two copies that diverged (for
example after `REPLICATION_FAILED`) both end up with the newest version; a
deleted key stays deleted. Until its sync has arrived, a restarted primary may
briefly answer `NOT_FOUND` for keys it hasn't received yet.

### Node connection and heartbeats (TCP)

```
Node A                         Node B
  |  HELLO (id A, ports)          |
  |------------------------------>|
  |  HELLO_ACK (id B)             |
  |<------------------------------|
  |  PING (request_id 1)          |
  |------------------------------>|
  |  PONG (request_id 1)          |
  |<------------------------------|
```

**Heartbeats.** Once the handshake is done, both sides send a `PING` every
**250 ms** on every control connection, each with a new `request_id`; the
receiver answers `PONG` with the same `request_id`. A `PONG` to any `PING` we
sent (even a late one) means the peer is alive. If **4 heartbeat intervals**
(1 s) pass without any `PONG`, the peer is considered failed and the
connection is closed; the normal reconnect rules then apply. This detects a
node that is frozen or overloaded while its TCP connection stays open, which
TCP alone would never report. `PING` / `PONG` are handled by the connection
itself and never reach replication or routing logic.

### Control channel rules

- **Port:** a node's TCP control channel listens on the **same port number** as
  its UDP data channel (e.g. `[::1]:9001` for both).
- **One connection per pair:** only the node with the **lower node id**
  connects (Node A above); the other only accepts. A node rejects an incoming
  connection from an unknown node or from a node with a higher id than its own.
- **Handshake:** the connecting node sends `HELLO` (its node id in `sender_id`,
  its ports in the payload) as the first message. The accepting node answers
  `HELLO_ACK` with its own node id. The connecting node checks that this is the
  node it intended to reach. Any other first message closes the connection.
- **Timeout:** connect + handshake must complete within 3 seconds.
- **Reconnects:** if a connection drops, the connecting node retries with
  backoff (0.25 s, doubling, at most 4 s). If a node reconnects while an old
  connection to it still exists, the new connection replaces the old one.

---

## 7. Versioning

- `version` changes only for incompatible changes; a node rejects other versions.
- New message types can be added within a version; nodes reject types they
  don't know.
- `reserved` and unused `flags` bits are available for future, compatible use.
