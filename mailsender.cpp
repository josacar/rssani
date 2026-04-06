#include "mailsender.h"
#include <QString>
#include <QSslSocket>
#include <QDataStream>
#include <QTextStream>
#include <QByteArray>
#include <QDateTime>
#include <QRandomGenerator>
#include <QFile>
#include <QFileInfo>
#include <QHostInfo>
#include <ctime>

enum QDataStreamError {
    BAD_FILE_FORMAT,
    BAD_FILE_TOO_OLD,
    BAD_FILE_TOO_NEW
};

static QString encodeBase64( QString s )
{
  QByteArray text;
  text.append(s.toUtf8());
  return QString::fromUtf8(text.toBase64());
}


static QString timeStamp()
{
  QTime now = QTime::currentTime();
  QDate today = QDate::currentDate();
  QStringList monthList = QStringList() << QStringLiteral("Jan") << QStringLiteral("Feb") << QStringLiteral("Mar")<<QStringLiteral("Apr")<< QStringLiteral("May") <<QStringLiteral("Jun")<<QStringLiteral("Jul")<<QStringLiteral("Aug")<<QStringLiteral("Sep")<<QStringLiteral("Oct")<<QStringLiteral("Nov")<<QStringLiteral("Dec");
  QString month = monthList.value(today.month()-1);
  QString day = QString::number(today.day());
  QString year = QString::number(today.year());
  QString result = QString( QStringLiteral("Date: %1 %2 %3 %4") ).arg(day, month, year, now.toString(QStringLiteral("hh:mm:ss")));
  return result;
}

static QString createBoundary()
{
  QByteArray hash = QCryptographicHash::hash(QString(QString::number(QRandomGenerator::global()->generate())).toUtf8(),QCryptographicHash::Md5);
  QString boundary = QString::fromUtf8(hash.toHex());
  boundary.truncate(26);
  boundary.prepend(QStringLiteral("----=_NextPart_"));
  return boundary;
}


MailSender::MailSender(const QString &smtpServer, const QString &from, const QStringList &to)
{
  setSmtpServer(smtpServer);
  setPort(25);
  setTimeout(30000);
  setFrom(from);
  setTo(to);
  setSubject(QStringLiteral("(no subject)"));
  setPriority (NormalPriority);
  setContentType(TextContent);
  setEncoding(Encoding_7bit);
  setISO(utf8);
  setSsl(false);
}

MailSender::~MailSender()
{
  if(m_socket) {
    delete m_socket;
  }
}

void MailSender::setFrom(const QString &from)
{
  m_from = from;
  m_fromName = from;
  m_replyTo = from;
}

void MailSender::setISO(ISO iso)
{
  switch(iso) {
    case utf8:     m_charset = QStringLiteral("utf-8"); m_bodyCodec = QStringLiteral("UTF-8"); break;
  }
}

void MailSender::setEncoding(Encoding encoding)
{
  switch(encoding) {
    case Encoding_7bit:     m_encoding = QStringLiteral("7bit"); break;
    case Encoding_8bit:     m_encoding = QStringLiteral("8bit"); break;
    case Encoding_base64:    m_encoding = QStringLiteral("base64"); break;
  }
}


QString MailSender::contentType() const
{
  switch(m_contentType) {
    case HtmlContent:            return QStringLiteral("text/html");
    case TextContent:
    default:              return QStringLiteral("text/plain");
  }
}

QString MailSender::priorityString() const
{
  QString res;

  switch(m_priority) {
    case LowPriority:
      res.append(QStringLiteral("X-Priority: 5\n"));
      res.append(QStringLiteral("Priority: Non-Urgent\n"));
      res.append(QStringLiteral("X-MSMail-Priority: Low\n"));
      res.append(QStringLiteral("Importance: low\n"));
      break;
    case HighPriority:
      res.append(QStringLiteral("X-Priority: 1\n"));
      res.append(QStringLiteral("Priority: Urgent\n"));
      res.append(QStringLiteral("X-MSMail-Priority: High\n"));
      res.append(QStringLiteral("Importance: high\n"));
      break;
    default:
      res.append(QStringLiteral("X-Priority: 3\n"));
      res.append(QStringLiteral("    X-MSMail-Priority: Normal\n"));
  }

  return res;
}

