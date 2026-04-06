# TODO.md

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
