# rssani

[![CI](https://github.com/josacar/rssani/actions/workflows/ci.yml/badge.svg)](https://github.com/josacar/rssani/actions/workflows/ci.yml)

Headless C++ Qt6 console application that monitors RSS feeds and IRC channels from XBT/XBTT torrent trackers, downloads matching `.torrent` files based on user-defined regexps, and optionally sends email notifications. Exposes an XML-RPC API for remote management.

Originally created in 2004 for the Animersion community. Tested on Linux.

## Architecture

| File | Description |
|---|---|
| `main.cpp` | Entry point, creates `rssani_lite` + XML-RPC thread |
| `rssani_lite.cpp/h` | Core app: settings, regexp management, IRC integration, POSIX signal handling (self-pipe trick + `QSocketNotifier`) |
| `rss_lite.cpp/h` | RSS fetching, XML parsing, torrent downloading, regexp matching |
| `myircsession.cpp/h` | IRC client (libirc/grumpy-irc) monitoring channels for new uploads |
| `xmlrpc.cpp/h` | XML-RPC server thread (xmlrpc-c / Abyss) exposing management API |
| `mailsender.cpp/h` | SMTP email sender with SSL/TLS support |
| `values.h` | Configuration value object (paths, mail, debug flag) |

## Requirements

- Qt6 (Core, Network)
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

### Docker

A `Dockerfile` is provided for building and running tests in a clean Debian environment:

```bash
docker build -t rssani-tests . && docker run --rm rssani-tests
```

To update the Docker CMD to run all tests, update the last line of `Dockerfile`:
```dockerfile
CMD ["sh", "-c", "cd build && ./rssani_tests_values && ./rssani_tests_mail && ./rssani_tests_rss && ./rssani_tests_rssani && ./rssani_tests_irc"]
```

## CI/CD

GitHub Actions (`.github/workflows/ci.yml`) runs on every push and PR to `master`. It builds the project and runs all unit tests and CTest in a `debian:trixie-slim` container.

## Testing

### Unit Tests (C++ / Qt Test)
```bash
docker build -t rssani-tests . && docker run --rm rssani-tests
```
39 unit tests covering `Values`, `MailSender`, `Rss_lite`, `rssani_lite`, and `MyIrcSession` classes.

Run individual test binaries from the build directory:
```bash
./build/rssani_tests_values
./build/rssani_tests_mail
./build/rssani_tests_rss
./build/rssani_tests_rssani
./build/rssani_tests_irc
```

Or run all via CTest:
```bash
ctest --test-dir build --output-on-failure
```

### Integration Tests (Python / XML-RPC)
```bash
python3 test_xmlrpc.py ./build/rssani
```
14 integration tests covering the XML-RPC API (regexp CRUD, auth CRUD, options, timer, log, save, shutdown).

## Configuration

Settings are stored via `QSettings` (INI-style) in the standard Qt config path. Configurable options include:

- Tracker URLs and credentials
- IRC server and channel
- SMTP server, login, and password
- Regexp rules for torrent matching

Logs (`rssani.log`, `matches.log`) are written to the config directory.
