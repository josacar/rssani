# rssani Improvement Plan — Modern C++ (2026)

Comprehensive analysis of the rssani codebase with improvements categorized by priority.

---

## 1. CRITICAL: Thread Safety & Ownership Bugs

### 1a. Internal data exposed without mutex protection

`listaRegexp()`, `listaAuths()`, and `getValues()` return raw pointers to internal data after releasing the mutex lock. Any gRPC caller holds the pointer after the lock releases, creating data races.

**Affected files:** `rssani_lite.cpp:133-136`, `rssani_lite.cpp:247-250`, `rssani_lite.cpp:483-486`

```cpp
// Current — mutex released, caller has raw pointer with no protection
QList<regexp*>* rssani_lite::listaRegexp() {
  QMutexLocker<QMutex> locker(&mutex);
  return lista.get();  // ← lock released here, pointer still valid but unprotected
}

QList<auth>* rssani_lite::listaAuths() {
  QMutexLocker<QMutex> locker(&mutex);
  return listAuths.get();  // ← same problem
}

Values* rssani_lite::getValues() const {
  QMutexLocker<QMutex> locker(&mutex);
  return values.get();  // ← same problem
}
```

**Fix:** Return copies/snapshots instead of internal pointers. The gRPC layer already copies data into protobuf messages, so returning copies is fine:

```cpp
QList<regexp> rssani_lite::listaRegexpSnapshot() {
  QMutexLocker<QMutex> locker(&mutex);
  QList<regexp> result;
  result.reserve(lista->size());
  for (const auto *re : *lista)
    result.append(*re);
  return result;
}

QList<auth> rssani_lite::listaAuthsSnapshot() {
  QMutexLocker<QMutex> locker(&mutex);
  return *listAuths;  // QList<auth> is a value type, returns a copy
}

Values rssani_lite::getValues() const {
  QMutexLocker<QMutex> locker(&mutex);
  return *values;  // Values is a value type, returns a copy
}
```

Then update `grpc_server.cpp` to work with the snapshot copies instead of dereferencing raw pointers.

### 1b. Detached gRPC server thread

**File:** `grpc_server.cpp:213-231`

`std::thread().detach()` is undefined behavior if the thread outlives `GrpcServer`. The `stop()` method attempts `server->Shutdown()` but the detached thread cannot be joined, so destruction order is non-deterministic.

```cpp
// Current — thread is detached, cannot be joined
std::thread([this]() {
  // ...
  server->Wait();
}).detach();
```

**Fix:** Store the thread as a member and join it in the destructor:

```cpp
class GrpcServer : public QObject {
  // ...
private:
  std::thread serverThread;  // ← add member
};

void GrpcServer::start() {
  if (running.exchange(true)) return;
  serverThread = std::thread([this]() {
    RssaniServiceImpl service(rss);
    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    server = builder.BuildAndStart();
    if (!server) {
      qCritical() << "Failed to start gRPC server";
      running.store(false);
      return;
    }
    qDebug() << "gRPC server listening on 0.0.0.0:50051";
    server->Wait();
    qDebug() << "gRPC server stopped";
    running.store(false);
  });
}

GrpcServer::~GrpcServer() {
  stop();
  if (serverThread.joinable())
    serverThread.join();
}
```

### 1c. Implicit switch fallthrough in `miraTitulo()` — **DONE**

The `[[fallthrough]]` attribute and `default: break;` were added. See 5d for the full enum class refactor.

### 5d. Replace magic return codes with `enum class` — **DONE**

`parseTitle()` now returns `MatchResult` enum class instead of magic integers:

```cpp
enum class MatchResult : int8_t {
  NotMatched = 0,
  Download = 1,
  AlreadyNotified = 2,
  MailFailed = 3
};
```

The `miraTitulo()` switch now uses `MatchResult::Download`, `MatchResult::AlreadyNotified`, and `default`.

---

## 2. HIGH: Modernize Ownership & Smart Pointers

### 2a. Replace raw `new` with smart pointers

**Files:** `rssani_lite.cpp:79`, `rssani_lite.cpp:82`

```cpp
// Current — raw new with Qt parent ownership
rss = new Rss_lite(values.get(), lista.get(), flog.get(), hashAuths.get(), this);
session = new MyIrcSession(this, &misdatos, misdatos.debug);
```

