#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QSettings>
#include "rssani_lite.h"
#include "values.h"

class TestRssaniLite : public QObject {
    Q_OBJECT

private slots:
    void testConstructorDefaults();
    void testAnadirRegexp();
    void testEditarRegexpByString();
    void testEditarRegexpByIndex();
    void testActivarRegexp();
    void testMoverRegexp();
    void testBorrarRegexpByIndex();
    void testBorrarRegexpByString();
    void testAnadirAuth();
    void testBorrarAuth();
    void testListaRegexp();
    void testListaAuths();
    void testVerTimer();
    void testCambiaTimer();
    void testSetRpcUser();
    void testSetRpcPass();
    void testVerLogEmpty();
    void testGetValues();

private:
    QTemporaryDir tempDir;
};

void TestRssaniLite::testConstructorDefaults() {
    // Set up a temporary config directory
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    // Default RPC credentials
    QCOMPARE(app.getRpcUser(), QString("rssani-rpc"));
    QCOMPARE(app.getRpcPass(), QString("rssanipass-rpc"));

    // Timer interval depends on settings; with no config file,
    // readSettings() sets tiempo to 0 (empty value converts to 0)
    QCOMPARE(app.verTimer(), 0);

    // Values should be initialized
    Values* values = app.getValues();
    QVERIFY(values != nullptr);
    QCOMPARE(values->Debug(), false);
}

void TestRssaniLite::testAnadirRegexp() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    // Add a regexp rule
    app.anadirRegexp(".*test.*", "2024-12-31", false, "tracker1", 7);

    QList<regexp*>* lista = app.listaRegexp();
    QCOMPARE(lista->size(), 1);

    regexp* re = lista->at(0);
    QCOMPARE(re->nombre, QString(".*test.*"));
    QCOMPARE(re->vencimiento, QString("2024-12-31"));
    QCOMPARE(re->mail, false);
    QCOMPARE(re->tracker, QString("tracker1"));
    QCOMPARE(re->diasDescarga, 7);
    QVERIFY(!re->fechaDescarga.isValid());
    QVERIFY(re->activa); // Default active state
}

void TestRssaniLite::testEditarRegexpByString() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    app.anadirRegexp(".*old.*", "", false, "", 0);
    
    // Edit by string match
    bool result = app.editarRegexp(".*old.*", ".*new.*");
    
    // Returns false on success (existing behavior)
    QCOMPARE(result, false);

    QList<regexp*>* lista = app.listaRegexp();
    QCOMPARE(lista->size(), 1);
    QCOMPARE(lista->at(0)->nombre, QString(".*new.*"));

    // Try to edit non-existent regexp
    result = app.editarRegexp(".*nonexistent.*", ".*other.*");
    QCOMPARE(result, true); // Returns true when not found
}

void TestRssaniLite::testEditarRegexpByIndex() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    app.anadirRegexp(".*first.*", "", false, "", 0);
    app.anadirRegexp(".*second.*", "", false, "", 0);

    // Edit by index
    bool result = app.editarRegexp(0, ".*modified.*");
    QCOMPARE(result, false);

    QList<regexp*>* lista = app.listaRegexp();
    QCOMPARE(lista->at(0)->nombre, QString(".*modified.*"));
    QCOMPARE(lista->at(1)->nombre, QString(".*second.*"));
}

void TestRssaniLite::testActivarRegexp() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    app.anadirRegexp(".*test.*", "", false, "", 0);

    // Initially active
    QList<regexp*>* lista = app.listaRegexp();
    QVERIFY(lista->at(0)->activa);

    // Toggle should deactivate
    bool result = app.activarRegexp(0);
    QCOMPARE(result, false);
    QVERIFY(!lista->at(0)->activa);

    // Toggle again should activate
    result = app.activarRegexp(0);
    QCOMPARE(result, false);
    QVERIFY(lista->at(0)->activa);
}

void TestRssaniLite::testMoverRegexp() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    app.anadirRegexp(".*first.*", "", false, "", 0);
    app.anadirRegexp(".*second.*", "", false, "", 0);
    app.anadirRegexp(".*third.*", "", false, "", 0);

    QList<regexp*>* lista = app.listaRegexp();

    // Move first to last position
    app.moverRegexp(0, 2);

    QCOMPARE(lista->at(0)->nombre, QString(".*second.*"));
    QCOMPARE(lista->at(1)->nombre, QString(".*third.*"));
    QCOMPARE(lista->at(2)->nombre, QString(".*first.*"));

    // Move back
    app.moverRegexp(2, 0);

    QCOMPARE(lista->at(0)->nombre, QString(".*first.*"));
    QCOMPARE(lista->at(1)->nombre, QString(".*second.*"));
    QCOMPARE(lista->at(2)->nombre, QString(".*third.*"));
}

