#ifndef MAILSENDER_H
#define MAILSENDER_H

#include <QString>
#include <QStringList>
#include <QObject>
#include <QTcpSocket>
#include <QPointer>
#include <QAuthenticator>

/**
 * @brief SMTP email sender with SSL/TLS support.
 */
class MailSender : public QObject
{
  Q_OBJECT

  public:

    enum Priority {HighPriority, NormalPriority, LowPriority};
    enum ContentType {TextContent, HtmlContent};
    enum Encoding {Encoding_7bit, Encoding_8bit, Encoding_base64};
    enum ISO {utf8};

    /**
     * @brief Constructs a mail sender.
     * @param smtpServer SMTP server hostname.
     * @param from Sender email address.
     * @param to List of recipient email addresses.
     */
    MailSender(const QString &smtpServer, const QString &from, const QStringList &to);
    ~MailSender();

    /**
     * @brief Sends the email.
     * @return True on success, false on failure.
     */
    bool send();

    /// @brief Returns the last error message.
    QString lastError() {return m_lastError;}
    /// @brief Returns the last SMTP command sent.
    QString lastCmd() {return m_lastCmd;}
    /// @brief Returns the last SMTP server response.
    QString lastResponse() {return m_lastResponse;}
    /// @brief Returns the raw mail data of the last send attempt.
    QString lastMailData() {return m_lastMailData;}

    /// @brief Sets the SMTP server hostname.
    void setSmtpServer (const QString &smtpServer) 	{m_smtpServer = smtpServer;}
    /// @brief Sets the SMTP server port.
    void setPort (int port)					        {m_port = port;}
    /// @brief Sets the socket timeout in milliseconds.
    void setTimeout (int timeout)				    {m_timeout = timeout;}
    /// @brief Sets SMTP authentication credentials.
    void setLogin (const QString &login, const QString &passwd)		{m_login = login; m_password = passwd;}
    /// @brief Enables or disables SSL/TLS.
    void setSsl(bool ssl)                           {m_ssl = ssl;}
    /// @brief Sets the CC recipients.
    void setCc (const QStringList &cc) 				{m_cc = cc;}
    /// @brief Sets the BCC recipients.
    void setBcc (const QStringList &bcc) 			{m_bcc = bcc;}
    /// @brief Sets file paths to attach.
    void setAttachments (const QStringList &attachments)	{m_attachments = attachments;}
    /// @brief Sets the Reply-To address.
    void setReplyTo (const QString &replyTo) 		{m_replyTo = replyTo;}
    /// @brief Sets the email priority.
    void setPriority (Priority priority) 			{m_priority = priority;}
    /// @brief Sets the sender email address.
    void setFrom (const QString &from);
    /// @brief Sets the list of recipient email addresses.
    void setTo (const QStringList &to) 				{m_to = to;}
    /// @brief Sets the email subject.
    void setSubject (const QString &subject) 		{m_subject = subject;}
    /// @brief Sets the email body text.
    void setBody (const QString &body) 				{m_body = body;}
    /// @brief Sets the sender display name.
    void setFromName (const QString &fromName)      {m_fromName = fromName;}
    /// @brief Sets the content type (text or HTML).
    void setContentType(ContentType contentType)	{m_contentType = contentType;}
    /// @brief Sets the character set.
    void setISO(ISO iso);
    /// @brief Sets the content transfer encoding.
    void setEncoding(Encoding encoding);
    /// @brief Sets the proxy authenticator.
    void setProxyAuthenticator(const QAuthenticator &authenticator);

    private Q_SLOTS:
      void errorReceived(QAbstractSocket::SocketError socketError);
    void proxyAuthentication(const QNetworkProxy & proxy, QAuthenticator * authenticator);

  private:

    QString getMimeType(QString ext) const;
    QString priorityString() const;
    int addMimeAttachment(QString *pdata, const QString &filename) const;
    void addMimeBody(QString *pdata) const;
    QString mailData() const;
    QString contentType() const;
    bool read(const QString &waitfor);
    bool sendCommand(const QString &cmd, const QString &waitfor);
    void error(const QString &msg);

    QString	    m_smtpServer;
    int         m_port;
    int         m_timeout;
    QString     m_login;
    QString     m_password;
    QPointer<QTcpSocket>  m_socket;
    bool        m_ssl;
    QAuthenticator m_authenticator;
    QString     m_lastError;
    QString     m_lastCmd;
    QString     m_lastResponse;
    QString     m_lastMailData;

    QString	    m_from;
    QStringList	m_to;
    QString	    m_subject;
    QString	    m_body;
    QStringList m_cc;
    QStringList m_bcc;
    QStringList m_attachments;
    QString	    m_fromName;
    QString	    m_replyTo;
    Priority	m_priority;
    ContentType m_contentType;
    QString	    m_encoding;
    QString	    m_charset;
    QString	    m_bodyCodec;
    QString	    m_confirmTo;
};

#endif
