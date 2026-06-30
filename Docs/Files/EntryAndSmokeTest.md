# Entry And Smoke Test

Covered files:

- `ConnectionMultiplexedUDP/ConnectionMultiplexedUDP/main.cpp`
- `ConnectionMultiplexedUDP/ConnectionMultiplexedUDP/SmokeTest.h`
- `ConnectionMultiplexedUDP/ConnectionMultiplexedUDP/SmokeTest.cpp`

## Role

`main.cpp` is the executable entry point. It either runs the smoke test when `--smoke-test` is passed, or starts a simple server runtime.

`SmokeTest` is an integration-level sanity check for the implemented runtime path. It is not a full unit test suite.

## Smoke Test Flow

```mermaid
flowchart TD
    Start["Run executable with --smoke-test"] --> Server["Create and start ServerCore"]
    Server --> Client["Register SmokeTestClient"]
    Client --> Session["Register client session"]
    Session --> Inbound["Client socket sends authenticated packet"]
    Inbound --> Callback["SmokeTestClient::OnRecvPacket observed"]
    Callback --> Outbound["Client::SendPacket"]
    Outbound --> Receive["External socket receives server packet"]
    Receive --> Disconnect["Send disconnect control packet"]
    Disconnect --> Removed["Verify removed connection cannot send"]
    Removed --> Timeout["Register idle timeout session"]
    Timeout --> Done["Verify heartbeat timeout removes session"]
```

## Important Behavior

- Uses localhost UDP sockets.
- Builds authenticated packets with the same protocol path used by the server.
- Verifies inbound dispatch, outbound send, disconnect handling, and heartbeat timeout removal.

## Threading Notes

The smoke test starts the real processor threads through `ServerCore`. Test waiting is timeout-based, so failures should terminate instead of hanging indefinitely.
