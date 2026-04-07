# AGENTS.md

## Project Overview

rssani is a headless C++ Qt6 console application that monitors RSS feeds and IRC channels from XBT/XBTT torrent trackers, downloads matching `.torrent` files based on user-defined regexps, and optionally sends email notifications. It exposes an XML-RPC API for remote management. Started in 2004, originally targeting the Animersion community.

## Architecture

```
main.cpp            → Entry point, creates rssani_lite + rssxmlrpc thread, registers signal handlers
rssani_lite.cpp/h   → Core app: settings, regexp management, IRC integration, signal wiring
rss_lite.cpp/h      → RSS fetching, XML parsing, torrent downloading, regexp matching
myircsession.cpp/h  → IRC client (libirc/grumpy-irc) monitoring channel for new uploads
xmlrpc.cpp/h        → XML-RPC server thread (xmlrpc-c / Abyss) exposing management API
mailsender.cpp/h    → SMTP email sender with SSL/TLS support
values.h            → Configuration value object (paths, mail, debug flag)
```

## Build

xmlrpc-c must be installed as a system package (`libxmlrpc-c++9-dev`). libirc (grumpy-irc) is fetched and built automatically via CMake `ExternalProject`:

```bash
mkdir build && cd build
cmake ..
make
```

## Agent Instructions

- After every code change, update `AGENTS.md`, `README.md`, and `TODO.md` to reflect the new state of the project.

## Key Conventions

- Language: C++20 with Qt6. Comments and identifiers are mostly in Spanish.
- Configuration: `QSettings` (INI-style), stored in the standard Qt config path.
- Logging: `QFile`-based plain text logs (`rssani.log`, `matches.log`) in the config directory.
- Signal/slot: Qt5 pointer-to-member syntax (`&Class::method`) throughout.
- Memory management: `std::unique_ptr` and `std::shared_ptr` used for owned objects.
- Thread safety: `rssani_lite` public methods are protected by `QMutex` for safe access from the XML-RPC thread.
- No CI/CD.

## CI/CD

GitHub Actions workflow at `.github/workflows/ci.yml` triggers on push/PR to `master`. Runs in a `debian:trixie-slim` container: installs dependencies, configures via CMake, builds with `make -j2`, runs all 5 unit test executables, and runs CTest.

## Testing

### Unit Tests (Qt Test / C++)
Unit tests are in `tests/` and built as separate executables (`rssani_tests_values`, `rssani_tests_mail`, `rssani_tests_rss`, `rssani_tests_rssani`, `rssani_tests_irc`). Built via CMake alongside the main binary.

Run via Docker:
```bash
docker build -t rssani-tests . && docker run --rm rssani-tests
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

### Integration Tests (Python / XML-RPC)
Integration tests for the XML-RPC interface are in `test_xmlrpc.py` (Python 3, uses `xmlrpc.client`):

```bash
python3 test_xmlrpc.py ./build/rssani
```

The script starts the binary, runs 14 tests covering all RPC methods (regexp CRUD, auth CRUD, options, timer, log, save, shutdown), then shuts down.

## Important Caveats

- IRC server/channel and tracker configuration are read from `QSettings`, with hardcoded defaults (`irc.irc-hispano.org`, `#PuntoTorrent`) as fallbacks.
- `rss` and `session` in `rssani_lite` constructor are still raw `new` (Qt parent-child ownership).

## File-by-File Notes

| File | Notes |
|---|---|
| `main.cpp` | Entry point. Constructs `rssani_lite`, starts `rssxmlrpc` thread. |
| `rss_lite.cpp` | RSS/torrent logic. Trackers loaded from settings via `iniciaTrackers()`. Uses reply URL as download key for concurrent torrent downloads. Downloads are written to disk in `readDataTorrent()` and logged via `saveLog()`. |
| `rssani_lite.cpp` | Settings I/O, regexp CRUD, signal wiring. All public methods mutex-protected. Uses `QRegularExpression`. POSIX signal handling uses self-pipe trick with `QSocketNotifier` to avoid deadlocks. |
| `myircsession.cpp` | IRC client. Uses `QRandomGenerator`. Connects to `libircclient::Network` signals (`Event_PRIVMSG`, `Event_Connected`, `Event_SelfKick`, `Event_MOTDEnd`). |
| `mailsender.cpp` | SMTP sender. Credentials read from `Values`. Uses `QRandomGenerator`. |
| `xmlrpc.cpp` | Runs in a `QThread`. Uses xmlrpc-c (Abyss server). Each RPC method is a `xmlrpc_c::method2` subclass. Accesses `rssani_lite` through mutex-protected API. Destructor terminates the Abyss server and waits for the thread to finish. |
| `CMakeLists.txt` | Uses `ExternalProject_Add` for libirc (grumpy-irc). xmlrpc-c linked as system library via `xmlrpc-c-config`. RPATH set for runtime linking. |
| `tests/test_values.cpp` | Unit tests for `Values` class. |
| `tests/test_mail_sender.cpp` | Unit tests for `MailSender` class. |
| `tests/test_rss_lite.cpp` | Unit tests for `Rss_lite` class. |
| `tests/test_rssani_lite.cpp` | Unit tests for `rssani_lite` class (regexp CRUD, auth CRUD, timer, settings). |
| `tests/test_myirc_session.cpp` | Unit tests for `MyIrcSession` class (datosIrc struct, IRC color stripping). |
