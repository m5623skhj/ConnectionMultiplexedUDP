# Protocol

Covered files:

- `ConnectionMultiplexedUDP/ConnectionMultiplexedUDP/Protocol/PacketProtocol.h`
- `ConnectionMultiplexedUDP/ConnectionMultiplexedUDP/Protocol/PacketEnvelope.proto`
- `ConnectionMultiplexedUDP/ConnectionMultiplexedUDP/Protocol/PacketAuthentication.h`
- `ConnectionMultiplexedUDP/ConnectionMultiplexedUDP/Protocol/PacketAuthentication.cpp`
- generated protobuf files under `ConnectionMultiplexedUDP/ConnectionMultiplexedUDP/Generated/`

## Role

The protocol layer defines packet constants, protobuf wire structures, and HMAC-SHA256 packet authentication helpers.

## Packet Shape

```mermaid
flowchart LR
    Datagram["UDP datagram"] --> Envelope["PacketEnvelope"]
    Envelope --> Connection["connection_id"]
    Envelope --> Data["authenticated_data"]
    Envelope --> Tag["authentication_tag"]
    Data --> Packet["AuthenticatedPacket"]
    Packet --> Version["protocol_version"]
    Packet --> PacketConnection["connection_id"]
    Packet --> Sequence["sequence"]
    Packet --> PacketType["packet_type"]
    Packet --> Payload["payload"]
```

## Constants

| Constant | Meaning |
| --- | --- |
| `CURRENT_PROTOCOL_VERSION` | Current packet protocol version. |
| `MAX_PACKET_SIZE` | Maximum accepted packet size. |
| `AUTHENTICATION_KEY_SIZE` | HMAC key size in bytes. |
| `AUTHENTICATION_TAG_SIZE` | HMAC-SHA256 tag size in bytes. |
| `HEARTBEAT_PACKET_TYPE` | Internal heartbeat control packet. |
| `DISCONNECT_PACKET_TYPE` | Internal disconnect control packet. |
| `FIRST_APPLICATION_PACKET_TYPE` | First packet type forwarded to clients. |

## Authentication Flow

```mermaid
sequenceDiagram
    participant Sender
    participant Auth as PacketAuthentication
    participant Wire
    participant Receiver

    Sender->>Auth: SerializeAuthenticatedPacket(...)
    Auth->>Auth: serialize AuthenticatedPacket
    Auth->>Auth: HMAC-SHA256(authenticated_data, key)
    Auth->>Wire: PacketEnvelope(data, tag)
    Wire->>Receiver: UDP datagram
    Receiver->>Auth: VerifyAuthenticationTag(key, data, tag)
```

## Generated Files

Generated protobuf files are treated as protocol artifacts, not handwritten source. They should be regenerated from `PacketEnvelope.proto` when the protobuf schema changes.
