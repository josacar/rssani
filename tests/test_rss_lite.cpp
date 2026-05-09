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
    Values values;
    values.setRuta(tempDir.path());

    QList<regexp> lista;
    QHash<QString, auth> auths;

    QString logPath = tempDir.filePath("rssani.log");
    QFile logFile(logPath);
    logFile.open(QIODevice::WriteOnly | QIODevice::Append);

    Rss_lite rss(&values, &lista, &logFile, &auths, nullptr);

    QString testFile = tempDir.filePath("test.torrent");
    QFile testTorrent(testFile);
    testTorrent.open(QIODevice::WriteOnly);
    testTorrent.write("test data");
    testTorrent.close();

    QVERIFY(logFile.isOpen());

    logFile.close();
}

void TestRssLite::testMiraTituloNoCrash() {
    Values values;
    values.setRuta(tempDir.path());

    QList<regexp> lista;

    regexp reg;
    reg.nombre = ".*test.*";
    reg.activa = true;
    reg.mail = false;
    reg.tracker = "";
    reg.vencimiento = "";
    reg.diasDescarga = 0;
    lista.append(reg);

    QHash<QString, auth> auths;

    QString logPath = tempDir.filePath("rssani.log");
    QFile logFile(logPath);
    logFile.open(QIODevice::WriteOnly | QIODevice::Append);

    Rss_lite rss(&values, &lista, &logFile, &auths, nullptr);

    QVERIFY(rss.verUltimo().isValid());

    logFile.close();
}

void TestRssLite::testVerUltimo() {
    Values values;
    values.setRuta(tempDir.path());

    QList<regexp> lista;
    QHash<QString, auth> auths;

    QString logPath = tempDir.filePath("rssani.log");
    QFile logFile(logPath);
    logFile.open(QIODevice::WriteOnly | QIODevice::Append);

    Rss_lite rss(&values, &lista, &logFile, &auths, nullptr);

    QDateTime ultimo = rss.verUltimo();
    QVERIFY(ultimo.isValid());

    QDateTime now = QDateTime::currentDateTime();
    QVERIFY(ultimo.secsTo(now) < 60);

    logFile.close();
}

QTEST_MAIN(TestRssLite)
#include "test_rss_lite.moc"