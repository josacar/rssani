# AGENTS.md

## Project Overview

rssani is a headless C++ Qt5 console application that monitors RSS feeds and IRC channels from XBT/XBTT torrent trackers, downloads matching `.torrent` files based on user-defined regexps, and optionally sends email notifications. It exposes an XML-RPC API for remote management. Started in 2004, originally targeting the Animersion community.

## Architecture

```
main.cpp            → Entry point, creates rssani_lite + rssxmlrpc thread
rssani_lite.cpp/h   → Core app: settings, regexp management, IRC integration, signal wiring
rss_lite.cpp/h      → RSS fetching, XML parsing, torrent downloading, regexp matching
myircsession.cpp/h  → IRC client (libcommuni) monitoring channel for new uploads
xmlrpc.cpp/h        → XML-RPC server thread (ulxmlrpcpp) exposing management API
mailsender.cpp/h    → SMTP email sender with SSL/TLS support
values.h            → Configuration value object (paths, mail, debug flag)
```

## Build

```bash
cmake . && make
```

Dependencies: Qt5 (Core, Network), ulxmlrpcpp, libcommuni (IrcCore). CMakeLists.txt currently has hardcoded paths to local builds of ulxmlrpcpp and libcommuni — these must be adapted per machine.

## Key Conventions

- Language: C++11 with Qt5. Comments and identifiers are mostly in Spanish.
- Configuration: `QSettings` (INI-style), stored in the standard Qt config path.
- Logging: `QFile`-based plain text logs (`rssani.log`, `matches.log`) in the config directory.
- Signal/slot: Currently uses old `SIGNAL()`/`SLOT()` macro syntax throughout.
- No tests exist. No CI/CD.

## Important Caveats

- The Qt5 migration from Qt4 is **incomplete**. See `TODO.md` for details.
- `finishedRSS()` and `finishedTorrent()` slots exist but are **never connected** — they are dead code left from the Qt4 `QHttp` API migration.
- `readDataTorrent()` uses a hardcoded `downloadId = 1` instead of tracking per-reply, so concurrent torrent downloads will collide.
- Tracker configuration is hardcoded in `iniciaTrackers()` rather than loaded from settings.
- SMTP server and login credentials are empty strings with `//FIXME` comments in `sendMail()`.
- The IRC channel (`#PuntoTorrent`) and server (`irc.irc-hispano.org`) are hardcoded.
- Memory management uses raw `new` without smart pointers; several potential leaks exist (e.g., `verLog()` returns `new QStringList` that callers never delete, tracker objects in `iniciaTrackers()`).
- Mix of `NULL`, `nullptr`, and `0` for null pointers.

## File-by-File Notes

| File | Notes |
|---|---|
| `rss_lite.cpp` | Most migration issues live here. Contains commented-out Qt4 `QHttp` code, dead slots, hardcoded download ID, and all RSS/torrent logic. |
| `rssani_lite.cpp` | Settings I/O, regexp CRUD, signal wiring. Uses `QRegExp` for IRC message parsing. |
| `myircsession.cpp` | Uses deprecated `qsrand()`/`qrand()`. IRC color stripping uses raw C `malloc`. |
| `mailsender.cpp` | Uses deprecated `qrand()`, `QTextCodec`. Attachment handling has a suspicious magic-number check. |
| `xmlrpc.cpp` | Runs in a `QThread`. Exposes all management methods. No thread-safety on shared data. |
| `CMakeLists.txt` | Hardcoded absolute paths. Should use `find_package` or `pkg-config` for dependencies. |
