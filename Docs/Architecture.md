# Architecture

이 문서는 ConnectionMultiplexedUDP의 전체 구조, packet 흐름, session lifecycle, thread model만 설명합니다.

개별 파일의 역할은 [FileReference.md](FileReference.md)와 `Docs/Files/` 아래 파일별 문서를 봅니다.

## High-Level Structure

```mermaid
flowchart TB
    Main["main.cpp"] --> ServerCore["ServerCore"]
    ServerCore --> ClientManager["ClientManager"]
    ServerCore --> ProcessorManager["ProcessorManager"]
    ServerCore -. "implements" .-> ClientPacketSender["ClientPacketSender"]

    ClientManager --> Client["Client subclass"]
    Client --> ClientPacketSender

    ProcessorManager --> SessionLookupTable["SessionLookupTable"]
    SessionLookupTable --> Session["Session"]

    ProcessorManager --> IOGroup["IOProcessor group"]
    ProcessorManager --> LogicGroup["LogicProcessor group"]
    ProcessorManager --> Ticker["TickerProcessor"]
    ProcessorManager --> Heartbeat["HeartbeatProcessor"]

    IOGroup --> UDP["UDP sockets"]
    LogicGroup --> ClientManager
```

`ServerCore` is the facade. It owns:

- client lifetime through `ClientManager`
- processor lifetime through `ProcessorManager`
- WSA startup/cleanup
- public session registration/removal API

`ProcessorManager` is the coordinator. It owns:

- processor groups
- session lookup table
- authenticated inbound dispatch
- authenticated outbound send path
- timeout/disconnect task scheduling

## Packet Model

UDP datagrams use a two-layer protobuf structure.

```mermaid
flowchart LR
    UDP["UDP Datagram"] --> Envelope["PacketEnvelope"]
    Envelope --> ConnectionId["connection_id"]
    Envelope --> AuthData["authenticated_data bytes"]
    Envelope --> AuthTag["authentication_tag bytes"]
    AuthData --> Packet["AuthenticatedPacket"]
    Packet --> Version["protocol_version"]
    Packet --> PacketConnection["connection_id"]
    Packet --> Sequence["sequence"]
    Packet --> PacketType["packet_type"]
    Packet --> Payload["payload bytes"]
```

Authentication is computed over serialized `AuthenticatedPacket`.

```mermaid
sequenceDiagram
    participant Sender
    participant Auth as PacketAuthentication
    participant Wire as UDP Wire
    participant Receiver

    Sender->>Auth: SerializeAuthenticatedPacket(connection, sequence, type, payload, key)
    Auth->>Auth: Serialize AuthenticatedPacket
    Auth->>Auth: HMAC-SHA256(authenticated_data, key)
    Auth->>Wire: PacketEnvelope(connection_id, authenticated_data, tag)
    Wire->>Receiver: UDP datagram
    Receiver->>Auth: VerifyAuthenticationTag(key, authenticated_data, tag)
```

Reserved packet types:

- `HEARTBEAT_PACKET_TYPE = 1`
- `DISCONNECT_PACKET_TYPE = 2`
- application packet types start at `FIRST_APPLICATION_PACKET_TYPE = 1024`

## Inbound Packet Flow

```mermaid
sequenceDiagram
    participant Socket as IOProcessor Socket Thread
    participant IOQ as IOProcessor
    participant PM as ProcessorManager
    participant Logic as LogicProcessor Task Thread
    participant Session as Session
    participant CM as ClientManager
    participant Client

    Socket->>Socket: recvfrom()
    Socket->>PM: PushTaskToProcessorByAffinity(Logic, ReceivedPacketTask, endpoint key)
    PM->>Logic: queue task
    Logic->>Logic: Parse PacketEnvelope
    Logic->>PM: AuthenticateAndDispatchPacket(sender, connection, data, tag)
    PM->>Session: Find connection
    PM->>Session: Verify endpoint and authentication tag
    PM->>Session: AcceptSequence(sequence)
    PM->>Session: MarkReceivedNow()
    alt heartbeat packet
        PM-->>Logic: handled internally
    else disconnect packet
        PM->>PM: RequestClientDisconnect(connection)
    else application packet
        PM->>CM: DispatchPacket(clientId, packetType, payload)
        CM->>Client: OnRecvPacket(packetType, payload)
    end
```