The AGENTS.md already documents this as a known caveat. These are the last two raw `new` calls in the codebase.

**Fix:** Use `std::unique_ptr` with custom deleter or `QScopedPointer`. Since Qt parent-child ownership handles deletion, you need to be careful not to double-delete. Best approach: remove the Qt parent and use only `std::unique_ptr`:

```cpp
// In header:
std::unique_ptr<Rss_lite> rss;
std::unique_ptr<MyIrcSession> session;

// In constructor:
rss = std::make_unique<Rss_lite>(values.get(), lista.get(), flog.get(), hashAuths.get());
session = std::make_unique<MyIrcSession>(&misdatos, misdatos.debug);
```

If you want to keep Qt parent ownership for convenience, use `QScopedPointer` with `deleteLater` or just leave the raw `new` with a documented lifetime contract.

### 2b. Make `regexp` a value type

**File:** `rss_lite.h:28-36`

Currently `QList<regexp*>` with manual `new`/`delete` is the #1 memory-safety risk. Every `borrarRegexp`, `editarRegexp`, and destructor must manually manage lifetime. Iterator invalidation bugs are easy to introduce.

```cpp
// Current — manual memory management
QList<regexp*> *lista;
// ...
auto *re = new regexp();
re->nombre = "...";
lista->append(re);
// ...later...
delete lista->at(i);
lista->removeAt(i);
```

**Fix:** Change to `QList<regexp>` (value type). Eliminate all `new regexp()` and `delete` calls:

```cpp
// In header:
QList<regexp> lista;  // value type, no pointer

// Usage:
regexp re;
re.nombre = "...";
re.activa = true;
lista.append(re);  // copies, no manual delete needed

// Editing:
lista[i].nombre = "...";

// Deleting:
lista.removeAt(i);  // no manual delete
```

This eliminates the entire class of use-after-free and leak bugs. The trade-off is slightly more copying, but `regexp` is small (3 QStrings + 2 bools + int + QDateTime) so the cost is negligible.

### 2c. Make `tracker` a value type

**File:** `rss_lite.h:41-49`, `rss_lite.cpp:427-441`

`QHash<QString, std::shared_ptr<tracker>>` — shared ownership is unnecessary since `tracker` is only owned by this hash.

**Fix:** Use `QHash<QString, tracker>` (value type):

```cpp
QHash<QString, tracker> trackers;

// In iniciaTrackers():
tracker trk;
trk.urlTracker = au.tracker;
trk.cookie = ...;
trackers.insert(url, trk);
```

### 2d. Eliminate global `gRss`

**File:** `rssani_lite.cpp:10`

```cpp
rssani_lite *gRss = nullptr;  // global mutable singleton
```

Used only by the POSIX signal handler (`sigHandler`). This is a classic C-style global that should be removed for testability and thread safety.

**Fix:** The self-pipe pattern is already in place (`QSocketNotifier`). The signal handler only needs to write a byte to the pipe — it doesn't need to access `rssani_lite` at all. Remove `gRss` entirely:

```cpp
// Remove: rssani_lite *gRss = nullptr;
// Remove: gRss = this; from constructor

// sigHandler just writes to the pipe (already correct):
void sigHandler(int) {
  char a = 1;
  ::write(rssani_lite::sigFd[0], &a, sizeof(a));
}
```

`gRss` is not used anywhere else. The `handleSigTerm()` slot handles the actual shutdown via `QSocketNotifier`.

---

## 3. HIGH: API Design & Return Types

### 3a. Inverted bool returns — **DONE**

**Files:** `rssani_lite.cpp:138-169`, `rssani_lite.cpp:171-180`

`editarRegexp()`, `editarRegexpI()`, and `activarRegexp()` returned `false` on success and `true` on failure, which contradicts convention and the gRPC API which maps the return to a `BoolResponse`:

```cpp
// BEFORE — inverted returns
bool rssani_lite::editarRegexp(std::string regexpOrig, std::string regexpDest) {
  // ...
  if (pos != -1) {
    // success case
    return false;  // ← confusing: false = success?
  } else {
    return true;   // ← confusing: true = failure?
  }
}
```

The gRPC layer in `grpc_server.cpp:91-92` passes this through directly:

```cpp
response->set_value(rss->editarRegexp(request->regexporig(), request->regexpdest()));
```

So the client receives `true` when the edit fails and `false` when it succeeds. This is backwards.

