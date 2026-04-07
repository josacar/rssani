#include <QtTest/QtTest>
#include "values.h"

class TestValues : public QObject {
    Q_OBJECT

private slots:
    void testDefaultValues();
    void testSetRuta();
    void testSetFromMail();
    void testSetToMail();
    void testSetFecha();
    void testSetDebug();
    void testSmtpSettings();
    void testFilledValues();
};

void TestValues::testDefaultValues() {
    Values values;
    
    QCOMPARE(values.Ruta(), QString());
    QCOMPARE(values.FromMail(), QString());
    QCOMPARE(values.ToMail(), QString());
    QCOMPARE(values.Fecha(), QString());
    QCOMPARE(values.Debug(), false);
    QCOMPARE(values.SmtpPort(), 587);
    QCOMPARE(values.SmtpServer(), QString());
    QCOMPARE(values.SmtpLogin(), QString());
    QCOMPARE(values.SmtpPass(), QString());
    QCOMPARE(values.filledValues(), false);
}

void TestValues::testSetRuta() {
    Values values;
    
    QString path = "/tmp/torrents";
    values.setRuta(path);
    
    QCOMPARE(values.Ruta(), path);
    QCOMPARE(values.filledValues(), true);
    
    // Test empty path
    values.setRuta("");
    QCOMPARE(values.Ruta(), QString());
    QCOMPARE(values.filledValues(), false);
}

void TestValues::testSetFromMail() {
    Values values;
    
    QString email = "user@example.com";
    values.setFromMail(email);
    
    QCOMPARE(values.FromMail(), email);
}

void TestValues::testSetToMail() {
    Values values;
    
    QString email = "recipient@example.com";
    values.setToMail(email);
    
    QCOMPARE(values.ToMail(), email);
}

void TestValues::testSetFecha() {
    Values values;
    
    QString date = "01/01/2024 12:00:00";
    values.setFecha(date);
    
    QCOMPARE(values.Fecha(), date);
}

void TestValues::testSetDebug() {
    Values values;
    
    values.setDebug(true);
    QCOMPARE(values.Debug(), true);
    
    values.setDebug(false);
    QCOMPARE(values.Debug(), false);
}

void TestValues::testSmtpSettings() {
    Values values;
    
    values.setSmtpServer("smtp.example.com");
    values.setSmtpLogin("user");
    values.setSmtpPass("password");
    values.setSmtpPort(465);
    
    QCOMPARE(values.SmtpServer(), QString("smtp.example.com"));
    QCOMPARE(values.SmtpLogin(), QString("user"));
    QCOMPARE(values.SmtpPass(), QString("password"));
    QCOMPARE(values.SmtpPort(), 465);
    
    // Test default port
    Values values2;
    QCOMPARE(values2.SmtpPort(), 587);
}

void TestValues::testFilledValues() {
    Values values;
    
    // Initially not filled
    QCOMPARE(values.filledValues(), false);
    
    // Set ruta
    values.setRuta("/tmp/test");
    QCOMPARE(values.filledValues(), true);
    
    // Empty ruta
    values.setRuta("");
    QCOMPARE(values.filledValues(), false);
}

QTEST_MAIN(TestValues)
#include "test_values.moc"
