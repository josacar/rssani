# TODO.md

## Unit Tests

- [x] Add Qt Test framework to CMakeLists.txt
- [x] Create Dockerfile for building and running tests (Debian bookworm, Qt6, xmlrpc-c)
- [x] Unit tests for `Values` class (10 tests: defaults, setters, SMTP settings, filledValues)
- [x] Unit tests for `MailSender` class (9 tests: constructor, setters, content type, priority, encoding)
- [x] Unit tests for `Rss_lite` class (5 tests: saveLog, miraTitulo, verUltimo)
- [ ] Unit tests for `rssani_lite` class (regexp CRUD, auth CRUD, timer, settings)
- [ ] Unit tests for `MyIrcSession` class
- [ ] Add CTest integration (`ctest --test-dir build`)

## Qt5 Migration (complete)

- [x] Replace `QRegExp` with `QRegularExpression` (5 occurrences)
- [x] Replace deprecated `qsrand()`/`qrand()` with `QRandomGenerator`
- [x] Remove dead slots `finishedRSS()` and `finishedTorrent()`
- [x] Remove commented-out Qt4 code blocks in `rss_lite.cpp`
- [x] Fix `readDataTorrent()` — use reply URL as key instead of hardcoded `downloadId = 1`
- [x] Convert all `SIGNAL()`/`SLOT()` macro connections to Qt5 pointer-to-member syntax
- [x] Replace `QTextCodec` usage in `mailsender.cpp`

## Qt6 Migration (complete)

- [x] Update `CMakeLists.txt`: `find_package(Qt6)`, `Qt6::Core`, `Qt6::Network`, `qt6_create_translation`
- [x] Update `CMakeLists.txt`: libcommuni ExternalProject `qmake` → `qmake6`
- [x] Patch libcommuni `module_build.pri` to add `core5compat` (Qt6 moved `QTextCodec` to Qt5Compat)
- [x] Fix `QMutexLocker` → `QMutexLocker<QMutex>` (templated in Qt6) in `rssani_lite.cpp`
- [x] Fix `QList::move()` removal in `rssani_lite.cpp` `moverRegexp()` — replaced with `takeAt()`/`insert()`
- [x] Fix `std::max(qsizetype, int)` type mismatch in `xmlrpc.cpp` (`QList::count()` returns `qsizetype` in Qt6)
- [x] Verify build compiles and links against Qt6

## Bugs / FIXMEs in code

- [x] SMTP server is empty string — now read from settings via `Values::SmtpServer()`
- [x] SMTP login/password empty — now read from settings via `Values::SmtpLogin()`/`SmtpPass()`
- [x] `editarRegexp()` FIXME comment removed — exact name match is the correct behavior given the API
- [x] `fechaDescarga = nullptr` on read — removed misleading TODO comment; nullptr is correct for missing dates
- [x] `readDataTorrent()` fallback filename path — uncommented `QUrlQuery(url).queryItemValue(trk->id)`
- [x] `parseXml()` XML element nesting logic — `isCharacters()` moved to correct branch level
- [x] `mailsender.cpp` attachment reader magic number `0xA0B0C0D0` — replaced with simple `file.readAll()` + `toBase64()`

## Build / Project hygiene

- [x] Remove hardcoded absolute paths in `CMakeLists.txt` — replaced with `ExternalProject_Add`
- [x] Add `build/` to `.gitignore`
- [x] Remove `run.sh` `LD_PRELOAD` hack — RPATH is set in CMakeLists.txt

## XML-RPC library migration (complete)

- [x] Replace ulxmlrpcpp with xmlrpc-c (system package `libxmlrpc-c++9-dev`)
- [x] Rewrite `xmlrpc.h` — removed `Metodos` class, header now only declares `rssxmlrpc` thread
- [x] Rewrite `xmlrpc.cpp` — each RPC method is a `xmlrpc_c::method2` subclass, server uses `xmlrpc_c::serverAbyss`
- [x] Update `CMakeLists.txt` — removed ulxmlrpcpp `ExternalProject_Add`, link xmlrpc-c via `xmlrpc-c-config`
- [x] Shutdown method calls `server.terminate()` to cleanly stop the Abyss event loop

## IRC library migration (complete)

- [x] Replace libcommuni with libirc (grumpy-irc) — Qt-based IRC library with native Qt6 support
- [x] Rewrite `myircsession.h` — `MyIrcSession` now inherits `QObject`, uses `libircclient::Network`
- [x] Rewrite `myircsession.cpp` — uses `libirc::ServerAddress`, `Network::Event_*` signals
- [x] Update `CMakeLists.txt` — replaced libcommuni `ExternalProject_Add` with libirc, `-DQT6_BUILD=true`
- [x] Removed `IRC_STATIC` define and `connectSlotsByName()` old-style macro connections

## Qt6 style modernization (complete)