**Fix:** Invert the return values to follow convention (true = success):

```cpp
bool rssani_lite::editarRegexp(std::string regexpOrig, std::string regexpDest) {
  QMutexLocker<QMutex> locker(&mutex);
  for (int i = 0; i < lista->size(); ++i) {
    if (lista->at(i)->nombre == QString::fromStdString(regexpOrig)) {
      lista->at(i)->nombre = QString::fromStdString(regexpDest);
      return true;   // success
    }
  }
  return false;  // not found
}
```

Update the integration tests accordingly (they currently assert inverted semantics).

### 3b. Use `std::optional` for find-or operations

Methods like `editarRegexp(string)` search and edit in one step, making error reporting opaque. A method returning `bool` doesn't communicate *why* it failed.

**Fix (C++20 compatible):**

```cpp
enum class EditError { NotFound, InvalidIndex };

std::expected<void, EditError> editarRegexp(std::string_view orig, std::string_view dest) {
  QMutexLocker<QMutex> locker(&mutex);
  auto it = std::ranges::find_if(*lista, [&](const auto &re) {
    return re.nombre == QString::fromStdString(std::string(orig));
  });
  if (it == lista->end()) return std::unexpected(EditError::NotFound);
  it->nombre = QString::fromStdString(std::string(dest));
  return {};
}
```

For C++20 compatibility, a simpler `Result<T>` type or just `std::optional<int>` (returning the found index) would work.

### 3c. Replace `std::string` parameters with `std::string_view`

**File:** `rssani_lite.h:50-104`

All public methods taking `std::string` make unnecessary copies when called from gRPC (which provides `std::string` from protobuf, but `string_view` would also avoid copies for string literals and substrings).

```cpp
// Current — copies on every call
void anadirRegexp(std::string nombre, std::string fecha, bool mail, std::string tracker, int dias);
bool editarRegexp(std::string regexpOrig, std::string regexpDest);
void borrarRegexp(std::string cad);
void setRpcUser(std::string theValue);
```

**Fix:**

```cpp
void anadirRegexp(std::string_view nombre, std::string_view fecha, bool mail, std::string_view tracker, int dias);
bool editarRegexp(std::string_view regexpOrig, std::string_view regexpDest);
void borrarRegexp(std::string_view cad);
void setRpcUser(std::string_view theValue);
```

On the Qt side, use `QStringView` where the input is already a `QString`.

### 3d. Add `[[nodiscard]]` to all const getters

None of the const getter methods have `[[nodiscard]]`, meaning their return values can be silently discarded.

```cpp
// Current
QString Ruta() const { return ruta; }
int SmtpPort() const { return smtpPort; }

// Fix
[[nodiscard]] QString Ruta() const { return ruta; }
[[nodiscard]] int SmtpPort() const { return smtpPort; }
```

---

## 4. MEDIUM: Encapsulate Data Model

### 4a. Convert `regexp` struct to class with proper encapsulation

**File:** `rss_lite.h:28-36`

All members are public with no invariants, no validation, and no encapsulation. This allows creating invalid states (e.g., empty `nombre`) that propagate through the system.

**Fix:**

```cpp
class Regexp {
public:
  Regexp() = default;
  Regexp(QString pattern, QString expiry = {}, bool mailOnly = false,
         QString tracker = {}, int minDays = 0, bool active = true)
    : m_pattern(std::move(pattern)), m_expiry(std::move(expiry)),
      m_mailOnly(mailOnly), m_tracker(std::move(tracker)),
      m_minDaysBetween(minDays), m_active(active) {}

  [[nodiscard]] QString pattern() const { return m_pattern; }
  void setPattern(QString p) { m_pattern = std::move(p); }

  [[nodiscard]] QString expiry() const { return m_expiry; }
  void setExpiry(QString e) { m_expiry = std::move(e); }

  [[nodiscard]] bool isMailOnly() const { return m_mailOnly; }
  void setMailOnly(bool m) { m_mailOnly = m; }

  [[nodiscard]] QString tracker() const { return m_tracker; }
  void setTracker(QString t) { m_tracker = std::move(t); }

  [[nodiscard]] int minDaysBetween() const { return m_minDaysBetween; }
  void setMinDaysBetween(int d) { m_minDaysBetween = d; }

  [[nodiscard]] QDateTime lastDownload() const { return m_lastDownload; }
  void setLastDownload(QDateTime dt) { m_lastDownload = std::move(dt); }

  [[nodiscard]] bool isActive() const { return m_active; }
  void setActive(bool a) { m_active = a; }
  void toggleActive() { m_active = !m_active; }

private:
  QString m_pattern;
  QString m_expiry;
  bool m_mailOnly = false;
  QString m_tracker;
  int m_minDaysBetween = 0;
  QDateTime m_lastDownload;
  bool m_active = true;
};
```

