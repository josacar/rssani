#include "mailsender.h"
#include <QString>
#include <QSslSocket>
#include <QDataStream>
#include <QTextStream>
#include <QByteArray>
#include <QDateTime>
#include <QTextCodec>
#include <QFile>
#include <QFileInfo>
#include <QHostInfo>
#include <time.h>

enum QDataStreamError {
    BAD_FILE_FORMAT,
    BAD_FILE_TOO_OLD,
    BAD_FILE_TOO_NEW
};

static QString encodeBase64( QString s )
{
  QByteArray text;
  text.append(s);
  return QString::fromUtf8(text.toBase64());
}


static QString timeStamp()
{
  QTime now = QTime::currentTime();
  QDate today = QDate::currentDate();
  QStringList monthList = QStringList() << QString::fromUtf8("Jan") << QString::fromUtf8("Feb") << QString::fromUtf8("Mar")<<QString::fromUtf8("Apr")<< QString::fromUtf8("May") <<QString::fromUtf8("Jun")<<QString::fromUtf8("Jul")<<QString::fromUtf8("Aug")<<QString::fromUtf8("Sep")<<QString::fromUtf8("Oct")<<QString::fromUtf8("Nov")<<QString::fromUtf8("Dec");
  QString month = monthList.value(today.month()-1);
  QString day = QString::number(today.day());
  QString year = QString::number(today.year());
  QString result = QString( QString::fromUtf8("Date: %1 %2 %3 %4") ).arg(day, month, year, now.toString(QString::fromUtf8("hh:mm:ss")));
  return result;
}

static QString createBoundary()
{
  QByteArray hash = QCryptographicHash::hash(QString(QString::number(qrand())).toUtf8(),QCryptographicHash::Md5);
  QString boundary = QString::fromUtf8(hash.toHex());
  boundary.truncate(26);
  boundary.prepend(QString::fromUtf8("----=_NextPart_"));
  return boundary;
}


MailSender::MailSender(const QString &smtpServer, const QString &from, const QStringList &to)
{
  setSmtpServer(smtpServer);
  setPort(25);
  setTimeout(30000);
  setFrom(from);
  setTo(to);
  setSubject(QString::fromUtf8("(no subject)"));
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
    case iso88591: m_charset = QString::fromUtf8("iso-8859-1"); m_bodyCodec = QString::fromUtf8("ISO 8859-1"); break;
    case utf8:     m_charset = QString::fromUtf8("utf-8"); m_bodyCodec = QString::fromUtf8("UTF-8"); break;
  }
}

void MailSender::setEncoding(Encoding encoding)
{
  switch(encoding) {
    case Encoding_7bit:     m_encoding = QString::fromUtf8("7bit"); break;
    case Encoding_8bit:     m_encoding = QString::fromUtf8("8bit"); break;
    case Encoding_base64:    m_encoding = QString::fromUtf8("base64"); break;
  }
}


QString MailSender::contentType() const
{
  switch(m_contentType) {
    case HtmlContent:            return QString::fromUtf8("text/html");
    case TextContent:
    default:              return QString::fromUtf8("text/plain");
  }
}

QString MailSender::priorityString() const
{
  QString res;

  switch(m_priority) {
    case LowPriority:
      res.append(QString::fromUtf8("X-Priority: 5\n"));
      res.append(QString::fromUtf8("Priority: Non-Urgent\n"));
      res.append(QString::fromUtf8("X-MSMail-Priority: Low\n"));
      res.append(QString::fromUtf8("Importance: low\n"));
      break;
    case HighPriority:
      res.append(QString::fromUtf8("X-Priority: 1\n"));
      res.append(QString::fromUtf8("Priority: Urgent\n"));
      res.append(QString::fromUtf8("X-MSMail-Priority: High\n"));
      res.append(QString::fromUtf8("Importance: high\n"));
      break;
    default:
      res.append(QString::fromUtf8("X-Priority: 3\n"));
      res.append(QString::fromUtf8("    X-MSMail-Priority: Normal\n"));
  }

  return res;
}

