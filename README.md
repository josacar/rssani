# rssani

[![CI](https://github.com/josacar/rssani/actions/workflows/ci.yml/badge.svg)](https://github.com/josacar/rssani/actions/workflows/ci.yml)

Headless C++ Qt6 console application that monitors RSS feeds and IRC channels from XBT/XBTT torrent trackers, downloads matching `.torrent` files based on user-defined regexps, and optionally sends email notifications. Exposes a **gRPC API** for remote management.

Originally created in 2004 for the Animersion community. Tested on Linux.

## Architecture

| File | Description |
|---|---|
| `main.cpp` | Entry point, creates `rssani_lite` + gRPC server |
| `rssani_lite.cpp/h` | Core app: settings, regexp management, IRC integration, POSIX signal handling |
| `rss_lite.cpp/h` | RSS fetching, XML parsing, torrent downloading, regexp matching |
| `myircsession.cpp/h` | IRC client (libirc/grumpy-irc) monitoring channels for new uploads |
| `grpc_server.cpp/h` | gRPC server (grpc++) exposing management API on port 50051 |
| `rssani.proto` | Protocol Buffers service definition (20 RPC methods) |
| `mailsender.cpp/h` | SMTP email sender with SSL/TLS support |
| `values.h` | Configuration value object (paths, mail, debug flag) |

## Requirements

- Qt6 (Core, Network)
- gRPC++ (`libgrpc++-dev`)
- Protobuf (`libprotobuf-dev`, `protobuf-compiler`, `protobuf-compiler-grpc`)
- [libirc](https://github.com/grumpy-irc/libirc) (grumpy-irc, built automatically)
- A C++ compiler with C++20 support
- CMake ≥ 3.14

## Build

```bash
mkdir build && cd build
cmake ..
make
```

### Using mise (Recommended)

This project uses [mise](https://mise.jdx.dev/) to manage common development tasks:

```bash
# Build the project
mise run build

# Run all unit tests
mise run test

# Run integration tests
mise run test:integration

# Run all tests
mise run test:all

# Run the application
mise run run

# Format code
mise run format

# Run linting checks
mise run lint
```

See all available tasks with `mise tasks ls`.

### Docker

```bash
# Unit tests (55 tests)
podman-compose run unit-tests

# Integration tests (14 gRPC tests)
podman-compose run integration-tests
```

The integration test image inherits from the unit test image — the C++ binary is built once and reused. Only Python gRPC stubs are generated at test time.

## CI/CD

GitHub Actions (`.github/workflows/ci.yml`) runs on every push and PR to `master`. It builds the project and runs all unit tests and CTest in a `debian:trixie-slim` container.

## Testing

### Unit Tests (C++ / Qt Test)
55 unit tests covering `Values`, `MailSender`, `Rss_lite`, `rssani_lite`, and `MyIrcSession`.

```bash
podman-compose run unit-tests
```

Or run all via CTest:
```bash
ctest --test-dir build --output-on-failure
```

### Integration Tests (Python / gRPC)
14 integration tests covering the gRPC API (regexp CRUD, auth CRUD, options, timer, log, save, shutdown).

```bash
python3 tests/integration/test_grpc.py ./build/rssani
```

## gRPC API

The server listens on `0.0.0.0:50051`. See `rssani.proto` for the full service definition. Key methods:

| Method | Description |
|---|---|
| `VerUltimo` | Timestamp of last RSS fetch |
| `VerTimer` | Remaining timer interval (ms) |
| `VerLog` | Log entries (paginated) |
| `ListaExpresiones` | List all regexp rules |
| `ListaAuths` | List all tracker auth entries |
| `VerOpciones` | General settings |
| `AnadirRegexp` / `EditarRegexp` / `BorrarRegexp*` | Regexp CRUD |
| `AnadirAuth` / `BorrarAuth` | Tracker auth CRUD |
| `PonerCredenciales` | Set RPC credentials |
| `PonerOpciones` | Set general options |
| `CambiaTimer` | Change timer interval |
| `Guardar` | Save configuration |
| `Shutdown` | Graceful shutdown |

## Configuration

Settings are stored via `QSettings` (INI-style) in the standard Qt config path. Configurable options include:

- Tracker URLs and credentials
- IRC server and channel
- SMTP server, login, and password
- Regexp rules for torrent matching

Logs (`rssani.log`, `matches.log`) are written to the config directory.
