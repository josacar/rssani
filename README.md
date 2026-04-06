# rssani

Headless C++ Qt6 console application that monitors RSS feeds and IRC channels from XBT/XBTT torrent trackers, downloads matching `.torrent` files based on user-defined regexps, and optionally sends email notifications. Exposes an XML-RPC API for remote management.

Originally created in 2004 for the Animersion community. Tested on Linux.

## Architecture

| File | Description |
|---|---|
| `main.cpp` | Entry point, creates `rssani_lite` + XML-RPC thread |
| `rssani_lite.cpp/h` | Core app: settings, regexp management, IRC integration, signal wiring |
| `rss_lite.cpp/h` | RSS fetching, XML parsing, torrent downloading, regexp matching |
| `myircsession.cpp/h` | IRC client (libirc/grumpy-irc) monitoring channels for new uploads |
| `xmlrpc.cpp/h` | XML-RPC server thread (xmlrpc-c / Abyss) exposing management API |
| `mailsender.cpp/h` | SMTP email sender with SSL/TLS support |
| `values.h` | Configuration value object (paths, mail, debug flag) |

## Requirements

- Qt6 (Core, Network, Core5Compat)
- [xmlrpc-c](http://xmlrpc-c.sourceforge.net/) (system package: `libxmlrpc-c++9-dev`)
- [libirc](https://github.com/grumpy-irc/libirc) (grumpy-irc)
- A C++ compiler with C++20 support
- CMake ≥ 3.14
- Doxygen (optional, for docs)

## Build

xmlrpc-c must be installed as a system package (`libxmlrpc-c++9-dev` on Debian/Ubuntu). libirc is fetched and built automatically via CMake `ExternalProject`:

```bash
mkdir build && cd build
cmake ..
make
```

## Configuration

Settings are stored via `QSettings` (INI-style) in the standard Qt config path. Configurable options include:

- Tracker URLs and credentials
- IRC server and channel
- SMTP server, login, and password
- Regexp rules for torrent matching

Logs (`rssani.log`, `matches.log`) are written to the config directory.