int MailSender::addMimeAttachment(QString *pdata, const QString &filename) const
{
  QFile file(filename);
  bool ok = file.open(QIODevice::ReadOnly);
  if(!ok) {
    pdata->append(QString::fromUtf8("Error attaching file: ") + filename + QString::fromUtf8("\r\n"));
    return -1;
  }

  QFileInfo fileinfo(filename);
  QString type = getMimeType(fileinfo.suffix());
  pdata->append(QString::fromUtf8("Content-Type: ") + type + QString::fromUtf8(";\n"));
  pdata->append(QString::fromUtf8("  name=") + fileinfo.fileName() + QString::fromUtf8("\n"));

  QString tt = fileinfo.fileName();

  pdata->append(QString::fromUtf8("Content-Transfer-Encoding: base64\n"));
  pdata->append(QString::fromUtf8("Content-Disposition: attachment\n"));
  pdata->append(QString::fromUtf8("  filename=") + fileinfo.fileName() + QString::fromUtf8("\n\n"));
  QString encodedFile;
  QDataStream in(&file);

  // Read and check the header
  quint32 magic;
  in >> magic;
  if (magic != 0xA0B0C0D0)
      return QDataStreamError::BAD_FILE_FORMAT;

  // Read the version
  qint32 version;
  in >> version;
  if (version < 100)
      return QDataStreamError::BAD_FILE_TOO_OLD;
  if (version > 123)
      return QDataStreamError::BAD_FILE_TOO_NEW;

  in.setVersion(QDataStream::Qt_4_0);

  quint8 a;
  char c;
  QString b;
  while ( ! in.atEnd() ) {
    in >> a;
    c = a;    
    b.append(QChar(c));
  }
  encodedFile = encodeBase64(b);
  pdata->append(encodedFile);
  pdata->append(QString("\r\n\n"));

  return 0;
}

void MailSender::addMimeBody(QString *pdata) const
{
  pdata->append(QString::fromUtf8("Content-Type: ") + contentType() + QString::fromUtf8(";\n"));
  pdata->append(QString::fromUtf8("  charset=") + m_charset + QString::fromUtf8("\r\n"));
  pdata->append(QString::fromUtf8("Content-Transfer-Encoding: ") + m_encoding + QString::fromUtf8("\r\n"));
  pdata->append(QString::fromUtf8("\r\n\n"));

  if ( m_contentType == HtmlContent ) {
    pdata->append(QString::fromUtf8("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\r\n"));
    pdata->append(QString::fromUtf8("<HTML><HEAD>\r\n"));
    pdata->append(QString::fromUtf8("<META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; charset=") + m_charset + QString::fromUtf8("\">\r\n"));
    pdata->append(QString::fromUtf8("<META content=\"MSHTML 6.00.2900.2802\" name=GENERATOR>\r\n"));
    pdata->append(QString::fromUtf8("<STYLE></STYLE>\r\n"));
    pdata->append(QString::fromUtf8("</head>\r\n"));
    pdata->append(QString::fromUtf8("<body bgColor=#ffffff>\r\n"));
  }

  QByteArray encodedBody(m_body.toLatin1()); // = array;
  QTextCodec *codec = QTextCodec::codecForName(m_bodyCodec.toLatin1()); 
  pdata->append(codec->toUnicode(encodedBody) + QString::fromUtf8("\r\n"));

  if ( m_contentType == HtmlContent ) {
    pdata->append(QString::fromUtf8("<DIV>&nbsp;</DIV></body></html>\r\n\n"));
  }
}