int MailSender::addMimeAttachment(QString *pdata, const QString &filename) const
{
  QFile file(filename);
  bool ok = file.open(QIODevice::ReadOnly);
  if(!ok) {
    pdata->append(QStringLiteral("Error attaching file: ") + filename + QStringLiteral("\r\n"));
    return -1;
  }

  QFileInfo fileinfo(filename);
  QString type = getMimeType(fileinfo.suffix());
  pdata->append(QStringLiteral("Content-Type: ") + type + QStringLiteral(";\n"));
  pdata->append(QStringLiteral("  name=") + fileinfo.fileName() + QStringLiteral("\n"));

  QString tt = fileinfo.fileName();

  pdata->append(QStringLiteral("Content-Transfer-Encoding: base64\n"));
  pdata->append(QStringLiteral("Content-Disposition: attachment\n"));
  pdata->append(QStringLiteral("  filename=") + fileinfo.fileName() + QStringLiteral("\n\n"));
  QByteArray fileData = file.readAll();
  QString encodedFile = QString::fromUtf8(fileData.toBase64());
  pdata->append(encodedFile);
  pdata->append(QString("\r\n\n"));

  return 0;
}

void MailSender::addMimeBody(QString *pdata) const
{
  pdata->append(QStringLiteral("Content-Type: ") + contentType() + QStringLiteral(";\n"));
  pdata->append(QStringLiteral("  charset=") + m_charset + QStringLiteral("\r\n"));
  pdata->append(QStringLiteral("Content-Transfer-Encoding: ") + m_encoding + QStringLiteral("\r\n"));
  pdata->append(QStringLiteral("\r\n\n"));

  if ( m_contentType == HtmlContent ) {
    pdata->append(QStringLiteral("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\r\n"));
    pdata->append(QStringLiteral("<HTML><HEAD>\r\n"));
    pdata->append(QStringLiteral("<META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; charset=") + m_charset + QStringLiteral("\">\r\n"));
    pdata->append(QStringLiteral("<META content=\"MSHTML 6.00.2900.2802\" name=GENERATOR>\r\n"));
    pdata->append(QStringLiteral("<STYLE></STYLE>\r\n"));
    pdata->append(QStringLiteral("</head>\r\n"));
    pdata->append(QStringLiteral("<body bgColor=#ffffff>\r\n"));
  }

  pdata->append(m_body + QStringLiteral("\r\n"));

  if ( m_contentType == HtmlContent ) {
    pdata->append(QStringLiteral("<DIV>&nbsp;</DIV></body></html>\r\n\n"));
  }
}