### 4b. Same for `auth` struct

**File:** `values.h:9-18`

```cpp
class Auth {
  // getters/setters for tracker, uid, pass, passkey, referer, idField, urlDownload, urlRss
  // with default values for referer, idField, urlDownload, urlRss
};
```

### 4c. Add default member initializers to `datosIrc`

**File:** `myircsession.h:16-26`

```cpp
// Current — no defaults, can contain garbage
struct datosIrc {
  bool activo;      // ← uninitialized if not set
  QString nick;
  QString user;
  QString name;
  QString server;
  int port;          // ← uninitialized if not set
  QStringList channels;
  QString botNick;
  bool debug;        // ← uninitialized if not set
};

// Fix
struct datosIrc {
  bool activo = false;
  QString nick;
  QString user;
  QString name;
  QString server = QStringLiteral("irc.irc-hispano.org");
  int port = 6667;
  QStringList channels = {QStringLiteral("#PuntoTorrent")};
  QString botNick = QStringLiteral("PuntoTorrent");
  bool debug = false;
};
```

This also removes the hardcoded defaults currently scattered across `rssani_lite.cpp:471-475`.

### 4d. Same for `tracker` struct

**File:** `rss_lite.h:41-49`

```cpp
struct tracker {
  QString urlTracker;
  QString referer = QStringLiteral("/browse.php");
  QString cookie;
  QString urlRss = QStringLiteral("/rss.php");
  QString urlDownload = QStringLiteral("/download.php?id=");
  QString idField = QStringLiteral("id");
  bool esRss = true;
};
```

This removes the hardcoded defaults from `rssani_lite.cpp:454-457` and `rss_lite.cpp` where they're currently duplicated.

---

## 5. MEDIUM: Modern C++ Idioms (C++20/23/26)

### 5a. Use `std::format` / Qt string formatting

Throughout the codebase, strings are built with `+` concatenation:

```cpp
QStringLiteral("RSSANI ") + QHostInfo::localHostName() + QDateTime::currentDateTime().toString(...)
```

**Fix:** Use `QString::asprintf()` or `QString::arg()` for structured formatting. When C++26 `std::print` is widely available, adopt `std::format`:

```cpp
// Qt style:
QString().arg("RSSANI %1 %2")
  .arg(QHostInfo::localHostName())
  .arg(QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy hh:mm:ss")));
```

### 5b. Use C++20 ranges/algorithms

**Files:** `rssani_lite.cpp:140-146`, `rssani_lite.cpp:214-221`, `rssani_lite.cpp:237-244`

Manual loops that search/mutate lists can use `<ranges>` and `<algorithm>`:

```cpp
// Current
for (int i = 0; i < lista->size(); ++i) {
  if (lista->at(i)->nombre == QString::fromStdString(regexpOrig)) {
    pos = i;
    break;
  }
}

// Fix (C++20 ranges)
auto it = std::ranges::find_if(*lista, [&](const auto &re) {
  return re.nombre == QString::fromStdString(std::string(regexpOrig));
});
if (it != lista->end()) {
  it->nombre = QString::fromStdString(std::string(regexpDest));
  return true;
}
return false;
```

### 5c. Use `std::chrono` for timer intervals

**File:** `rssani_lite.cpp:77`

```cpp
// Current — magic multiplication
timer.start(tiempo * 60 * 1000);

// Fix
using namespace std::chrono_literals;
timer.start(std::chrono::duration_cast<std::chrono::milliseconds>(
  std::chrono::minutes(tiempo)
).count());
```

Or define a helper:

```cpp
constexpr int minutesToMs(int minutes) { return minutes * 60 * 1000; }
timer.start(minutesToMs(tiempo));
```

### 5d. Replace magic return codes with `enum class`

**File:** `rss_lite.cpp:216-288`, `rss_lite.h:139`