QString MailSender::mailData() const
{
  QString data;

  QByteArray hash = QCryptographicHash::hash(QString(QString::number(qrand())).toUtf8(),QCryptographicHash::Md5);
  QString id = QString::fromUtf8(hash.toHex());
  data.append(QString::fromUtf8("Message-ID: ") + id + QString::fromUtf8("@") + QHostInfo::localHostName() + QString::fromUtf8("\n"));
  data.append(QString::fromUtf8("From: \"") + m_from + QString::fromUtf8("\" <") + m_fromName + QString::fromUtf8(">\n"));

  if ( m_to.count() > 0 ) {
    data.append(QString::fromUtf8("To: "));
    bool first = true;
    //foreach (QString val, m_to) {
    for (int i = 0;i < m_to.size(); i++) {
      if(!first) {
        data.append(QString::fromUtf8(","));
      }
      data.append(QString::fromUtf8("<") + m_to.at(i) + QString::fromUtf8(">"));
      first = false;
    }
    data.append(QString::fromUtf8("\n"));
  }

  if ( m_cc.count() > 0 ) {
    data.append(QString::fromUtf8("Cc: "));
    bool first = true;
    //foreach (QString val, m_cc) {
    for (int i = 0;i < m_cc.size(); i++) {
      if(!first) {
        data.append(QString::fromUtf8(","));
      }
      data.append(m_cc.at(i));
      first = false;
    }
    data.append(QString::fromUtf8("\n"));
  }

  data.append(QString::fromUtf8("Subject: ") + m_subject + QString::fromUtf8("\n"));
  data.append(timeStamp() + QString::fromUtf8("\n"));
  data.append(QString::fromUtf8("MIME-Version: 1.0\n"));

  QString boundary = createBoundary();    
  data.append(QString::fromUtf8("Content-Type: Multipart/Mixed; boundary=\"") + boundary + QString::fromUtf8("\"\n"));
  data.append(priorityString());
  data.append(QString::fromUtf8("X-Mailer: QT4\r\n"));

  if ( ! m_confirmTo.isEmpty() ) {
    data.append(QString::fromUtf8("Disposition-Notification-To: ") + m_confirmTo + QString::fromUtf8("\n"));
  }

  if ( ! m_replyTo.isEmpty() && m_confirmTo != m_from ) {
    data.append(QString::fromUtf8("Reply-to: ") + m_replyTo + QString::fromUtf8("\n"));
    data.append(QString::fromUtf8("Return-Path: <") + m_replyTo + QString::fromUtf8(">\n"));
  }

  data.append(QString::fromUtf8("\n"));
  data.append(QString::fromUtf8("This is a multi-part message in MIME format.\r\n\n"));

  data.append(QString::fromUtf8("--") + boundary + QString::fromUtf8("\n"));

  addMimeBody(&data);

  if ( m_attachments.count() > 0 ) {
    //foreach (QString val, m_attachments) {
    for (int i = 0;i < m_attachments.size(); i++) {
      data.append(QString::fromUtf8("--") + boundary + QString::fromUtf8("\n"));
      addMimeAttachment(&data, m_attachments.at(i));
    }
  }

  data.append(QString::fromUtf8("--") + boundary + QString::fromUtf8("--\r\n\n"));

  return data; 
  }


  QString MailSender::getMimeType(QString ext) const
  {
    //texte
    if (ext == QString::fromUtf8("txt"))			return QString::fromUtf8("text/plain");
    if (ext == QString::fromUtf8("htm") || ext == QString::fromUtf8("html"))	return QString::fromUtf8("text/html");
    if (ext == QString::fromUtf8("css"))			return QString::fromUtf8("text/css");
    //Images
    if (ext == QString::fromUtf8("png"))			return QString::fromUtf8("image/png");
    if (ext == QString::fromUtf8("gif"))			return QString::fromUtf8("image/gif");
    if (ext == QString::fromUtf8("jpg") || ext == QString::fromUtf8("jpeg"))	return QString::fromUtf8("image/jpeg");
    if (ext == QString::fromUtf8("bmp"))			return QString::fromUtf8("image/bmp");
    if (ext == QString::fromUtf8("tif"))			return QString::fromUtf8("image/tiff");
    //Archives
    if (ext == QString::fromUtf8("bz2"))			return QString::fromUtf8("application/x-bzip");
    if (ext == QString::fromUtf8("gz"))			return QString::fromUtf8("application/x-gzip");
    if (ext == QString::fromUtf8("tar") )			return QString::fromUtf8("application/x-tar");
    if (ext == QString::fromUtf8("zip") )			return QString::fromUtf8("application/zip");
    //Audio
    if ( ext == QString::fromUtf8("aif") || ext == QString::fromUtf8("aiff"))	return QString::fromUtf8("audio/aiff");
    if ( ext == QString::fromUtf8("mid") || ext == QString::fromUtf8("midi"))	return QString::fromUtf8("audio/mid");
    if ( ext == QString::fromUtf8("mp3"))			return QString::fromUtf8("audio/mpeg");
    if ( ext == QString::fromUtf8("ogg"))			return QString::fromUtf8("audio/ogg");
    if ( ext == QString::fromUtf8("wav"))			return QString::fromUtf8("audio/wav");
    if ( ext == QString::fromUtf8("wma"))			return QString::fromUtf8("audio/x-ms-wma");
    //Video
    if ( ext == QString::fromUtf8("asf") || ext == QString::fromUtf8("asx"))	return QString::fromUtf8("video/x-ms-asf");
    if ( ext == QString::fromUtf8("avi"))			return QString::fromUtf8("video/avi");
    if ( ext == QString::fromUtf8("mpg") || ext == QString::fromUtf8("mpeg"))	return QString::fromUtf8("video/mpeg");
    if ( ext == QString::fromUtf8("wmv"))			return QString::fromUtf8("video/x-ms-wmv");
    if ( ext == QString::fromUtf8("wmx"))			return QString::fromUtf8("video/x-ms-wmx");
    //XML
    if ( ext == QString::fromUtf8("xml"))			return QString::fromUtf8("text/xml");
    if ( ext == QString::fromUtf8("xsl"))			return QString::fromUtf8("text/xsl");
    //Microsoft
    if ( ext == QString::fromUtf8("doc") || ext == QString::fromUtf8("rtf"))	return QString::fromUtf8("application/msword");
    if ( ext == QString::fromUtf8("xls"))			return QString::fromUtf8("application/excel");
    if ( ext == QString::fromUtf8("ppt") || ext == QString::fromUtf8("pps"))	return QString::fromUtf8("application/vnd.ms-powerpoint");
    //Adobe
    if ( ext == QString::fromUtf8("pdf"))			return QString::fromUtf8("application/pdf");
    if ( ext == QString::fromUtf8("ai") || ext == QString::fromUtf8("eps"))	return QString::fromUtf8("application/postscript");
    if ( ext == QString::fromUtf8("psd"))			return QString::fromUtf8("image/psd");
    //Macromedia
    if ( ext == QString::fromUtf8("swf"))			return QString::fromUtf8("application/x-shockwave-flash");
    //Real
    if ( ext == QString::fromUtf8("ra"))			return QString::fromUtf8("audio/vnd.rn-realaudio");
    if ( ext == QString::fromUtf8("ram"))			return QString::fromUtf8("audio/x-pn-realaudio");
    if ( ext == QString::fromUtf8("rm"))			return QString::fromUtf8("application/vnd.rn-realmedia");
    if ( ext == QString::fromUtf8("rv"))			return QString::fromUtf8("video/vnd.rn-realvideo");
    //Other
    if ( ext == QString::fromUtf8("exe"))			return QString::fromUtf8("application/x-msdownload");
    if ( ext == QString::fromUtf8("pls"))			return QString::fromUtf8("audio/scpls");
    if ( ext == QString::fromUtf8("m3u"))			return QString::fromUtf8("audio/x-mpegurl");

    return QString::fromUtf8("text/plain"); // default
  }

  void MailSender::errorReceived(QAbstractSocket::SocketError socketError)
  {
    QString msg;

    switch(socketError) {
      case QAbstractSocket::ConnectionRefusedError: msg = QString::fromUtf8("ConnectionRefusedError"); break;
      case QAbstractSocket::RemoteHostClosedError: msg = QString::fromUtf8("RemoteHostClosedError"); break;
      case QAbstractSocket::HostNotFoundError: msg = QString::fromUtf8("HostNotFoundError"); break;
      case QAbstractSocket::SocketAccessError: msg = QString::fromUtf8("SocketAccessError"); break;
      case QAbstractSocket::SocketResourceError: msg = QString::fromUtf8("SocketResourceError"); break;
      case QAbstractSocket::SocketTimeoutError: msg = QString::fromUtf8("SocketTimeoutError"); break;
      case QAbstractSocket::DatagramTooLargeError: msg = QString::fromUtf8("DatagramTooLargeError"); break;
      case QAbstractSocket::NetworkError: msg = QString::fromUtf8("NetworkError"); break;
      case QAbstractSocket::AddressInUseError: msg = QString::fromUtf8("AddressInUseError"); break;
      case QAbstractSocket::SocketAddressNotAvailableError: msg = QString::fromUtf8("SocketAddressNotAvailableError"); break;
      case QAbstractSocket::UnsupportedSocketOperationError: msg = QString::fromUtf8("UnsupportedSocketOperationError"); break;
      case QAbstractSocket::ProxyAuthenticationRequiredError: msg = QString::fromUtf8("ProxyAuthenticationRequiredError"); break;
      default: msg = QString::fromUtf8("Unknown Error");
    }

    error(QString::fromUtf8("Socket error [") + msg + QString::fromUtf8("]"));
  }


  bool MailSender::send()
  {
    m_lastError = QString::fromUtf8("");

    if(m_socket) {
      delete m_socket;
    }

    m_socket = m_ssl ? new QSslSocket(this) : new QTcpSocket(this); 

    connect( m_socket, SIGNAL( error( QAbstractSocket::SocketError) ), this, SLOT( errorReceived( QAbstractSocket::SocketError ) ) );
    connect( m_socket, SIGNAL( proxyAuthenticationRequired(const QNetworkProxy & , QAuthenticator *) ), this, SLOT(proxyAuthentication(const QNetworkProxy &, QAuthenticator * ) ) );

    bool auth = ! m_login.isEmpty();

    m_socket->connectToHost( m_smtpServer, m_port );

    if( !m_socket->waitForConnected( m_timeout ) ) {
      error(QString::fromUtf8("Time out connecting host"));
      return false;
    }

    if(!read(QString::fromUtf8("220"))) {
      return false;
    }

    if ( !sendCommand(QString::fromUtf8("EHLO there"), QString::fromUtf8("250")) ) {
      if ( !sendCommand(QString::fromUtf8("HELO there"), QString::fromUtf8("250")) ) {
        return false;
      }
    }

    if(m_ssl) {
      if ( !sendCommand(QString::fromUtf8("STARTTLS"), QString::fromUtf8("220")) ) {
        return false;
      }
      QSslSocket *pssl = qobject_cast<QSslSocket *>(m_socket);
      if(pssl == 0) {
        error(QString::fromUtf8("internal error casting to QSslSocket"));
        return false;
      }
      pssl->startClientEncryption ();
    }


    if ( auth ) {
      if( !sendCommand(QString::fromUtf8("AUTH LOGIN"), QString::fromUtf8("334")) ) {
        return false;
      }
      if( !sendCommand(encodeBase64(m_login), QString::fromUtf8("334")) ) {
        return false;
      }
      if( !sendCommand(encodeBase64(m_password), QString::fromUtf8("235")) ) {
        return false;
      }
    }

    if( !sendCommand(QString::fromLatin1("MAIL FROM:<") +m_from + QString::fromLatin1(">"), QString::fromUtf8("250")) ) {
      return false;
    }

    QStringList recipients = m_to + m_cc + m_bcc;
    for (int i=0; i< recipients.count(); i++) {
      if( !sendCommand(QString::fromLatin1("RCPT TO:<") + recipients.at(i) + QString::fromLatin1(">"), QString::fromUtf8("250")) ) {
        return false;
      }
    }

    if( !sendCommand(QString::fromLatin1("DATA"), QString::fromUtf8("354")) ) {
      return false;
    }
    if( !sendCommand(mailData() + QString::fromLatin1("\r\n."), QString::fromUtf8("250")) ) {
      return false;
    }
    if( !sendCommand(QString::fromLatin1("QUIT"), QString::fromUtf8("221")) ) {
      return false;
    }

    m_socket->disconnectFromHost();
    return true;
  }

  bool MailSender::read(const QString &waitfor)
  {
    if ( m_socket->state() != QAbstractSocket::ConnectedState ) return false;
    if( ! m_socket->waitForReadyRead( m_timeout ) ) {
      error(QString::fromUtf8("Read timeout"));
      return false;
    }

    if( !m_socket->canReadLine() ) {
      error(QString::fromUtf8("Can't read"));
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
      error(QString::fromUtf8("waiting for ") + waitfor + QString::fromUtf8(", received ") + prefix);
    }

    return isOk;
  }


  bool MailSender::sendCommand(const QString &cmd, const QString &waitfor)
  {
    if ( m_socket->state() != QAbstractSocket::ConnectedState ) return false;
    QTextStream t(m_socket);
    t << cmd + QString::fromUtf8("\r\n");
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
