# AGENTS.md

## Project Overview

rssani is a headless C++ Qt6 console application that monitors RSS feeds and IRC channels from XBT/XBTT torrent trackers, downloads matching `.torrent` files based on user-defined regexps, and optionally sends email notifications. It exposes a **gRPC API** for remote management. Started in 2004, originally targeting the Animersion community.

## Architecture

```
main.cpp               → Entry point, creates rssani_lite + gRPC server
rssani_lite.cpp/h      → Core app: settings, regexp management, IRC integration, signal wiring
rss_lite.cpp/h         → RSS fetching, XML parsing, torrent downloading, regexp matching
myircsession.cpp/h     → IRC client (libirc/grumpy-irc) monitoring channel for new uploads
grpc_server.cpp/h      → gRPC server (grpc++) exposing management API on port 50051
rssani.proto           → Protocol Buffers service definition (19 RPC methods)
mailsender.cpp/h       → SMTP email sender with SSL/TLS support
values.h               → Configuration value object (paths, mail, debug flag)
```

## Build

gRPC and Protobuf must be installed as system packages (`libgrpc++-dev`, `libprotobuf-dev`, `protobuf-compiler`, `protobuf-compiler-grpc`). libirc (grumpy-irc) is fetched and built automatically via CMake `ExternalProject`:

```bash
mkdir build && cd build
cmake ..
make
```

The `rssani.proto` file is compiled automatically by CMake into `rssani.pb.cc/h` and `rssani.grpc.pb.cc/h`.

## Agent Instructions

- After every code change, update `AGENTS.md`, `README.md`, and `TODO.md` to reflect the new state of the project.
- After modifying code or tests, run `podman-compose run unit-tests` to verify all unit tests pass, then `podman-compose run integration-tests` for gRPC integration tests.

## Key Conventions

- Language: C++20 with Qt6. Comments and identifiers are mostly in Spanish.
- Configuration: `QSettings` (INI-style), stored in the standard Qt config path.
- Logging: `QFile`-based plain text logs (`rssani.log`, `matches.log`) in the config directory.
- Signal/slot: Qt5 pointer-to-member syntax (`&Class::method`) throughout.
- Memory management: `std::unique_ptr` and `std::shared_ptr` used for owned objects.
- Thread safety: `rssani_lite` public methods are protected by `QMutex` for safe access from the gRPC thread.
- No CI/CD.

## Docker

The project uses a **multi-stage Dockerfile** with separate stages for testing and release builds:

| Stage | Target | Image | Purpose |
|---|---|---|---|
| `test-builder` | (internal) | — | Builds all source code + test executables |
| `test` | `test` | `rssani-tests` | Runs all 5 unit test executables |
| `release-builder` | (internal) | — | Builds only the rssani binary (no tests) via `-DRSSANI_BUILD_TESTS=OFF` |
| `release` | `release` | `rssani-release` | Minimal runtime image with only the rssani binary and libirc `.so` files (via `LD_LIBRARY_PATH=/app/lib`) |

The `test` stage copies test sources and builds everything. The `release` stage builds only the `rssani` binary using `-DRSSANI_BUILD_TESTS=OFF` to skip test compilation.

The integration test image (`Dockerfile.integration`) **does not rebuild** the C++ binary — it reuses the pre-built one from `rssani-tests`, only installing the Python gRPC runtime and generating Python stubs from `rssani.proto`.

Run:
```bash
podman-compose run unit-tests
podman-compose run integration-tests
podman-compose run release
```

Or build the release image specifically:
```bash
podman-compose build release
```

Or use mise tasks (recommended):
```bash
mise run test:docker:unit
mise run test:docker:integration
mise run test:docker:all
```

### mise Tasks