- [x] Replace `.count()` with `.size()` across all source files
- [x] Replace `QLatin1String` with `QStringLiteral`, `QLatin1Char` with `QChar`
- [x] Replace `QString::fromUtf8`/`fromLatin1` string literals with `QStringLiteral`
- [x] Replace `!= ""` comparisons with `!isEmpty()`
- [x] Replace `.size() > 0` with `!isEmpty()`
- [x] Modernize `values.h`: const getters, const ref setters, default ctor/dtor, member initializers
- [x] Replace `#include <time.h>` with `<ctime>`, remove unused `<cstdio>`
- [x] Remove commented-out `foreach` lines
- [x] IRC encoding: `EncodingLatin` → `EncodingUTF8`
- [x] Remove unused `iso88591` enum from mailsender

## Code quality

- [x] Normalize null pointers — `nullptr` everywhere
- [x] Replace raw C `malloc` in `irc_color_strip_from_mirc()` with `QString` operations
- [x] Fix memory leaks:
  - `verLog()` returns `QStringList` by value now
  - `iniciaTrackers()` tracker objects freed in `~Rss_lite()` via `qDeleteAll`
  - `url` in `fetch()` is now a stack `QUrl`, no more `new`/`delete`
  - `regexp::fechaDescarga` is now `QDateTime` value type, no more `new`/`delete`
- [x] Move IRC channel/server (`#PuntoTorrent`, `irc.irc-hispano.org`) to settings
- [x] Move tracker configuration from `iniciaTrackers()` hardcoded block to settings
- [x] Use HTTPS for tracker URLs (no more hardcoded `http://`; scheme derived from URL)
- [x] Thread safety: `xmlrpc.cpp` runs in a `QThread` — all `rssani_lite` public methods now protected by `QMutex`
- [x] Fix signal handler deadlock — replaced unsafe `sigHandler` (called mutex methods from signal context) with self-pipe trick using `QSocketNotifier`; `salir()` now uses `QCoreApplication::quit()` for clean shutdown
- [x] Fix memory leaks:
  - `borrarRegexp(string)` now deletes `regexp*` before removing from list
  - `parseTitle()` now deletes expired `regexp*` before removing from list
  - `readDataTorrent()` now writes torrent to disk and clears `ficheros`/`datos`/`sites` hashes
  - `~rssani_lite()` now calls `qDeleteAll(*lista)` to free all `regexp*` pointers
  - `~rssani_lite()` now closes `sigFd` socketpair file descriptors

## clang-tidy findings

### Bugs / potential crashes

- [ ] `xmlrpc.cpp` `VerLog::execute()` — `qsizetype` (64-bit) narrowed to `int` for `ini`/`fin` loop variables
- [ ] `rss_lite.cpp` `miraTitulo()` — switch on `parseTitle()` return has no default case
- [ ] `rss_lite.h`/`rss_lite.cpp` — `parseTitle` param names differ between declaration (`titleString`, `linkString`) and definition (`titulo`, `enlace`)
- [ ] `rssani_lite.h`/`rssani_lite.cpp` — `editarRegexp(int)` param name differs: `regexpOrig` vs `pos`
- [ ] `rssani_lite.h`/`rssani_lite.cpp` — `activarRegexp` param name differs: `regexpOrig` vs `pos`
- [ ] `rssani_lite.h`/`rssani_lite.cpp` — `miraSubida` param name differs: `msg` vs `subida`
- [ ] `rssani_lite.cpp` — `tmp` in `handleSigTerm()` and `re` in `writeSettings()`/`readSettings()` uninitialized

### Performance

- [ ] Pass `QString`/`std::string` by `const &` instead of by value (~15 instances across `parseTitle`, `miraTitulo`, `parseLink`, `sendMail`, `saveLog`, `anadirRegexp`, `anadirAuth`, `borrarAuth`, `borrarRegexp`, `editarRegexp`, `setRpcUser`, `setRpcPass`, `miraSubida`, `getMimeType`)
- [ ] `rss_lite.cpp` — use `'\n'` instead of `std::endl` (avoids unnecessary flush)
- [ ] `rss_lite.cpp` `iniciaTrackers()` — `std::move(trk)` on `shared_ptr` passed to `QHash::insert` taking const ref; move has no effect

### Modernize (clang-tidy)

- [x] `mailsender.cpp:370` — use `auto` when initializing with `qobject_cast` (`modernize-use-auto`)
- [x] `rss_lite.cpp:27` — use `= default` for trivial `~Rss_lite()` destructor (`modernize-use-equals-default`)
- [x] `rssani_lite.cpp:13` — replace C-style array `sigFd[2]` with `std::array<int, 2>` (`modernize-avoid-c-arrays`)
- [x] `rssani_lite.cpp` — `editarRegexp`/`activarRegexp` return `0`/`1` as `bool`; use `false`/`true` (`modernize-use-bool-literals`)
- [x] `rssani_lite.cpp:192` — use `auto` when initializing with `new regexp()` (`modernize-use-auto`)
- [x] `xmlrpc.cpp:310` — use default member initializer `{nullptr}` for `Shutdown::server` (`modernize-use-default-member-init`)
