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
    // Test that datosIrc struct has proper defaults and can be overridden
    datosIrc datos;
    QCOMPARE(datos.activo, false);
    QCOMPARE(datos.server, QString("irc.irc-hispano.org"));
    QCOMPARE(datos.port, 6667);
    QCOMPARE(datos.channels, QStringList{QStringLiteral("#PuntoTorrent")});
    QCOMPARE(datos.botNick, QString("PuntoTorrent"));
    QCOMPARE(datos.debug, false);

    // Override all fields
    datos.activo = true;
    datos.nick = "testnick";
    datos.user = "testuser";
    datos.name = "Test User";
    datos.server = "irc.example.com";
    datos.port = 7000;
    datos.channels = {"#channel1", "#channel2"};
    datos.botNick = "BotNick";
    datos.debug = true;

    QCOMPARE(datos.activo, true);
    QCOMPARE(datos.nick, QString("testnick"));
    QCOMPARE(datos.user, QString("testuser"));
    QCOMPARE(datos.name, QString("Test User"));
    QCOMPARE(datos.server, QString("irc.example.com"));
    QCOMPARE(datos.port, 7000);
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
    // Test bold code (IrcCode::Bold)
    QString input = QString("Hello %1World").arg(QChar(static_cast<ushort>(IrcCode::Bold)));
    QString result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));
}

void TestMyIrcSession::testIrcColorStripFromMircColor() {
    // Test color code (IrcCode::Color) with color number
    // Format: IrcCode::Color followed by color number (1-2 digits)
    QString input = QString("Hello %104World").arg(QChar(static_cast<ushort>(IrcCode::Color)));
    QString result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));

    // Test with two-digit color number
    input = QString("Hello %112World").arg(QChar(static_cast<ushort>(IrcCode::Color)));
    result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));

    // Test with foreground,background colors
    input = QString("Hello %104,01World").arg(QChar(static_cast<ushort>(IrcCode::Color)));
    result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));
}

void TestMyIrcSession::testIrcColorStripFromMircUnderline() {
    // Test underline code (IrcCode::Underline)
    QString input = QString("Hello %1World").arg(QChar(static_cast<ushort>(IrcCode::Underline)));
    QString result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));
}

void TestMyIrcSession::testIrcColorStripFromMircReverse() {
    // Test reverse code (IrcCode::Reverse)
    QString input = QString("Hello %1World").arg(QChar(static_cast<ushort>(IrcCode::Reverse)));
    QString result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));
}

void TestMyIrcSession::testIrcColorStripFromMircReset() {
    // Test reset code (IrcCode::Reset)
    QString input = QString("Hello %1World").arg(QChar(static_cast<ushort>(IrcCode::Reset)));
    QString result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QString("Hello World"));
}

void TestMyIrcSession::testIrcColorStripFromMircComplex() {
    // Test complex string with multiple IRC codes
    // Bold + Color + Underline
    QString input = QStringLiteral("%1Hello%2 %3World%4").arg(
        QChar(static_cast<ushort>(IrcCode::Bold)),
        QString(QChar(static_cast<ushort>(IrcCode::Color))) + QStringLiteral("04"),
        QChar(static_cast<ushort>(IrcCode::Underline)),
        QChar(static_cast<ushort>(IrcCode::Reset))
    );
    QString result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QStringLiteral("Hello World"));

    // Test multiple codes in sequence
    input = QStringLiteral("%1%2%3Test%4%5%6").arg(
        QChar(static_cast<ushort>(IrcCode::Bold)),
        QChar(static_cast<ushort>(IrcCode::Underline)),
        QChar(static_cast<ushort>(IrcCode::Reverse)),
        QChar(static_cast<ushort>(IrcCode::Reset)),
        QChar(static_cast<ushort>(IrcCode::Bold)),
        QChar(static_cast<ushort>(IrcCode::Underline))
    );
    result = MyIrcSession::irc_color_strip_from_mirc(input);
    QCOMPARE(result, QStringLiteral("Test"));
}

QTEST_MAIN(TestMyIrcSession)
#include "test_myirc_session.moc"