`parseTitle()` returns magic integers -1, 0, 1, 2, 3 with no type safety:

```cpp
int parseTitle(QString seccion, QString titleString, QString linkString, bool fromIrc);
```

**Fix:**

```cpp
enum class MatchResult : int8_t {
  NotMatched = 0,
  Download = 1,
  AlreadyNotified = 2,
  MailSent = 3,
  MailFailed = -1
};

MatchResult parseTitle(QString seccion, QString titleString, QString linkString, bool fromIrc);
```

Then in `miraTitulo()`:

```cpp
switch (result) {
  case MatchResult::Download:
    emit linkCorrecto(link, titulo);
    [[fallthrough]];
  case MatchResult::AlreadyNotified:
    if (recientes.size() > 30) recientes.removeFirst();
    recientes.append(titulo);
    break;
  default:
    break;
}
```

### 5e. Use `QLoggingCategory` instead of raw `qDebug()`

**Files:** All `.cpp` files

Unconditional `qDebug()` calls make it impossible to filter log output at runtime. Every log statement is always active.

**Fix:**

```cpp
// In header or namespace:
Q_LOGGING_CATEGORY(logRss, "rssani.rss")
Q_LOGGING_CATEGORY(logIrc, "rssani.irc")
Q_LOGGING_CATEGORY(logRpc, "rssani.rpc")
Q_LOGGING_CATEGORY(logCore, "rssani.core")

// Usage:
qCDebug(logRss) << "+ Me bajo" << urlTracker.host() << trk->urlRss;
qCDebug(logIrc) << "message:" << nick << channel << message;
```

This allows runtime filtering via `QT_LOGGING_RULES="rssani.rss=true;rssani.irc=false"` or `QLoggingCategory::setFilterRules()`.

### 5f. Use `std::expected` (C++23) or a Result type for error handling

Methods returning `int` or `bool` don't communicate error details. For C++20 compatibility, define a simple Result type:

```cpp
template<typename T, typename E = QString>
class Result {
public:
  Result(T value) : m_value(std::move(value)), m_ok(true) {}
  Result(E error) : m_error(std::move(error)), m_ok(false) {}
  [[nodiscard]] bool ok() const { return m_ok; }
  [[nodiscard]] T value() const { return m_value; }
  [[nodiscard]] E error() const { return m_error; }
private:
  union { T m_value; E m_error; };
  bool m_ok;
};
```

### 5g. RAII wrapper for `QSettings` groups

**Files:** `rssani_lite.cpp:318-391`, `rssani_lite.cpp:395-479`

Repeated `settings->beginGroup()` / `settings->endGroup()` pairs without RAII guarantees. If an exception or early return happens, the group remains open.

**Fix:**

```cpp
class SettingsGroup {
public:
  SettingsGroup(QSettings &s, const QString &group) : m_settings(s) { m_settings.beginGroup(group); }
  ~SettingsGroup() { m_settings.endGroup(); }
  SettingsGroup(const SettingsGroup &) = delete;
  SettingsGroup &operator=(const SettingsGroup &) = delete;
private:
  QSettings &m_settings;
};

// Usage:
{
  SettingsGroup group(*settings, QStringLiteral("principal"));
  settings->setValue(QStringLiteral("fromMail"), values->FromMail());
  // ...
}  // auto endGroup
```

---

## 6. MEDIUM: Security

### 6a. Remove credential logging

**File:** `grpc_server.cpp:159`

```cpp
qDebug() << "USER:" << rss->getRpcUser() << "PASS:" << rss->getRpcPass();
```

This logs RPC credentials in plain text to stderr/log files. Remove this line entirely, or at minimum redact the password:

```cpp
qCDebug(logRpc) << "RPC credentials updated, user:" << rss->getRpcUser();
```

### 6b. Secure gRPC channel

**File:** `grpc_server.cpp:217`

```cpp
builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
```

Uses insecure (plaintext) credentials. For production use, at minimum add TLS:

```cpp
grpc::SslServerCredentialsOptions ssl_opts;
// Configure with certificate and key
builder.AddListeningPort("0.0.0.0:50051", grpc::SslServerCredentials(ssl_opts));
```

Or add a `PonerCredenciales`-based authentication interceptor for a middle ground.

### 6c. Protect stored credentials

QSettings stores `rpcPass`, `pass`, `passkey` in plain-text INI files. At minimum:

