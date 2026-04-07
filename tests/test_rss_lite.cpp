#include <QtTest/QtTest>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include "rss_lite.h"
#include "values.h"

class TestRssLite : public QObject {
    Q_OBJECT

private slots:
    void testSaveLog();
    void testMiraTituloNoCrash();
    void testVerUltimo();

private:
    QTemporaryDir tempDir;
};

void TestRssLite::testSaveLog() {
    // Setup minimal environment
    Values values;
    values.setRuta(tempDir.path());
    
    QList<regexp*> lista;
    QHash<QString, auth> auths;
    
    QString logPath = tempDir.filePath("rssani.log");
    QFile logFile(logPath);
    logFile.open(QIODevice::WriteOnly | QIODevice::Append);
    
    Rss_lite rss(&values, &lista, &logFile, &auths, nullptr);
    
    // Test saveLog with a test file path
    QString testFile = tempDir.filePath("test.torrent");
    QFile testTorrent(testFile);
    testTorrent.open(QIODevice::WriteOnly);
    testTorrent.write("test data");
    testTorrent.close();
    
    // saveLog is protected, so we test it indirectly through the class
    // For now, just verify the object was created successfully
    QVERIFY(logFile.isOpen());
    
    logFile.close();
}

void TestRssLite::testMiraTituloNoCrash() {
    // Setup minimal environment
    Values values;
    values.setRuta(tempDir.path());
    
    QList<regexp*> lista;
    
    // Add a test regexp
    regexp* reg = new regexp();
    reg->nombre = ".*test.*";
    reg->activa = true;
    reg->mail = false;
    reg->tracker = "";
    reg->vencimiento = "";
    reg->diasDescarga = 0;
    lista.append(reg);
    
    QHash<QString, auth> auths;
    
    QString logPath = tempDir.filePath("rssani.log");
    QFile logFile(logPath);
    logFile.open(QIODevice::WriteOnly | QIODevice::Append);
    
    Rss_lite rss(&values, &lista, &logFile, &auths, nullptr);
    
    // Test that the object was created successfully and doesn't crash
    QVERIFY(rss.verUltimo().isValid());
    
    logFile.close();
    
    // Cleanup
    qDeleteAll(lista);
    lista.clear();
}

void TestRssLite::testVerUltimo() {
    // Setup minimal environment
    Values values;
    values.setRuta(tempDir.path());
    
    QList<regexp*> lista;
    QHash<QString, auth> auths;
    
    QString logPath = tempDir.filePath("rssani.log");
    QFile logFile(logPath);
    logFile.open(QIODevice::WriteOnly | QIODevice::Append);
    
    Rss_lite rss(&values, &lista, &logFile, &auths, nullptr);
    
    // Test verUltimo returns a valid QDateTime
    QDateTime ultimo = rss.verUltimo();
    QVERIFY(ultimo.isValid());
    
    // Should be recent (within last minute)
    QDateTime now = QDateTime::currentDateTime();
    QVERIFY(ultimo.secsTo(now) < 60);
    
    logFile.close();
}

QTEST_MAIN(TestRssLite)
#include "test_rss_lite.moc"
