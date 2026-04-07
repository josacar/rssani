#include <QtTest/QtTest>
#include "mailsender.h"

class TestMailSender : public QObject {
    Q_OBJECT

private slots:
    void testConstructor();
    void testSettersAndGetters();
    void testContentType();
    void testPriority();
    void testEncoding();
    void testISO();
    void testMailData();
};

void TestMailSender::testConstructor() {
    QStringList recipients = {"recipient@example.com"};
    MailSender sender("smtp.example.com", "sender@example.com", recipients);
    
    QCOMPARE(sender.lastError(), QString());
    // Default values set by constructor
}

void TestMailSender::testSettersAndGetters() {
    QStringList recipients = {"to@example.com"};
    MailSender sender("smtp.example.com", "from@example.com", recipients);
    
    sender.setSmtpServer("smtp.newserver.com");
    QCOMPARE(sender.lastError(), QString()); // No error yet
    
    sender.setPort(465);
    sender.setTimeout(60000);
    sender.setLogin("user", "pass");
    sender.setSsl(true);
    
    QStringList cc = {"cc@example.com"};
    QStringList bcc = {"bcc@example.com"};
    sender.setCc(cc);
    sender.setBcc(bcc);
    
    sender.setReplyTo("reply@example.com");
    sender.setFromName("Test User");
    sender.setSubject("Test Subject");
    sender.setBody("Test Body");
    
    // Verify through lastError/lastCmd (no direct getters for most)
    // We're testing that setters don't crash
}

void TestMailSender::testContentType() {
    QStringList recipients = {"to@example.com"};
    MailSender sender("smtp.example.com", "from@example.com", recipients);
    
    sender.setContentType(MailSender::TextContent);
    sender.setContentType(MailSender::HtmlContent);
    
    // No crash, setter works
}

void TestMailSender::testPriority() {
    QStringList recipients = {"to@example.com"};
    MailSender sender("smtp.example.com", "from@example.com", recipients);
    
    sender.setPriority(MailSender::HighPriority);
    sender.setPriority(MailSender::NormalPriority);
    sender.setPriority(MailSender::LowPriority);
    
    // No crash, setter works
}

void TestMailSender::testEncoding() {
    QStringList recipients = {"to@example.com"};
    MailSender sender("smtp.example.com", "from@example.com", recipients);
    
    sender.setEncoding(MailSender::Encoding_7bit);
    sender.setEncoding(MailSender::Encoding_8bit);
    sender.setEncoding(MailSender::Encoding_base64);
    
    // No crash, setter works
}

void TestMailSender::testISO() {
    QStringList recipients = {"to@example.com"};
    MailSender sender("smtp.example.com", "from@example.com", recipients);
    
    sender.setISO(MailSender::utf8);
    
    // No crash, setter works
}

void TestMailSender::testMailData() {
    QStringList recipients = {"to@example.com"};
    MailSender sender("smtp.example.com", "from@example.com", recipients);
    
    sender.setSubject("Test Subject");
    sender.setBody("Test Body");
    sender.setFromName("Sender Name");
    
    // Set content type and encoding
    sender.setContentType(MailSender::TextContent);
    sender.setEncoding(MailSender::Encoding_7bit);
    sender.setISO(MailSender::utf8);
    
    // Note: mailData() is private, but we can test that the object is properly configured
    // Actual SMTP sending would require a real server, so we test configuration only
}

QTEST_MAIN(TestMailSender)
#include "test_mail_sender.moc"