1. Use `QSettings::registerFormat()` with a custom read/write that encrypts sensitive fields
2. Or store credentials in the OS keychain via `QKeychain`
3. Or at least base64-encode passwords (obfuscation only, not real security)

### 6d. Input validation on gRPC methods

None of the RPC methods validate input (empty strings, negative indices, out-of-range values). A negative `pos` in `BorrarRegexpI` could crash the application or cause undefined behavior.

**Fix:** Add bounds checking:

```cpp
grpc::Status BorrarRegexpI(..., const rssani::BorrarRegexpIRequest *request, ...) {
  if (request->pos() < 0 || request->pos() >= rss->listaRegexpSnapshot().size()) {
    return grpc::Status(grpc::StatusCode::OUT_OF_RANGE, "Index out of range");
  }
  // ...
}
```

---

## 7. MEDIUM: Code Organization & Single Responsibility

### 7a. Split `rss_lite.cpp` responsibilities (457 lines)

The class handles five distinct concerns:
1. RSS feed fetching (`fetch()`, `readDataRSS()`)
2. XML parsing (`parseXml()`)
3. Regexp matching (`parseTitle()`, `miraTitulo()`)
4. Torrent downloading (`parseLink()`, `readDataTorrent()`)
5. Email notifications (`sendMail()`)

**Proposed refactor:**

| New Class | Responsibility | Methods |
|---|---|---|
| `RssParser` | XML parsing | `parseXml()`, `parseTitle()` |
| `TorrentDownloader` | Download logic | `parseLink()`, `readDataTorrent()` |
| `MatchEngine` | Regexp matching | `miraTitulo()`, `parseTitle()` |
| `MailNotifier` | Email sending | `sendMail()` (wraps `MailSender`) |

`Rss_lite` becomes a coordinator that wires these together.

### 7b. Split `mailsender.cpp` (473 lines)

Separate concerns:
- `SmtpTransport` — low-level TCP socket + SMTP protocol (`read()`, `sendCommand()`)
- `MailComposer` — MIME construction (`mailData()`, `addMimeAttachment()`, `addMimeBody()`)
- `MailSender` — high-level API remains as-is

### 7c. Extract `SettingsRepository` from `rssani_lite`

Reading/writing settings currently occupies ~100 lines of `rssani_lite.cpp` (lines 313-480). Extract to a separate class:

```cpp
class SettingsRepository {
public:
  explicit SettingsRepository(const QString &configPath);
  void load(Values &values, QList<regexp> &regexps, QList<auth> &auths, datosIrc &irc);
  void save(const Values &values, const QList<regexp> &regexps, const QList<auth> &auths, const datosIrc &irc);
private:
  QSettings m_settings;
};
```

---

## 8. MEDIUM: Fix Known Bugs

### 8a. `cambiaTimer()` doesn't update the QTimer

**File:** `rssani_lite.cpp:264-267`

```cpp
void rssani_lite::cambiaTimer(int tiempo) {
  QMutexLocker<QMutex> locker(&mutex);
  this->tiempo = tiempo;  // Only updates the stored value
  // BUG: timer.setInterval() is never called!
}
```

The `verTimer()` returns `timer.interval()` which is set once at construction. Changing `tiempo` doesn't affect the actual QTimer.

**Fix:**

```cpp
void rssani_lite::cambiaTimer(int tiempo) {
  QMutexLocker<QMutex> locker(&mutex);
  this->tiempo = tiempo;
  timer.setInterval(tiempo * 60 * 1000);  // Actually update the timer
}
```

### 8b. Uninitialized variables in signal handler

**File:** `rssani_lite.cpp:23`

```cpp
void rssani_lite::handleSigTerm() {
  snTerm->setEnabled(false);
  char tmp;  // ← uninitialized
  ::read(sigFd[1], &tmp, sizeof(tmp));
  // ...
}
```

While `tmp` is immediately overwritten by `read()`, this is technically UB if `read()` fails. Initialize to zero:

```cpp
char tmp = 0;
```

### 8c. Switch on `parseTitle()` has no `default` case

**File:** `rss_lite.cpp:125-131`

The switch handles cases 1 and 2 but not 0 or -1. While not a bug (no action needed), adding `default: break;` improves readability and silences compiler warnings.

### 8d. Parameter name mismatches between declarations and definitions

