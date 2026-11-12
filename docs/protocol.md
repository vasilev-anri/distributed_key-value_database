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
| 4 | 1 | `flags` | Bit 0 = `FORWARDED` (section 5). Other bits are reserved: sent as 0, ignored on receipt. |
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
| `0x81` | `PUT_RESPONSE` | — | `status` = result. |
| `0x82` | `GET_RESPONSE` | value *(only if `status = OK`)* | |
| `0x83` | `DELETE_RESPONSE` | — | `status` = result. |

### TCP control channel — node ↔ node

| Type | Name | Payload | Notes |
|---|---|---|---|
| `0x10` | `HELLO` | udp_port, tcp_port | First message on every connection; `sender_id` = node id. |
| `0x11` | `HELLO_ACK` | — | Reply to `HELLO`; `sender_id` = node id. |
| `0x12` | `PING` | — | Heartbeat. |
| `0x13` | `PONG` | — | Reply to `PING`, same `request_id`. |
| `0x20` | `REPLICA_PUT` | key, value | Primary → replica: apply this write. |
| `0x21` | `REPLICA_DELETE` | key | Primary → replica: apply this delete. |
| `0x22` | `REPLICA_ACK` | — | Replica → primary, same `request_id`; `status` = result. |

### Payload encoding

| Element | Encoding | Limit |
|---|---|---|
| key | `u16` length + bytes | 128 bytes |
| value | `u32` length + bytes | 1024 bytes |
| udp_port, tcp_port | `u16` each | — |

Keys and values are arbitrary bytes (not necessarily text). A payload must be
consumed exactly: missing or leftover bytes make the message invalid.

The limits guarantee that the largest `PUT`
(28 + 2 + 128 + 4 + 1024 = **1186 bytes**) fits in one datagram.

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
**same request with the same `request_id`**. Nodes remember recently processed
`(sender_id, request_id)` pairs and answer a retry with the stored result instead
of applying the operation twice (duplicate detection).

**Forwarding.** If the node that receives a request is not the key's owner, it
forwards the request unchanged — same `sender_id` and `request_id` — to the
owner, with the `FORWARDED` flag set, and relays the owner's response back to
the client. A node **never forwards a message that already has `FORWARDED`
set**: if it is not the owner either (for example, because membership changed
a moment ago), it answers `UNAVAILABLE` and the client retries. This prevents
forwarding loops between nodes that briefly disagree about ownership.

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

A peer that misses several consecutive `PONG`s is considered failed.

---

## 7. Versioning

- `version` changes only for incompatible changes; a node rejects other versions.
- New message types can be added within a version; nodes reject types they
  don't know.
- `reserved` and unused `flags` bits are available for future, compatible use.