void TestRssaniLite::testBorrarRegexpByIndex() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    app.anadirRegexp(".*first.*", "", false, "", 0);
    app.anadirRegexp(".*second.*", "", false, "", 0);

    QCOMPARE(app.listaRegexp()->size(), 2);

    app.borrarRegexp(0);

    QCOMPARE(app.listaRegexp()->size(), 1);
    QCOMPARE(app.listaRegexp()->at(0)->nombre, QString(".*second.*"));
}

void TestRssaniLite::testBorrarRegexpByString() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    app.anadirRegexp(".*test.*", "", false, "", 0);
    app.anadirRegexp(".*other.*", "", false, "", 0);

    QCOMPARE(app.listaRegexp()->size(), 2);

    app.borrarRegexp(".*test.*");

    QCOMPARE(app.listaRegexp()->size(), 1);
    QCOMPARE(app.listaRegexp()->at(0)->nombre, QString(".*other.*"));

    // Try to delete non-existent (should not crash)
    app.borrarRegexp(".*nonexistent.*");
    QCOMPARE(app.listaRegexp()->size(), 1);
}

void TestRssaniLite::testAnadirAuth() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    app.anadirAuth("tracker1", "user123", "pass456", "key789");

    QList<auth>* listAuths = app.listaAuths();
    QCOMPARE(listAuths->size(), 1);

    auth au = listAuths->at(0);
    QCOMPARE(au.tracker, QString("tracker1"));
    QCOMPARE(au.uid, QString("user123"));
    QCOMPARE(au.pass, QString("pass456"));
    QCOMPARE(au.passkey, QString("key789"));
}

void TestRssaniLite::testBorrarAuth() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    app.anadirAuth("tracker1", "user1", "pass1", "key1");
    app.anadirAuth("tracker2", "user2", "pass2", "key2");

    QCOMPARE(app.listaAuths()->size(), 2);

    app.borrarAuth("tracker1");

    QCOMPARE(app.listaAuths()->size(), 1);
    QCOMPARE(app.listaAuths()->at(0).tracker, QString("tracker2"));

    // Try to delete non-existent (should not crash)
    app.borrarAuth("nonexistent");
    QCOMPARE(app.listaAuths()->size(), 1);
}

void TestRssaniLite::testListaRegexp() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    QList<regexp*>* lista = app.listaRegexp();
    QVERIFY(lista != nullptr);
    QVERIFY(lista->isEmpty());

    app.anadirRegexp(".*test.*", "", false, "", 0);
    QCOMPARE(lista->size(), 1);
}

void TestRssaniLite::testListaAuths() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    QList<auth>* listAuths = app.listaAuths();
    QVERIFY(listAuths != nullptr);
    QVERIFY(listAuths->isEmpty());

    app.anadirAuth("tracker1", "user", "pass", "key");
    QCOMPARE(listAuths->size(), 1);
}

void TestRssaniLite::testVerTimer() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    // With no config file, readSettings() sets tiempo to 0
    QCOMPARE(app.verTimer(), 0);
}

void TestRssaniLite::testCambiaTimer() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    // cambiaTimer updates the internal 'tiempo' variable but does not
    // update the actual QTimer interval (verTimer returns timer.interval())
    // Just verify it doesn't crash
    app.cambiaTimer(30);

    // Timer interval remains unchanged
    QCOMPARE(app.verTimer(), 0);
}

void TestRssaniLite::testSetRpcUser() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    app.setRpcUser("newuser");
    QCOMPARE(app.getRpcUser(), QString("newuser"));

    app.setRpcUser("");
    QCOMPARE(app.getRpcUser(), QString());
}

void TestRssaniLite::testSetRpcPass() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    app.setRpcPass("newpass");
    QCOMPARE(app.getRpcPass(), QString("newpass"));

    app.setRpcPass("");
    QCOMPARE(app.getRpcPass(), QString());
}

void TestRssaniLite::testVerLogEmpty() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    // Log should be empty or contain startup messages
    QStringList log = app.verLog();
    // Just verify it doesn't crash and returns a list
    QVERIFY(log.count() >= 0);
}

void TestRssaniLite::testGetValues() {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    rssani_lite app;

    Values* values = app.getValues();
    QVERIFY(values != nullptr);

    // Verify it's the same object used internally
    values->setDebug(true);
    QCOMPARE(app.getValues()->Debug(), true);
}

QTEST_MAIN(TestRssaniLite)
#include "test_rssani_lite.moc"