**Files noted in AGENTS.md TODO:**
- `rss_lite.h:139` — `titleString`, `linkString` vs `rss_lite.cpp:216` — `titulo`, `enlace`
- `rssani_lite.h:66` — `regexpOrig` vs `rssani_lite.cpp:158` — `pos`
- `rssani_lite.h:73` — `regexpOrig` vs `rssani_lite.cpp:171` — `pos`
- `rssani_lite.h:206` — `msg` vs `rssani_lite.cpp:91` — `subida`

These make code harder to read and may confuse documentation tools. Unify them.

### 8e. Delete `ficheros`/`datos`/`sites` entries during iteration

**File:** `rss_lite.cpp:396-398`

After writing a torrent to disk, entries are removed from the hashes:

```cpp
ficheros.remove(downloadKey);
datos.remove(downloadKey);
sites.remove(downloadKey);
```

This pattern works with `QHash` but is fragile. Consider using `QHash::take()` or a `struct` to group related data per download key:

```cpp
struct DownloadContext {
  QString filename;
  QByteArray data;
  QString site;
};
QHash<QString, DownloadContext> downloads;
```

---

## 9. LOW: Build System Modernization

### 9a. Use `FetchContent` for gRPC/Protobuf

`pkg_check_modules` is the old CMake way. Modern CMake (3.14+) supports `FetchContent` for dependency management:

```cmake
include(FetchContent)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
```

Or build gRPC from source with `FetchContent` for self-contained builds.

### 9b. Modern CMake target-based approach

Currently using `target_include_directories` and `target_link_directories` with manual paths. Modern CMake prefers exporting targets from subprojects:

```cmake
# Instead of:
target_link_directories(rssani PRIVATE ${LIBIRC_INSTALL_DIR}/lib)
target_link_libraries(rssani PRIVATE ircclient irc ...)

# Use:
target_link_libraries(rssani PRIVATE irc::ircclient irc::irc ...)
```

### 9c. Add `CMakePresets.json`

```json
{
  "version": 6,
  "configurePresets": [
    { "name": "debug", "binaryDir": "build/debug", "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" } },
    { "name": "release", "binaryDir": "build/release", "cacheVariables": { "CMAKE_BUILD_TYPE": "Release" } },
    { "name": "test", "inherits": "debug", "cacheVariables": { "RSSANI_BUILD_TESTS": "ON" } }
  ],
  "buildPresets": [
    { "name": "debug", "configurePreset": "debug" },
    { "name": "release", "configurePreset": "release" }
  ],
  "testPresets": [
    { "name": "test", "configurePreset": "test" }
  ]
}
```

### 9d. Enable `compile_commands.json` for tooling

```cmake
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

This enables `clang-tidy`, `clangd`, and other tooling to work correctly.

### 9e. Add `clang-tidy` configuration

Create `.clang-tidy` with the checks already identified in AGENTS.md:

```yaml
Checks: >
  -*,
  modernize-use-auto,
  modernize-use-override,
  modernize-use-equals-default,
  modernize-use-bool-literals,
  modernize-avoid-c-arrays,
  modernize-use-nodiscard,
  bugprone-narrowing-cast,
  cppcoreguidelines-prefer-member-initializer,
  performance-unnecessary-value-param,
  readability-const-return-type,
  modernize-use-override
