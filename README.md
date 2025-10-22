# High-Performance Multithreaded Key-Value Store

A production-grade, multithreaded in-memory key-value store written in modern C++ with crash-safe persistence.

## Architecture

### Core Components

**Sharded Hash Map** - 256-way partitioned storage using reader-writer locks to minimize contention and enable concurrent reads.

**Thread Pool** - Fixed worker pool that handles client connections asynchronously, scaling with available CPU cores.

**Binary Protocol** - Custom wire protocol with fixed 16-byte headers for efficient parsing and minimal overhead.

**AOF Persistence** - Append-only file logging ensures durability with automatic recovery on startup.

**Raw TCP Networking** - POSIX socket-based server with `SO_REUSEADDR` and `TCP_NODELAY` optimizations.

### Concurrency Model

- 256 shards reduce lock contention across different keys
- `std::shared_mutex` per shard allows multiple simultaneous readers
- Thread-per-connection model via thread pool
- Lock-free operations where possible using atomic primitives

## Features

- GET, SET, DELETE operations with microsecond latency
- Crash-safe persistence via AOF logging
- Automatic recovery on startup
- Graceful shutdown with SIGINT/SIGTERM handling
- Configurable port and AOF file location

## Building

```bash
make
```

Requires:
- C++17 compatible compiler
- POSIX-compliant system (Linux, macOS)
- pthread support

## Running

```bash
./kvstore [port] [aof_file]
```

Defaults to port `7878` and AOF file `store.aof`.

## Protocol Specification

### Request Format
```
[Header: 16 bytes]
  - Magic: 4 bytes (0x4B565354)
  - Command: 1 byte (0x01=GET, 0x02=SET, 0x03=DELETE, 0x04=PING)
  - Flags: 1 byte
  - Key Length: 4 bytes
  - Value Length: 4 bytes
  - Sequence ID: 2 bytes
  - Reserved: 4 bytes

[Payload: variable]
  - Key: [key_length] bytes
  - Value: [value_length] bytes (SET only)
```

### Response Format
```
[Header: 16 bytes]
  - Magic: 4 bytes (0x4B565352)
  - Status: 1 byte (0x00=OK, 0x01=NOT_FOUND, 0x02=INVALID, 0x03=ERROR)
  - Flags: 1 byte
  - Data Length: 4 bytes
  - Reserved: 8 bytes
  - Sequence ID: 2 bytes

[Payload: variable]
  - Data: [data_length] bytes (GET responses)
```

## Performance Characteristics

- Concurrent read scalability via sharded reader-writer locks
- Sub-millisecond operation latency for cache hits
- Thread pool prevents connection exhaustion
- AOF write synchronization ensures durability

## Project Structure

```
src/
├── main.cpp           - Server lifecycle and signal handling
├── kv_store.hpp       - Sharded hash map implementation
├── thread_pool.hpp    - Worker pool for connection handling
├── protocol.hpp       - Binary protocol definitions
├── persistence.hpp    - AOF writer and recovery reader
└── networking.hpp     - TCP server and client handler
```

## Technical Highlights

- Zero external dependencies (standard library only)
- RAII-based resource management
- Type-safe enum classes for commands and status codes
- Network byte order conversion for protocol compatibility
- Self-documenting code structure without inline comments
