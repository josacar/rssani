# TODO.md

## Qt5 Migration (incomplete)

- [ ] Replace `QRegExp` with `QRegularExpression` (5 occurrences)
  - `rss_lite.cpp:178-180` — `rxlen`, `syl`, `titulo` regexps in `parseXml()`
  - `rss_lite.cpp:281` — regexp matching in `parseTitle()`
  - `rssani_lite.cpp:78` — `rehttp` in `miraSubida()`
- [ ] Replace deprecated `qsrand()`/`qrand()` with `QRandomGenerator`
  - `myircsession.cpp:90-91` — IRC ping
  - `mailsender.cpp:42` — `createBoundary()`
  - `mailsender.cpp:214` — `mailData()` message ID
- [ ] Remove dead slots `finishedRSS()` and `finishedTorrent()` — they were connected to Qt4 `QHttp::requestFinished()` which no longer exists; the commented-out connections in `prepareSignals()` confirm this
- [ ] Remove commented-out Qt4 code blocks in `rss_lite.cpp` (lines 55-62, 377, 463-471)
- [ ] Fix `readDataTorrent()` — uses hardcoded `downloadId = 1` instead of tracking per `QNetworkReply*`; concurrent downloads will collide
- [ ] Convert all `SIGNAL()`/`SLOT()` macro connections to Qt5 pointer-to-member syntax (~30 connections across 5 files)
- [ ] Replace `QTextCodec` usage in `mailsender.cpp:202` (deprecated in Qt6, consider `QString::toUtf8()` / `QString::fromUtf8()`)

## Bugs / FIXMEs in code

- [ ] SMTP server is empty string — `rss_lite.cpp:556` `MailSender mail( QLatin1String(""), ...)` — mail will never send
- [ ] SMTP login/password empty — `rss_lite.cpp:560` `mail.setLogin( QLatin1String(""), QLatin1String(""))` — needs settings
- [ ] `editarRegexp()` doesn't verify tracker match — `rssani_lite.cpp:119` `// FIXME: Asegurarse que cambia la del tracker y no otra`
- [ ] `fechaDescarga = NULL` on read — `rssani_lite.cpp:376` `// TODO: Ver estooooo`
- [ ] `readDataTorrent()` fallback filename path is commented out — `rss_lite.cpp:403` `//FIXME fichero = url.queryItemValue( trk->id )`
- [ ] `parseXml()` XML element nesting logic is broken — the `isCharacters()` check is inside the `isEndElement()` branch (line ~213), so character data between tags may be missed
- [ ] `mailsender.cpp` attachment reader checks magic number `0xA0B0C0D0` — this will reject most real files; the attachment logic looks broken

## Build / Project hygiene

- [x] Remove hardcoded absolute paths in `CMakeLists.txt` — replaced with `ExternalProject_Add` for ulxmlrpcpp and libcommuni
- [ ] Add build artifacts to `.gitignore` (CMakeFiles/, CMakeCache.txt, Makefile, etc. are already listed but were committed before the gitignore existed)
- [ ] Remove `run.sh` `LD_PRELOAD` hack — proper `RPATH` or installed libraries should be used

## Code quality

- [ ] Normalize null pointers — mix of `NULL`, `nullptr`, and `0` throughout; use `nullptr` everywhere
- [ ] Fix memory leaks:
  - `verLog()` returns `new QStringList` that callers (xmlrpc.cpp) never delete
  - `iniciaTrackers()` allocates `tracker*` with `new` but they're never freed
  - `url` in `fetch()` is `new QUrl` allocated per-tracker per-fetch, fragile lifetime
  - `regexp::fechaDescarga` is `new QDateTime` — use value type or smart pointer
- [ ] Replace raw C `malloc` in `irc_color_strip_from_mirc()` with `QString` operations
- [ ] Move IRC channel/server (`#PuntoTorrent`, `irc.irc-hispano.org`) to settings
- [ ] Move tracker configuration from `iniciaTrackers()` hardcoded block to settings
- [ ] Use HTTPS for tracker URLs (currently all `http://`)
- [ ] Thread safety: `xmlrpc.cpp` runs in a `QThread` and calls `rssani_lite` methods directly without mutex protection
