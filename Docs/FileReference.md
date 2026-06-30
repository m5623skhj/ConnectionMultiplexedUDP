# File Reference

Source documents are grouped by runtime component instead of one document per `.h` or `.cpp` file.

## Entry And Test

- [EntryAndSmokeTest.md](Files/EntryAndSmokeTest.md)
  - `main.cpp`
  - `SmokeTest.h`
  - `SmokeTest.cpp`

## Public Facade And Client Layer

- [ServerCore.md](Files/ServerCore.md)
  - `ServerCore.h`
  - `ServerCore.cpp`
- [Client.md](Files/Client.md)
  - `Client.h`
  - `Client.cpp`
  - `ClientPacketSender.h`
- [ClientManager.md](Files/ClientManager.md)
  - `ClientManager.h`
  - `ClientManager.cpp`

## Session Layer

- [Session.md](Files/Session.md)
  - `ConnectionId.h`
  - `Session.h`
  - `Session.cpp`
- [SessionLookupTable.md](Files/SessionLookupTable.md)
  - `SessionLookupTable.h`
  - `SessionLookupTable.cpp`

## Processor Layer

- [ProcessorBase.md](Files/ProcessorBase.md)
  - `ProcessorBase.h`
  - `ProcessorBase.cpp`
  - `ProcessorType.h`
  - `ProcessorTaskBase.h`
- [ProcessorManager.md](Files/ProcessorManager.md)
  - `ProcessorManager.h`
  - `ProcessorManager.cpp`
- [IOProcessor.md](Files/IOProcessor.md)
  - `IOProcessor.h`
  - `IOProcessor.cpp`
- [LogicProcessor.md](Files/LogicProcessor.md)
  - `LogicProcessor.h`
  - `LogicProcessor.cpp`
- [HeartbeatProcessor.md](Files/HeartbeatProcessor.md)
  - `HeartbeatProcessor.h`
  - `HeartbeatProcessor.cpp`
- [TickerProcessor.md](Files/TickerProcessor.md)
  - `TickerProcessor.h`
  - `TickerProcessor.cpp`
  - `TickerTask.h`
- [ProcessorTasks.md](Files/ProcessorTasks.md)
  - `ReceivedPacketTask.h`
  - `SendPacketTask.h`
  - `ClientDisconnectTask.h`

## Protocol Layer

- [Protocol.md](Files/Protocol.md)
  - `Protocol/PacketProtocol.h`
  - `Protocol/PacketEnvelope.proto`
  - `Protocol/PacketAuthentication.h`
  - `Protocol/PacketAuthentication.cpp`
  - generated protobuf files

## Excluded Files

The following are build or IDE metadata and are not documented separately:

- `ConnectionMultiplexedUDP.slnx`
- `vcpkg.json`
- `ConnectionMultiplexedUDP.vcxproj`
- `ConnectionMultiplexedUDP.vcxproj.filters`
- `ConnectionMultiplexedUDP.vcxproj.user`