Inbound affinity is based on sender endpoint:

```cpp
(addr.sin_addr.s_addr << 16) | addr.sin_port
```

That keeps packets from the same UDP endpoint biased toward the same Logic processor.

## Outbound Packet Flow

```mermaid
sequenceDiagram
    participant App as Client subclass
    participant Client
    participant Sender as ClientPacketSender(ServerCore)
    participant PM as ProcessorManager
    participant Session
    participant IO as IOProcessor Task Thread
    participant Socket as UDP Socket

    App->>Client: SendPacket(connectionId, packetType, payload)
    Client->>Sender: SendPacket(clientId, connectionId, packetType, payload)
    Sender->>PM: SendAuthenticatedPacket(clientId, connectionId, packetType, payload)
    PM->>Session: Find connection and verify owner clientId
    PM->>Session: BuildSendPacket()
    Session->>Session: assign send sequence
    Session->>Session: serialize + HMAC
    PM->>IO: SendPacketTask(destination, packetData)
    IO->>Socket: sendto()
```

The `Client` layer does not know about `ProcessorManager` directly. `ClientPacketSender` decouples application-level client code from server internals.

## Session Lifecycle

```mermaid
stateDiagram-v2
    [*] --> Allocated: RegisterClientSession()
    Allocated --> Active: valid authenticated packet
    Active --> Active: heartbeat packet
    Active --> Active: application packet
    Active --> DisconnectRequested: disconnect packet
    Active --> TimeoutRequested: HeartbeatProcessor timeout scan
    TimeoutRequested --> Active: timeout validation fails
    TimeoutRequested --> Removed: timeout validation passes
    DisconnectRequested --> Removed: ClientDisconnectTask
    Active --> Removed: RemoveClientSession() / RemoveClient()
    Removed --> [*]
```

Session identity uses slot index plus generation.

```mermaid
flowchart LR
    ConnectionId["ConnectionId uint64"] --> Generation["upper 32 bits: generation"]
    ConnectionId --> Index["lower 32 bits: slot index"]
    Index --> Slot["SessionSlot"]
    Generation --> StaleCheck["stale id rejection"]
```

When a session is released, its slot generation is incremented. Old connection ids targeting the same slot fail generation checks.

## Thread Model

Each `ProcessorBase` instance can have up to two worker threads:

- `processorThread`
  - optional long-running thread started by `StartProcessorThread()`
  - examples: IO receive loop, heartbeat scan loop, ticker loop
- `processTaskThread`
  - common task queue worker
  - created for every started processor after `StartImpl()` succeeds

```mermaid
flowchart TB
    subgraph ProcessorBaseInstance["ProcessorBase instance"]
        Queue["taskQueue + condition_variable"]
        TaskThread["processTaskThread"]
        ProcThread["processorThread optional"]
        Queue --> TaskThread
        TaskThread --> ProcessTask["virtual ProcessTask(task)"]
        ProcThread --> LongRun["processor-specific loop"]
    end
```

### IOProcessor Threads

For each IO processor:

- processor thread
  - owns receive loop
  - calls `recvfrom()`
  - converts datagrams to `ReceivedPacketTask`
- task thread
  - processes `SendPacketTask`
  - calls `sendto()`

Socket access is protected by `socketMutex`.

```mermaid
flowchart LR
    RecvThread["IO processorThread recvfrom"] --> RTask["ReceivedPacketTask"]
    RTask --> LogicQueue["Logic queue"]
    LogicQueue --> LogicTaskThread["Logic processTaskThread"]

    SendCaller["ProcessorManager SendPacket"] --> STask["SendPacketTask"]
    STask --> IOQueue["IO queue"]
    IOQueue --> IOTaskThread["IO processTaskThread"]
    IOTaskThread --> SendTo["sendto"]
```

