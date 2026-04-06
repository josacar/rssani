# AGENTS.md

## Project Overview

rssani is a headless C++ Qt6 console application that monitors RSS feeds and IRC channels from XBT/XBTT torrent trackers, downloads matching `.torrent` files based on user-defined regexps, and optionally sends email notifications. It exposes an XML-RPC API for remote management. Started in 2004, originally targeting the Animersion community.

## Architecture

```
main.cpp            → Entry point, creates rssani_lite + rssxmlrpc thread
rssani_lite.cpp/h   → Core app: settings, regexp management, IRC integration, signal wiring
rss_lite.cpp/h      → RSS fetching, XML parsing, torrent downloading, regexp matching
myircsession.cpp/h  → IRC client (libcommuni) monitoring channel for new uploads
xmlrpc.cpp/h        → XML-RPC server thread (xmlrpc-c / Abyss) exposing management API
mailsender.cpp/h    → SMTP email sender with SSL/TLS support
values.h            → Configuration value object (paths, mail, debug flag)
```

## Build

xmlrpc-c must be installed as a system package (`libxmlrpc-c++9-dev`). libcommuni is fetched and built automatically via CMake `ExternalProject`:

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
- Signal/slot: Qt5 pointer-to-member syntax (`&Class::method`) throughout, except `myircsession.cpp` `connectSlotsByName()` which uses old `SIGNAL()`/`SLOT()` macros (required by libcommuni API).
- Memory management: `std::unique_ptr` and `std::shared_ptr` used for owned objects.
- Thread safety: `rssani_lite` public methods are protected by `QMutex` for safe access from the XML-RPC thread.
- No tests exist. No CI/CD.

## Important Caveats

- IRC server/channel and tracker configuration are read from `QSettings`, with hardcoded defaults (`irc.irc-hispano.org`, `#PuntoTorrent`) as fallbacks.
- `myircsession.cpp` `connectSlotsByName()` still uses old `SIGNAL()`/`SLOT()` macros — this is a libcommuni API constraint, not a migration oversight.
- `rss` and `session` in `rssani_lite` constructor are still raw `new` (Qt parent-child ownership).

## File-by-File Notes

| File | Notes |
|---|---|
| `rss_lite.cpp` | RSS/torrent logic. Trackers loaded from settings via `iniciaTrackers()`. Uses reply URL as download key for concurrent torrent downloads. |
| `rssani_lite.cpp` | Settings I/O, regexp CRUD, signal wiring. All public methods mutex-protected. Uses `QRegularExpression`. |
| `myircsession.cpp` | IRC client. Uses `QRandomGenerator`. `connectSlotsByName()` uses old-style macros (libcommuni constraint). |
| `mailsender.cpp` | SMTP sender. Credentials read from `Values`. Uses `QRandomGenerator`. |
| `xmlrpc.cpp` | Runs in a `QThread`. Uses xmlrpc-c (Abyss server). Each RPC method is a `xmlrpc_c::method2` subclass. Accesses `rssani_lite` through mutex-protected API. |
| `CMakeLists.txt` | Uses `ExternalProject_Add` for libcommuni. xmlrpc-c linked as system library via `xmlrpc-c-config`. RPATH set for runtime linking. |
