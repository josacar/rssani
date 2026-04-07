#include <QtTest/QtTest>
#include "myircsession.h"

class TestMyIrcSession : public QObject {
    Q_OBJECT

private slots:
    void testDatosIrcStruct();
    void testIrcColorStripFromMirc();
    void testIrcColorStripFromMircEmpty();
    void testIrcColorStripFromMircBold();
    void testIrcColorStripFromMircColor();
    void testIrcColorStripFromMircUnderline();
    void testIrcColorStripFromMircReverse();
    void testIrcColorStripFromMircReset();
    void testIrcColorStripFromMircComplex();
};

void TestMyIrcSession::testDatosIrcStruct() {
    // Test that datosIrc struct can be initialized properly
    datosIrc datos;
    datos.activo = true;
    datos.nick = "testnick";
    datos.user = "testuser";
    datos.name = "Test User";
    datos.server = "irc.example.com";
    datos.port = 6667;
    datos.channels << "#channel1" << "#channel2";
    datos.botNick = "BotNick";
    datos.debug = true;

    QCOMPARE(datos.activo, true);
    QCOMPARE(datos.nick, QString("testnick"));
    QCOMPARE(datos.user, QString("testuser"));
    QCOMPARE(datos.name, QString("Test User"));
    QCOMPARE(datos.server, QString("irc.example.com"));
    QCOMPARE(datos.port, 6667);
    QCOMPARE(datos.channels.size(), 2);
    QCOMPARE(datos.botNick, QString("BotNick"));
    QCOMPARE(datos.debug, true);
}

void TestMyIrcSession::testIrcColorStripFromMirc() {
    // Test basic string without any IRC codes
    QString input = "Hello World";
    QString result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));
}

void TestMyIrcSession::testIrcColorStripFromMircEmpty() {
    // Test empty string
    QString input = "";
    QString result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString(""));
}

void TestMyIrcSession::testIrcColorStripFromMircBold() {
    // Test bold code (0x02)
    QString input = QString("Hello %1World").arg(QChar(0x02));
    QString result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));
}

void TestMyIrcSession::testIrcColorStripFromMircColor() {
    // Test color code (0x03) with color number
    // Format: 0x03 followed by color number (1-2 digits)
    QString input = QString("Hello %104World").arg(QChar(0x03));
    QString result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));

    // Test with two-digit color number
    input = QString("Hello %112World").arg(QChar(0x03));
    result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));

    // Test with foreground,background colors
    input = QString("Hello %104,01World").arg(QChar(0x03));
    result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));
}

void TestMyIrcSession::testIrcColorStripFromMircUnderline() {
    // Test underline code (0x1F)
    QString input = QString("Hello %1World").arg(QChar(0x1F));
    QString result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));
}

void TestMyIrcSession::testIrcColorStripFromMircReverse() {
    // Test reverse code (0x16)
    QString input = QString("Hello %1World").arg(QChar(0x16));
    QString result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));
}

void TestMyIrcSession::testIrcColorStripFromMircReset() {
    // Test reset code (0x0F)
    QString input = QString("Hello %1World").arg(QChar(0x0F));
    QString result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));
}

void TestMyIrcSession::testIrcColorStripFromMircComplex() {
    // Test complex string with multiple IRC codes
    // Bold + Color + Underline
    QString input = QString("%1Hello%2 %3World%4").arg(
        QChar(0x02),  // Bold
        QChar(0x03) + "04",  // Color
        QChar(0x1F),  // Underline
        QChar(0x0F)   // Reset
    );
    QString result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));

    // Test multiple codes in sequence
    input = QString("%1%2%3Test%4%5%6").arg(
        QChar(0x02),  // Bold
        QChar(0x1F),  // Underline
        QChar(0x16),  // Reverse
        QChar(0x0F),  // Reset
        QChar(0x02),  // Bold again
        QChar(0x1F)   // Underline again
    );
    result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Test"));
}

QTEST_MAIN(TestMyIrcSession)
#include "test_myirc_session.moc"