The project uses [mise](https://mise.jdx.dev/) for common development tasks. All tasks are defined in `mise.toml`:

| Task | Description |
|---|---|
| `build` | Build the project |
| `clean` | Clean build artifacts |
| `test` | Run all unit tests via CTest |
| `test:unit` | Run all C++ unit tests individually |
| `test:unit:values` | Run Values class unit tests |
| `test:unit:mail` | Run MailSender unit tests |
| `test:unit:rss` | Run Rss_lite unit tests |
| `test:unit:rssani` | Run rssani_lite unit tests |
| `test:unit:irc` | Run MyIrcSession unit tests |
| `test:integration` | Run gRPC integration tests |
| `test:all` | Run all tests (unit + integration) |
| `test:docker:unit` | Run unit tests in Docker (podman) |
| `test:docker:integration` | Run integration tests in Docker (podman) |
| `test:docker:all` | Run all tests in Docker |
| `run` | Run the rssani binary |
| `proto` | Regenerate gRPC/Protobuf stubs manually |
| `lint` | Run clang-tidy for code quality checks |
| `format` | Format C++ code with clang-format |
| `docs:update` | Reminder to update docs after code changes |
| `docker:build` | Build Docker test images |
| `dev` | Quick rebuild and run during development |

Run `mise tasks ls` to see all available tasks.

## Testing

### Unit Tests (Qt Test / C++)
Unit tests are in `tests/` and built as separate executables (`rssani_tests_values`, `rssani_tests_mail`, `rssani_tests_rss`, `rssani_tests_rssani`, `rssani_tests_irc`). Built via CMake alongside the main binary.

Run via Docker:
```bash
podman-compose run unit-tests
```

Or run individual executables from the build directory:
```bash
./build/rssani_tests_values
./build/rssani_tests_mail
./build/rssani_tests_rss
./build/rssani_tests_rssani
./build/rssani_tests_irc
```

Or run all tests via CTest:
```bash
ctest --test-dir build
```

**Test coverage:**
- `test_values.cpp` — `Values` class (defaults, setters, SMTP settings, filledValues)
- `test_mail_sender.cpp` — `MailSender` class (constructor, setters, content type, priority, encoding)
- `test_rss_lite.cpp` — `Rss_lite` class (saveLog, miraTitulo, verUltimo)
- `test_rssani_lite.cpp` — `rssani_lite` class (regexp CRUD, auth CRUD, timer, settings, RPC credentials, debug)
- `test_myirc_session.cpp` — `MyIrcSession` class (datosIrc struct, IRC color stripping)

### Integration Tests (Python / gRPC)
Integration tests for the gRPC interface are in `tests/integration/test_grpc.py` (Python 3, uses `grpcio`):

```bash
python3 tests/integration/test_grpc.py ./build/rssani
```

The script starts the binary, waits for the gRPC server on port 50051, runs 14 tests covering all RPC methods (regexp CRUD, auth CRUD, options, timer, log, save, shutdown), then shuts down.

Python stubs (`rssani_pb2.py`, `rssani_pb2_grpc.py`) are generated from `rssani.proto` by `grpc_tools.protoc`.

## gRPC API

All methods are defined in `rssani.proto` under the `rssani.RssaniService` service, listening on `0.0.0.0:50051`.

| Method | Request | Response | Description |
|---|---|---|---|
| `VerUltimo` | `EmptyRequest` | `StringResponse` | Timestamp of last RSS fetch |
| `VerTimer` | `EmptyRequest` | `IntResponse` | Remaining timer interval in ms |
| `VerLog` | `LogRequest{ini, fin}` | `LogResponse{lines[]}` | Log entries (reversed, paginated) |
| `ListaExpresiones` | `EmptyRequest` | `RegexpListResponse{entries[]}` | All regexp rules |
| `ListaAuths` | `EmptyRequest` | `AuthListResponse{entries[]}` | All tracker auth entries |
| `VerOpciones` | `EmptyRequest` | `OpcionesResponse{fromMail, toMail, path}` | General settings |
| `AnadirRegexp` | `AnadirRegexpRequest{nombre, fecha, mail, tracker, dias}` | `BoolResponse` | Add regexp rule |
| `EditarRegexp` | `EditarRegexpRequest{regexpOrig, regexpDest}` | `BoolResponse` | Edit regexp by pattern |
| `EditarRegexpI` | `EditarRegexpIRequest{regexpOrig, regexpDest}` | `BoolResponse` | Edit regexp by index |
| `ActivarRegexp` | `ActivarRegexpRequest{regexpOrig}` | `BoolResponse` | Toggle regexp active state |
| `MoverRegexp` | `MoverRegexpRequest{from_position, to}` | `BoolResponse` | Move regexp in list |
| `BorrarRegexpI` | `BorrarRegexpIRequest{pos}` | `BoolResponse` | Delete regexp by index |
| `BorrarRegexpS` | `BorrarRegexpSRequest{nombre}` | `BoolResponse` | Delete regexp by pattern |
| `AnadirAuth` | `AnadirAuthRequest{tracker, uid, password, passkey}` | `BoolResponse` | Add tracker auth |
| `BorrarAuth` | `BorrarAuthRequest{tracker}` | `BoolResponse` | Delete tracker auth |
| `PonerCredenciales` | `PonerCredencialesRequest{user, password}` | `BoolResponse` | Set RPC credentials |
| `PonerOpciones` | `PonerOpcionesRequest{fromMail, toMail, path}` | `BoolResponse` | Set general options |
| `CambiaTimer` | `CambiaTimerRequest{tiempo}` | `BoolResponse` | Change timer interval (minutes) |
| `Guardar` | `EmptyRequest` | `BoolResponse` | Save configuration |
| `Shutdown` | `EmptyRequest` | `BoolResponse` | Graceful shutdown |

## Important Caveats

- IRC server/channel and tracker configuration are read from `QSettings`, with hardcoded defaults (`irc.irc-hispano.org`, `#PuntoTorrent`) as fallbacks.
- `rss` and `session` in `rssani_lite` constructor are still raw `new` (Qt parent-child ownership).

## File-by-File Notes

| File | Notes |
|---|---|
| `main.cpp` | Entry point. Constructs `rssani_lite`, creates and starts `GrpcServer`. |
| `rss_lite.cpp` | RSS/torrent logic. Trackers loaded from settings via `iniciaTrackers()`. Uses reply URL as download key for concurrent torrent downloads. Downloads are written to disk in `readDataTorrent()` and logged via `saveLog()`. |
| `rssani_lite.cpp` | Settings I/O, regexp CRUD, signal wiring. All public methods mutex-protected. Uses `QRegularExpression`. POSIX signal handling uses self-pipe trick with `QSocketNotifier` to avoid deadlocks. |
| `myircsession.cpp` | IRC client. Uses `QRandomGenerator`. Connects to `libircclient::Network` signals (`Event_PRIVMSG`, `Event_Connected`, `Event_SelfKick`, `Event_MOTDEnd`). |
| `mailsender.cpp` | SMTP sender. Credentials read from `Values`. Uses `QRandomGenerator`. |
| `grpc_server.cpp` | gRPC service implementation + `GrpcServer` class. Runs on port 50051. Each RPC method delegates to `rssani_lite` through its public API. Shutdown calls `rss->salir()`. |
| `rssani.proto` | Protocol Buffers v3 service definition. 20 RPC methods with typed request/response messages. |
| `CMakeLists.txt` | Uses `ExternalProject_Add` for libirc (grumpy-irc). gRPC and Protobuf linked via pkg-config (`grpc++`, `protobuf`). Custom command generates C++ stubs from `.proto`. RPATH set for runtime linking. |
| `tests/test_values.cpp` | Unit tests for `Values` class. |
| `tests/test_mail_sender.cpp` | Unit tests for `MailSender` class. |
| `tests/test_rss_lite.cpp` | Unit tests for `Rss_lite` class. |
| `tests/test_rssani_lite.cpp` | Unit tests for `rssani_lite` class (regexp CRUD, auth CRUD, timer, settings). |
| `tests/test_myirc_session.cpp` | Unit tests for `MyIrcSession` class (datosIrc struct, IRC color stripping). |
| `tests/integration/test_grpc.py` | Python integration tests. Starts binary, waits for gRPC on 50051, exercises all 14 test scenarios, sends shutdown. |
| `tests/integration/test_xmlrpc.py` | Legacy XML-RPC integration test (kept for reference). |
| `xmlrpc.cpp/h` | **Deprecated.** Old XML-RPC server. Kept for reference only. |
