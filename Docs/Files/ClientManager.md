# ClientManager

Covered files:

- `ConnectionMultiplexedUDP/ConnectionMultiplexedUDP/ClientManager.h`
- `ConnectionMultiplexedUDP/ConnectionMultiplexedUDP/ClientManager.cpp`

## Role

`ClientManager` owns registered `Client` objects and dispatches received application packets to the correct client instance.

## Data Model

```mermaid
flowchart LR
    Clients["unordered_map<ClientId, ClientEntry>"] --> Entry["ClientEntry"]
    Entry --> Client["unique_ptr<Client>"]
    Entry --> Visiting["visit count"]
    Entry --> Removing["removal flag"]
```

## Main Responsibilities

- Allocate client ids.
- Store client ownership.
- Dispatch packets to `Client::OnRecvPacket()`.
- Remove clients without destroying an object while a callback is visiting it.

## Removal Flow

```mermaid
sequenceDiagram
    participant Caller
    participant CM as ClientManager
    participant Entry as ClientEntry
    participant Client

    Caller->>CM: RemoveClient(clientId)
    CM->>Entry: mark removing
    alt no active visit
        CM->>Client: destroy outside map
    else active visit exists
        CM->>Entry: wait for visit count to reach zero
        CM->>Client: destroy after callback exits
    end
```

## Threading Notes

- `clientsMutex` protects the client map.
- Each `ClientEntry` has its own mutex and condition variable for callback/removal coordination.
- Callbacks are invoked without holding the global client map mutex.
