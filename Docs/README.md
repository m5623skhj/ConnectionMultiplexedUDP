# ConnectionMultiplexedUDP Docs

This folder documents the current codebase at a component level.

## Documents

- [Architecture.md](Architecture.md)
  - Overall structure, packet flow, session lifecycle, and thread model.
- [FileReference.md](FileReference.md)
  - Index of component-level source documents.
- [Files/](Files/)
  - Component documents grouped by runtime responsibility.

## Current Scope

- UDP receive/send processing
- protobuf packet envelope
- HMAC-SHA256 authentication tag
- per-session replay protection
- client callback dispatch
- client-initiated send path
- heartbeat timeout disconnect
- disconnect control packet
- `--smoke-test` integration smoke test

Project/build files such as `.vcxproj`, `.filters`, `.user`, `.slnx`, and `vcpkg.json` are intentionally not documented as separate files.