### LogicProcessor Threads

Logic processor currently relies on the common task thread.

It processes:

- `ReceivedPacketTask`
- `ClientDisconnectTask`

It does not start a separate long-running processor thread.

### HeartbeatProcessor Threads

Heartbeat processor uses:

- processor thread
  - sleeps for computed scan interval
  - requests timeout disconnects
- task thread
  - currently unused

Scan interval is computed from session timeout:

- lower bound: 100ms
- upper bound: 1000ms
- target: timeout / 2

### TickerProcessor Threads

Ticker processor uses:

- processor thread
  - wakes by tick interval
  - fires due callbacks
- task thread
  - receives and queues `TickerTask`

Ticker is available as a scheduler foundation but is not yet central to the session flow.

## Lifecycle Ordering

Start order:

```mermaid
flowchart LR
    Logic --> Ticker --> Heartbeat --> IO
```

Rationale:

- Logic should be ready before IO starts receiving datagrams.
- Heartbeat should start before IO accepts traffic so session timeout maintenance is available.

Stop order:

```mermaid
flowchart LR
    IO --> Ticker --> Heartbeat --> Logic
```

Rationale:

- Stop IO first to stop new inbound network traffic.
- Stop background schedulers next.
- Stop Logic last so queued disconnect/received tasks have the best chance to drain.

## Synchronization Summary

| Component | Protected State | Mechanism |
| --- | --- | --- |
| `ServerCore` | lifecycle state | `lifecycleMutex` |
| `ServerCore` | client/session external operations | `clientSessionMutex` |
| `ClientManager` | client map | `clientsMutex` |
| `ClientManager::ClientEntry` | one-client visit/removal state | entry mutex + condition variable |
| `ProcessorBase` | processor lifecycle | `lifecycleMutex`, atomic state |
| `ProcessorBase` | task queue | `messageQueueMutex`, condition variable |
| `IOProcessor` | UDP socket handle | `socketMutex` |
| `Session` | replay window | `replayMutex` |
| `Session` | last received time | `activityMutex` |
| `Session` | send sequence | `sendMutex` |
| `SessionLookupTable` | session slots/free indices | `slotsMutex` |
| `TickerProcessor` | scheduled task queue | `tickerTaskQueueMutex` |

## Control Packet Handling

```mermaid
flowchart TD
    Packet["AuthenticatedPacket"] --> Type{"packet_type"}
    Type -->|HEARTBEAT_PACKET_TYPE| Heartbeat["MarkReceivedNow and return"]
    Type -->|DISCONNECT_PACKET_TYPE| Disconnect["RequestClientDisconnect"]
    Type -->|Application type| Dispatch["ClientManager::DispatchPacket"]
```

Control packets are consumed internally and are not forwarded to `Client::OnRecvPacket()`.

## Smoke Test Coverage

`ConnectionMultiplexedUDP.exe --smoke-test` covers:

```mermaid
flowchart TD
    Start["Start ServerCore"] --> RegisterClient["Add dummy Client"]
    RegisterClient --> RegisterSession["Register session"]
    RegisterSession --> C2S["Client socket sends authenticated packet"]
    C2S --> OnRecv["Client::OnRecvPacket observed"]
    OnRecv --> S2C["Client::SendPacket"]
    S2C --> ClientRecv["Client socket receives authenticated packet"]
    ClientRecv --> Disconnect["Send disconnect control packet"]
    Disconnect --> VerifyDisconnect["Send via removed connection fails"]
    VerifyDisconnect --> TimeoutSession["Register timeout session"]
    TimeoutSession --> VerifyTimeout["Heartbeat removes timed-out session"]
    VerifyTimeout --> Stop["Stop ServerCore"]
```

This is an integration smoke test, not a full unit test suite.

## Known Gaps

- No explicit handshake/key exchange protocol exists yet.
- Session registration is currently server API driven.
- Ticker infrastructure exists but is not integrated into session management.
- Runtime diagnostics are mostly `std::cout`/`std::cerr`; structured logging does not exist yet.
