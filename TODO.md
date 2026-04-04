# TODO.md

## Qt5 Migration (complete)

- [x] Replace `QRegExp` with `QRegularExpression` (5 occurrences)
- [x] Replace deprecated `qsrand()`/`qrand()` with `QRandomGenerator`
- [x] Remove dead slots `finishedRSS()` and `finishedTorrent()`
- [x] Remove commented-out Qt4 code blocks in `rss_lite.cpp`
- [x] Fix `readDataTorrent()` — use reply URL as key instead of hardcoded `downloadId = 1`
- [x] Convert all `SIGNAL()`/`SLOT()` macro connections to Qt5 pointer-to-member syntax
- [x] Replace `QTextCodec` usage in `mailsender.cpp`

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
- [ ] Fix memory leaks:
  - `verLog()` returns `new QStringList` that callers (xmlrpc.cpp) never delete
  - `iniciaTrackers()` allocates `tracker*` with `new` but they're never freed
  - `url` in `fetch()` is `new QUrl` allocated per-tracker per-fetch, fragile lifetime
  - `regexp::fechaDescarga` is `new QDateTime` — use value type or smart pointer
- [ ] Move IRC channel/server (`#PuntoTorrent`, `irc.irc-hispano.org`) to settings
- [ ] Move tracker configuration from `iniciaTrackers()` hardcoded block to settings
- [ ] Use HTTPS for tracker URLs (currently all `http://`)
- [ ] Thread safety: `xmlrpc.cpp` runs in a `QThread` and calls `rssani_lite` methods directly without mutex protection
