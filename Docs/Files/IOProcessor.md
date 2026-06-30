# IOProcessor

Covered files:

- `ConnectionMultiplexedUDP/ConnectionMultiplexedUDP/IOProcessor.h`
- `ConnectionMultiplexedUDP/ConnectionMultiplexedUDP/IOProcessor.cpp`

## Role

`IOProcessor` owns one UDP socket. Its processor thread receives datagrams, and its task thread sends queued packets.

## Receive Flow

```mermaid
sequenceDiagram
    participant Socket as UDP socket
    participant IO as IOProcessor processorThread
    participant PM as ProcessorManager
    participant Logic as LogicProcessor queue

    Socket->>IO: recvfrom()
    IO->>PM: PushTaskToProcessorByAffinity(Logic, ReceivedPacketTask, endpoint key)
    PM->>Logic: enqueue task
```

## Send Flow

```mermaid
sequenceDiagram
    participant PM as ProcessorManager
    participant IO as IOProcessor taskThread
    participant Socket as UDP socket

    PM->>IO: SendPacketTask
    IO->>Socket: sendto()
```

## Important Behavior

- Uses receive timeout behavior through `SO_RCVTIMEO`.
- Computes endpoint affinity from sender IP and port.
- Converts received datagrams into `ReceivedPacketTask`.
- Processes `SendPacketTask` for outbound UDP sends.

## Threading Notes

The UDP socket handle is shared by receive and send paths, so socket access is protected by `socketMutex`.