QString MailSender::mailData() const
{
  QString data;

  QByteArray hash = QCryptographicHash::hash(QString(QString::number(QRandomGenerator::global()->generate())).toUtf8(),QCryptographicHash::Md5);
  QString id = QString::fromUtf8(hash.toHex());
  data.append(QStringLiteral("Message-ID: ") + id + QStringLiteral("@") + QHostInfo::localHostName() + QStringLiteral("\n"));
  data.append(QStringLiteral("From: \"") + m_from + QStringLiteral("\" <") + m_fromName + QStringLiteral(">\n"));

  if ( !m_to.isEmpty() ) {
    data.append(QStringLiteral("To: "));
    bool first = true;
    for (const auto &val : m_to) {
      if(!first) {
        data.append(QStringLiteral(","));
      }
      data.append(QStringLiteral("<") + val + QStringLiteral(">"));
      first = false;
    }
    data.append(QStringLiteral("\n"));
  }

  if ( !m_cc.isEmpty() ) {
    data.append(QStringLiteral("Cc: "));
    bool first = true;
    for (const auto &val : m_cc) {
      if(!first) {
        data.append(QStringLiteral(","));
      }
      data.append(val);
      first = false;
    }
    data.append(QStringLiteral("\n"));
  }

  data.append(QStringLiteral("Subject: ") + m_subject + QStringLiteral("\n"));
  data.append(timeStamp() + QStringLiteral("\n"));
  data.append(QStringLiteral("MIME-Version: 1.0\n"));

  QString boundary = createBoundary();    
  data.append(QStringLiteral("Content-Type: Multipart/Mixed; boundary=\"") + boundary + QStringLiteral("\"\n"));
  data.append(priorityString());
  data.append(QStringLiteral("X-Mailer: QT4\r\n"));

  if ( ! m_confirmTo.isEmpty() ) {
    data.append(QStringLiteral("Disposition-Notification-To: ") + m_confirmTo + QStringLiteral("\n"));
  }

  if ( ! m_replyTo.isEmpty() && m_confirmTo != m_from ) {
    data.append(QStringLiteral("Reply-to: ") + m_replyTo + QStringLiteral("\n"));
    data.append(QStringLiteral("Return-Path: <") + m_replyTo + QStringLiteral(">\n"));
  }

  data.append(QStringLiteral("\n"));
  data.append(QStringLiteral("This is a multi-part message in MIME format.\r\n\n"));

  data.append(QStringLiteral("--") + boundary + QStringLiteral("\n"));

  addMimeBody(&data);

  if ( !m_attachments.isEmpty() ) {
    for (const auto &val : m_attachments) {
      data.append(QStringLiteral("--") + boundary + QStringLiteral("\n"));
      addMimeAttachment(&data, val);
    }
  }

  data.append(QStringLiteral("--") + boundary + QStringLiteral("--\r\n\n"));

  return data; 
  }


  QString MailSender::getMimeType(QString ext) const
  {
    //texte
    if (ext == QStringLiteral("txt"))			return QStringLiteral("text/plain");
    if (ext == QStringLiteral("htm") || ext == QStringLiteral("html"))	return QStringLiteral("text/html");
    if (ext == QStringLiteral("css"))			return QStringLiteral("text/css");
    //Images
    if (ext == QStringLiteral("png"))			return QStringLiteral("image/png");
    if (ext == QStringLiteral("gif"))			return QStringLiteral("image/gif");
    if (ext == QStringLiteral("jpg") || ext == QStringLiteral("jpeg"))	return QStringLiteral("image/jpeg");
    if (ext == QStringLiteral("bmp"))			return QStringLiteral("image/bmp");
    if (ext == QStringLiteral("tif"))			return QStringLiteral("image/tiff");
    //Archives
    if (ext == QStringLiteral("bz2"))			return QStringLiteral("application/x-bzip");
    if (ext == QStringLiteral("gz"))			return QStringLiteral("application/x-gzip");
    if (ext == QStringLiteral("tar") )			return QStringLiteral("application/x-tar");
    if (ext == QStringLiteral("zip") )			return QStringLiteral("application/zip");
    //Audio
    if ( ext == QStringLiteral("aif") || ext == QStringLiteral("aiff"))	return QStringLiteral("audio/aiff");
    if ( ext == QStringLiteral("mid") || ext == QStringLiteral("midi"))	return QStringLiteral("audio/mid");
    if ( ext == QStringLiteral("mp3"))			return QStringLiteral("audio/mpeg");
    if ( ext == QStringLiteral("ogg"))			return QStringLiteral("audio/ogg");
    if ( ext == QStringLiteral("wav"))			return QStringLiteral("audio/wav");
    if ( ext == QStringLiteral("wma"))			return QStringLiteral("audio/x-ms-wma");
    //Video
    if ( ext == QStringLiteral("asf") || ext == QStringLiteral("asx"))	return QStringLiteral("video/x-ms-asf");
    if ( ext == QStringLiteral("avi"))			return QStringLiteral("video/avi");
    if ( ext == QStringLiteral("mpg") || ext == QStringLiteral("mpeg"))	return QStringLiteral("video/mpeg");
    if ( ext == QStringLiteral("wmv"))			return QStringLiteral("video/x-ms-wmv");
    if ( ext == QStringLiteral("wmx"))			return QStringLiteral("video/x-ms-wmx");
    //XML
    if ( ext == QStringLiteral("xml"))			return QStringLiteral("text/xml");
    if ( ext == QStringLiteral("xsl"))			return QStringLiteral("text/xsl");
    //Microsoft
    if ( ext == QStringLiteral("doc") || ext == QStringLiteral("rtf"))	return QStringLiteral("application/msword");
    if ( ext == QStringLiteral("xls"))			return QStringLiteral("application/excel");
    if ( ext == QStringLiteral("ppt") || ext == QStringLiteral("pps"))	return QStringLiteral("application/vnd.ms-powerpoint");
    //Adobe
    if ( ext == QStringLiteral("pdf"))			return QStringLiteral("application/pdf");
    if ( ext == QStringLiteral("ai") || ext == QStringLiteral("eps"))	return QStringLiteral("application/postscript");
    if ( ext == QStringLiteral("psd"))			return QStringLiteral("image/psd");
    //Macromedia
    if ( ext == QStringLiteral("swf"))			return QStringLiteral("application/x-shockwave-flash");
    //Real
    if ( ext == QStringLiteral("ra"))			return QStringLiteral("audio/vnd.rn-realaudio");
    if ( ext == QStringLiteral("ram"))			return QStringLiteral("audio/x-pn-realaudio");
    if ( ext == QStringLiteral("rm"))			return QStringLiteral("application/vnd.rn-realmedia");
    if ( ext == QStringLiteral("rv"))			return QStringLiteral("video/vnd.rn-realvideo");
    //Other
    if ( ext == QStringLiteral("exe"))			return QStringLiteral("application/x-msdownload");
    if ( ext == QStringLiteral("pls"))			return QStringLiteral("audio/scpls");
    if ( ext == QStringLiteral("m3u"))			return QStringLiteral("audio/x-mpegurl");

    return QStringLiteral("text/plain"); // default
  }

  void MailSender::errorReceived(QAbstractSocket::SocketError socketError)
  {
    QString msg;

    switch(socketError) {
      case QAbstractSocket::ConnectionRefusedError: msg = QStringLiteral("ConnectionRefusedError"); break;
      case QAbstractSocket::RemoteHostClosedError: msg = QStringLiteral("RemoteHostClosedError"); break;
      case QAbstractSocket::HostNotFoundError: msg = QStringLiteral("HostNotFoundError"); break;
      case QAbstractSocket::SocketAccessError: msg = QStringLiteral("SocketAccessError"); break;
      case QAbstractSocket::SocketResourceError: msg = QStringLiteral("SocketResourceError"); break;
      case QAbstractSocket::SocketTimeoutError: msg = QStringLiteral("SocketTimeoutError"); break;
      case QAbstractSocket::DatagramTooLargeError: msg = QStringLiteral("DatagramTooLargeError"); break;
      case QAbstractSocket::NetworkError: msg = QStringLiteral("NetworkError"); break;
      case QAbstractSocket::AddressInUseError: msg = QStringLiteral("AddressInUseError"); break;
      case QAbstractSocket::SocketAddressNotAvailableError: msg = QStringLiteral("SocketAddressNotAvailableError"); break;
      case QAbstractSocket::UnsupportedSocketOperationError: msg = QStringLiteral("UnsupportedSocketOperationError"); break;
      case QAbstractSocket::ProxyAuthenticationRequiredError: msg = QStringLiteral("ProxyAuthenticationRequiredError"); break;
      default: msg = QStringLiteral("Unknown Error");
    }

    error(QStringLiteral("Socket error [") + msg + QStringLiteral("]"));
  }


  bool MailSender::send()
  {
    m_lastError.clear();

    if(m_socket) {
      delete m_socket;
    }

    m_socket = m_ssl ? new QSslSocket(this) : new QTcpSocket(this); 

    connect( m_socket, &QAbstractSocket::errorOccurred, this, &MailSender::errorReceived );
    connect( m_socket, &QAbstractSocket::proxyAuthenticationRequired, this, &MailSender::proxyAuthentication );

    bool auth = ! m_login.isEmpty();

    m_socket->connectToHost( m_smtpServer, m_port );

    if( !m_socket->waitForConnected( m_timeout ) ) {
      error(QStringLiteral("Time out connecting host"));
      return false;
    }

    if(!read(QStringLiteral("220"))) {
      return false;
    }

    if ( !sendCommand(QStringLiteral("EHLO there"), QStringLiteral("250")) ) {
      if ( !sendCommand(QStringLiteral("HELO there"), QStringLiteral("250")) ) {
        return false;
      }
    }

    if(m_ssl) {
      if ( !sendCommand(QStringLiteral("STARTTLS"), QStringLiteral("220")) ) {
        return false;
      }
      QSslSocket *pssl = qobject_cast<QSslSocket *>(m_socket);
      if(pssl == nullptr) {
        error(QStringLiteral("internal error casting to QSslSocket"));
        return false;
      }
      pssl->startClientEncryption ();
    }


    if ( auth ) {
      if( !sendCommand(QStringLiteral("AUTH LOGIN"), QStringLiteral("334")) ) {
        return false;
      }
      if( !sendCommand(encodeBase64(m_login), QStringLiteral("334")) ) {
        return false;
      }
      if( !sendCommand(encodeBase64(m_password), QStringLiteral("235")) ) {
        return false;
      }
    }

    if( !sendCommand(QStringLiteral("MAIL FROM:<") +m_from + QStringLiteral(">"), QStringLiteral("250")) ) {
      return false;
    }

    QStringList recipients = m_to + m_cc + m_bcc;
    for (const auto &recipient : recipients) {
      if( !sendCommand(QStringLiteral("RCPT TO:<") + recipient + QStringLiteral(">"), QStringLiteral("250")) ) {
        return false;
      }
    }

    if( !sendCommand(QStringLiteral("DATA"), QStringLiteral("354")) ) {
      return false;
    }
    if( !sendCommand(mailData() + QStringLiteral("\r\n."), QStringLiteral("250")) ) {
      return false;
    }
    if( !sendCommand(QStringLiteral("QUIT"), QStringLiteral("221")) ) {
      return false;
    }

    m_socket->disconnectFromHost();
    return true;
  }

  bool MailSender::read(const QString &waitfor)
  {
    if ( m_socket->state() != QAbstractSocket::ConnectedState ) return false;
    if( ! m_socket->waitForReadyRead( m_timeout ) ) {
      error(QStringLiteral("Read timeout"));
      return false;
    }

    if( !m_socket->canReadLine() ) {
      error(QStringLiteral("Can't read"));
      return false;
    }

    QString responseLine;

    do {
      responseLine = QString(m_socket->readLine());
    } while( m_socket->canReadLine() && responseLine[3] != QChar(' ') );

    m_lastResponse = responseLine;

    QString prefix = responseLine.left(3);
    bool isOk = (prefix == waitfor);
    if(!isOk) {
      error(QStringLiteral("waiting for ") + waitfor + QStringLiteral(", received ") + prefix);
    }

    return isOk;
  }


  bool MailSender::sendCommand(const QString &cmd, const QString &waitfor)
  {
    if ( m_socket->state() != QAbstractSocket::ConnectedState ) return false;
    QTextStream t(m_socket);
    t << cmd + QStringLiteral("\r\n");
    t.flush();

    m_lastCmd = cmd;

    return read(waitfor);
  }

  void MailSender::error(const QString &msg)
  {
    m_lastError = msg;
  }


  void MailSender::proxyAuthentication(const QNetworkProxy &, QAuthenticator * authenticator)
  {
    *authenticator = m_authenticator;
  }

  void MailSender::setProxyAuthenticator(const QAuthenticator &authenticator)
  {
    m_authenticator = authenticator;
  }