```

---

## 10. LOW: Testing Improvements

### 10a. Add `QSignalSpy` tests

Currently no tests verify that signals are emitted correctly. Key signals to test:
- `nuevaSubida(seccion, titulo, url)` from `rssani_lite`
- `timeout()` from `rssani_lite`
- `linkCorrecto(link, title)` from `Rss_lite`

```cpp
QSignalSpy spy(&rss, &Rss_lite::linkCorrecto);
// trigger the signal
QCOMPARE(spy.count(), 1);
QCOMPARE(spy.at(0).at(0).toString(), expectedUrl);
```

### 10b. Mock `QNetworkAccessManager` for RSS fetch tests

The `Rss_lite` unit tests currently only test construction and `verUltimo()`. No tests exercise `fetch()`, `readDataRSS()`, `parseXml()`, or `readDataTorrent()` because they depend on network access.

**Fix:** Subclass `QNetworkAccessManager` to return canned replies:

```cpp
class MockNetworkManager : public QNetworkAccessManager {
  Q_OBJECT
public:
  QNetworkReply *get(const QNetworkRequest &req) override;
  void setMockResponse(const QByteArray &data);
};
```

### 10c. Test error paths

- Invalid XML in `parseXml()`
- Network timeouts in `fetch()` / `readDataRSS()`
- Malformed URLs in `parseLink()`
- Empty responses
- Disk-full scenarios in `readDataTorrent()`

### 10d. Test `miraTitulo()` and `parseTitle()` independently

Currently these are private/protected and not unit-tested. Make them accessible for testing (friend class or test interface):

```cpp
class Rss_lite : public QObject {
  friend class TestRssLite;  // Already done in test
  // ...
};
```

### 10e. Property-based testing for `irc_color_strip_from_mirc()`

The IRC color stripping function is a great candidate for property-based testing (e.g., via RapidCheck or Catch2 Generators):

Properties to test:
1. `strip(strip(s)) == strip(s)` — idempotent
2. `strip(s).length() <= s.length()` — never grows
3. `strip(s)` contains no IRC control codes (0x02, 0x03, 0x0F, 0x16, 0x1F)
4. For plain text, `strip(s) == s`

### 10f. Increase integration test coverage

Current integration tests cover CRUD operations but not:
- Error conditions (invalid indices, out-of-range values)
- Concurrent access (multiple gRPC clients)
- Shutdown while fetching

---

## 11. LOW: Minor Modernizations

| Item | Current | Suggested | Files |
|---|---|---|---|
| Double hash lookup | `QHash::contains()` + `QHash::operator[]` in `readDataTorrent()` | Use `QHash::find()` iterator | `rss_lite.cpp` |
| Qt container `at()` | `lista->at(i)->nombre` | Range-based for loops or `(*lista)[i]` | `rssani_lite.cpp`, `rss_lite.cpp` |
| Verbose boolean comparison | `values->Debug() == false` | `!values->Debug()` | `rss_lite.cpp:64`, `rssani_lite.cpp:33` |
| Magic numbers | `timer.start(tiempo * 60 * 1000)` | `constexpr int msPerMinute = 60000;` | `rssani_lite.cpp:77` |
| File separator | `QChar('/')` | `QDir::separator()` or `QFileInfo` join | `rss_lite.cpp:368` |
| Blocking sleep | `sleep(5)` in `on_kick()` | `QTimer::singleShot(5000, ...)` | `myircsession.cpp:107` |
| `QChar` from int | `QChar(0x02)` etc. | Use named constants in an `enum class` | `myircsession.cpp:15-16` |
| Raw string reads | `QString xml(reply->readAll())` | Use `QByteArray` for binary-safe handling of RSS data | `rss_lite.cpp:105` |
| Mismatched Unicode | `QStringLiteral` mixed with `QString::fromStdString` | Pick one convention — prefer `QStringLiteral` for literal strings, `QStringView` for params | Throughout |
| Unused enum values | `mailsender.h:23` — `ISO { utf8 }` single-value enum | Already documented as cleaned up, but consider removing the enum entirely | `mailsender.h` |

---

## Implementation Priority

| Priority | Item | Effort | Impact |
|---|---|---|---|
| 1 | Thread safety — return copies not pointers (1a) | Medium | High — data race bug |
| 2 | Detached gRPC thread — store and join (1b) | Small | High — UB on shutdown |
| 3 | Make `regexp` value type (2b) | Medium | High — eliminates leak/UAF bugs |
| 4 | Inverted bool returns (3a) — **DONE** | Small | Medium — API correctness |
| 5 | `[[fallthrough]]` + enum class for parseTitle (5d, 1c) — **DONE** | Small | Medium — correctness |
| 6 | Remove credential logging (6a) | Tiny | High — security |
| 7 | Fix `cambiaTimer()` bug (8a) | Tiny | High — timer doesn't actually update |
| 8 | Default member initializers (4c, 4d) | Small | Medium — prevents UB |
| 9 | Encapsulate data model (4a, 4b) | Large | Medium — maintainability |
| 10 | `QLoggingCategory` (5e) | Medium | Medium — operational visibility |
| 11 | Input validation in gRPC (6d) | Medium | Medium — security |
| 12 | `std::string_view` params (3c) | Small | Low — performance |
| 13 | Split responsibilities (7a-7c) | Large | Medium — maintainability |
| 14 | Mock network tests (10b) | Large | Medium — test quality |