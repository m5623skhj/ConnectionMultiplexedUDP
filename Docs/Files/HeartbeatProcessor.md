# HeartbeatProcessor

Covered files:

- `ConnectionMultiplexedUDP/ConnectionMultiplexedUDP/HeartbeatProcessor.h`
- `ConnectionMultiplexedUDP/ConnectionMultiplexedUDP/HeartbeatProcessor.cpp`

## Role

`HeartbeatProcessor` periodically scans active sessions and requests disconnect processing for sessions that have not received packets within the configured timeout.

## Timeout Flow

```mermaid
sequenceDiagram
    participant HB as HeartbeatProcessor
    participant PM as ProcessorManager
    participant LT as SessionLookupTable
    participant Logic as LogicProcessor

    HB->>PM: RequestTimedOutSessionDisconnects(timeout)
    PM->>LT: SnapshotActiveSessions()
    PM->>Logic: ClientDisconnectTask(connectionId, requestedTime, timeout)
    Logic->>PM: Remove session if timeout still applies
```

## Important Behavior

- Scan interval is clamped between 100ms and 1000ms.
- Timeout removal is requested through a task instead of removing directly from the heartbeat thread.
- The disconnect task revalidates timeout state before removing a session.

## Threading Notes

Heartbeat scanning runs on the processor thread. The common task thread exists through `ProcessorBase`, but this processor currently has no meaningful task handling.
