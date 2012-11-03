#include "mailsender.h"
#include <QString>
#include <QSslSocket>
#include <QTextStream>
#include <QByteArray>
#include <QDateTime>
#include <QTextCodec>
#include <QFile>
#include <QFileInfo>
#include <QHostInfo>
#include <time.h>



static QString encodeBase64( QString s )
{
  QByteArray text;
  text.append(s);
  return QString::fromAscii(text.toBase64());
}


static QString timeStamp()
{
  QTime now = QTime::currentTime();
  QDate today = QDate::currentDate();
  QStringList monthList = QStringList() << QString::fromAscii("Jan") << QString::fromAscii("Feb") << QString::fromAscii("Mar")<<QString::fromAscii("Apr")<< QString::fromAscii("May") <<QString::fromAscii("Jun")<<QString::fromAscii("Jul")<<QString::fromAscii("Aug")<<QString::fromAscii("Sep")<<QString::fromAscii("Oct")<<QString::fromAscii("Nov")<<QString::fromAscii("Dec");
  QString month = monthList.value(today.month()-1);
  QString day = QString::number(today.day());
  QString year = QString::number(today.year());
  QString result = QString( QString::fromAscii("Date: %1 %2 %3 %4") ).arg(day, month, year, now.toString(QString::fromAscii("hh:mm:ss"))); 
  return result;
}

static QString createBoundary()
{
  QByteArray hash = QCryptographicHash::hash(QString(QString::number(qrand())).toUtf8(),QCryptographicHash::Md5);
  QString boundary = QString::fromAscii(hash.toHex());
  boundary.truncate(26);
  boundary.prepend(QString::fromAscii("----=_NextPart_"));
  return boundary;
}


MailSender::MailSender(const QString &smtpServer, const QString &from, const QStringList &to)
{
  setSmtpServer(smtpServer);
  setPort(25);
  setTimeout(30000);
  setFrom(from);
  setTo(to);
  setSubject(QString::fromAscii("(no subject)"));
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
    case iso88591: m_charset = QString::fromAscii("iso-8859-1"); m_bodyCodec = QString::fromAscii("ISO 8859-1"); break;
    case utf8:     m_charset = QString::fromAscii("utf-8"); m_bodyCodec = QString::fromAscii("UTF-8"); break;
  }
}

void MailSender::setEncoding(Encoding encoding)
{
  switch(encoding) {
    case Encoding_7bit:     m_encoding = QString::fromAscii("7bit"); break;
    case Encoding_8bit:     m_encoding = QString::fromAscii("8bit"); break;
    case Encoding_base64:    m_encoding = QString::fromAscii("base64"); break;
  }
}


QString MailSender::contentType() const
{
  switch(m_contentType) {
    case HtmlContent:            return QString::fromAscii("text/html");
    case TextContent:
    default:              return QString::fromAscii("text/plain");
  }
}

QString MailSender::priorityString() const
{
  QString res;

  switch(m_priority) {
    case LowPriority:
      res.append(QString::fromAscii("X-Priority: 5\n"));
      res.append(QString::fromAscii("Priority: Non-Urgent\n"));
      res.append(QString::fromAscii("X-MSMail-Priority: Low\n"));
      res.append(QString::fromAscii("Importance: low\n"));
      break;
    case HighPriority:
      res.append(QString::fromAscii("X-Priority: 1\n"));
      res.append(QString::fromAscii("Priority: Urgent\n"));
      res.append(QString::fromAscii("X-MSMail-Priority: High\n"));
      res.append(QString::fromAscii("Importance: high\n"));
      break;
    default:
      res.append(QString::fromAscii("X-Priority: 3\n"));
      res.append(QString::fromAscii("    X-MSMail-Priority: Normal\n"));
  }

  return res;
}

void MailSender::addMimeAttachment(QString *pdata, const QString &filename) const
{
  QFile file(filename);
  bool ok = file.open(QIODevice::ReadOnly);
  if(!ok) {
    pdata->append(QString::fromAscii("Error attaching file: ") + filename + QString::fromAscii("\r\n"));
    return;
  }

  QFileInfo fileinfo(filename);
  QString type = getMimeType(fileinfo.suffix());
  pdata->append(QString::fromAscii("Content-Type: ") + type + QString::fromAscii(";\n"));
  pdata->append(QString::fromAscii("  name=") + fileinfo.fileName() + QString::fromAscii("\n"));

  QString tt = fileinfo.fileName();

  pdata->append(QString::fromAscii("Content-Transfer-Encoding: base64\n"));
  pdata->append(QString::fromAscii("Content-Disposition: attachment\n"));
  pdata->append(QString::fromAscii("  filename=") + fileinfo.fileName() + QString::fromAscii("\n\n"));
  QString encodedFile;
  QDataStream in(&file);    
  quint8 a;
  char c;
  QString b;
  while ( ! in.atEnd() ) {
    in >> a;
    c = a;    
    b.append(QChar::fromAscii(c));
  }
  encodedFile = encodeBase64(b);
  pdata->append(encodedFile);
  pdata->append(QString::fromAscii("\r\n\n"));
}

void MailSender::addMimeBody(QString *pdata) const
{
  pdata->append(QString::fromAscii("Content-Type: ") + contentType() + QString::fromAscii(";\n"));
  pdata->append(QString::fromAscii("  charset=") + m_charset + QString::fromAscii("\r\n"));
  pdata->append(QString::fromAscii("Content-Transfer-Encoding: ") + m_encoding + QString::fromAscii("\r\n"));
  pdata->append(QString::fromAscii("\r\n\n"));

  if ( m_contentType == HtmlContent ) {
    pdata->append(QString::fromAscii("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\r\n"));
    pdata->append(QString::fromAscii("<HTML><HEAD>\r\n"));
    pdata->append(QString::fromAscii("<META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; charset=") + m_charset + QString::fromAscii("\">\r\n"));
    pdata->append(QString::fromAscii("<META content=\"MSHTML 6.00.2900.2802\" name=GENERATOR>\r\n"));
    pdata->append(QString::fromAscii("<STYLE></STYLE>\r\n"));
    pdata->append(QString::fromAscii("</head>\r\n"));
    pdata->append(QString::fromAscii("<body bgColor=#ffffff>\r\n"));
  }

  QByteArray encodedBody(m_body.toLatin1()); // = array;
  QTextCodec *codec = QTextCodec::codecForName(m_bodyCodec.toLatin1()); 
  pdata->append(codec->toUnicode(encodedBody) + QString::fromAscii("\r\n"));

  if ( m_contentType == HtmlContent ) {
    pdata->append(QString::fromAscii("<DIV>&nbsp;</DIV></body></html>\r\n\n"));
  }
}

QString MailSender::mailData() const
{
  QString data;

  QByteArray hash = QCryptographicHash::hash(QString(QString::number(qrand())).toUtf8(),QCryptographicHash::Md5);
  QString id = QString::fromAscii(hash.toHex());
  data.append(QString::fromAscii("Message-ID: ") + id + QString::fromAscii("@") + QHostInfo::localHostName() + QString::fromAscii("\n"));
  data.append(QString::fromAscii("From: \"") + m_from + QString::fromAscii("\" <") + m_fromName + QString::fromAscii(">\n"));

  if ( m_to.count() > 0 ) {
    data.append(QString::fromAscii("To: "));
    bool first = true;
    //foreach (QString val, m_to) {
    for (int i = 0;i < m_to.size(); i++) {
      if(!first) {
        data.append(QString::fromAscii(","));
      }
      data.append(QString::fromAscii("<") + m_to.at(i) + QString::fromAscii(">"));
      first = false;
    }
    data.append(QString::fromAscii("\n")); 
  }

  if ( m_cc.count() > 0 ) {
    data.append(QString::fromAscii("Cc: ")); 
    bool first = true;
    //foreach (QString val, m_cc) {
    for (int i = 0;i < m_cc.size(); i++) {
      if(!first) {
        data.append(QString::fromAscii(","));
      }
      data.append(m_cc.at(i));
      first = false;
    }
    data.append(QString::fromAscii("\n"));
  }

  data.append(QString::fromAscii("Subject: ") + m_subject + QString::fromAscii("\n"));
  data.append(timeStamp() + QString::fromAscii("\n"));
  data.append(QString::fromAscii("MIME-Version: 1.0\n"));

  QString boundary = createBoundary();    
  data.append(QString::fromAscii("Content-Type: Multipart/Mixed; boundary=\"") + boundary + QString::fromAscii("\"\n"));
  data.append(priorityString());
  data.append(QString::fromAscii("X-Mailer: QT4\r\n"));  

  if ( ! m_confirmTo.isEmpty() ) {
    data.append(QString::fromAscii("Disposition-Notification-To: ") + m_confirmTo + QString::fromAscii("\n"));
  }

  if ( ! m_replyTo.isEmpty() && m_confirmTo != m_from ) {
    data.append(QString::fromAscii("Reply-to: ") + m_replyTo + QString::fromAscii("\n"));
    data.append(QString::fromAscii("Return-Path: <") + m_replyTo + QString::fromAscii(">\n"));
  }

  data.append(QString::fromAscii("\n"));
  data.append(QString::fromAscii("This is a multi-part message in MIME format.\r\n\n"));

  data.append(QString::fromAscii("--") + boundary + QString::fromAscii("\n"));

  addMimeBody(&data);

  if ( m_attachments.count() > 0 ) {
    //foreach (QString val, m_attachments) {
    for (int i = 0;i < m_attachments.size(); i++) {
      data.append(QString::fromAscii("--") + boundary + QString::fromAscii("\n"));
      addMimeAttachment(&data, m_attachments.at(i));
    }
  }

  data.append(QString::fromAscii("--") + boundary + QString::fromAscii("--\r\n\n"));

  return data; 
  }


  QString MailSender::getMimeType(QString ext) const
  {
    //texte
    if (ext == QString::fromAscii("txt"))			return QString::fromAscii("text/plain");
    if (ext == QString::fromAscii("htm") || ext == QString::fromAscii("html"))	return QString::fromAscii("text/html");
    if (ext == QString::fromAscii("css"))			return QString::fromAscii("text/css");
    //Images
    if (ext == QString::fromAscii("png"))			return QString::fromAscii("image/png");
    if (ext == QString::fromAscii("gif"))			return QString::fromAscii("image/gif");
    if (ext == QString::fromAscii("jpg") || ext == QString::fromAscii("jpeg"))	return QString::fromAscii("image/jpeg");
    if (ext == QString::fromAscii("bmp"))			return QString::fromAscii("image/bmp");
    if (ext == QString::fromAscii("tif"))			return QString::fromAscii("image/tiff");
    //Archives
    if (ext == QString::fromAscii("bz2"))			return QString::fromAscii("application/x-bzip");
    if (ext == QString::fromAscii("gz"))			return QString::fromAscii("application/x-gzip");
    if (ext == QString::fromAscii("tar") )			return QString::fromAscii("application/x-tar");
    if (ext == QString::fromAscii("zip") )			return QString::fromAscii("application/zip");
    //Audio
    if ( ext == QString::fromAscii("aif") || ext == QString::fromAscii("aiff"))	return QString::fromAscii("audio/aiff");
    if ( ext == QString::fromAscii("mid") || ext == QString::fromAscii("midi"))	return QString::fromAscii("audio/mid");
    if ( ext == QString::fromAscii("mp3"))			return QString::fromAscii("audio/mpeg");
    if ( ext == QString::fromAscii("ogg"))			return QString::fromAscii("audio/ogg");
    if ( ext == QString::fromAscii("wav"))			return QString::fromAscii("audio/wav");
    if ( ext == QString::fromAscii("wma"))			return QString::fromAscii("audio/x-ms-wma");
    //Video
    if ( ext == QString::fromAscii("asf") || ext == QString::fromAscii("asx"))	return QString::fromAscii("video/x-ms-asf");
    if ( ext == QString::fromAscii("avi"))			return QString::fromAscii("video/avi");
    if ( ext == QString::fromAscii("mpg") || ext == QString::fromAscii("mpeg"))	return QString::fromAscii("video/mpeg");
    if ( ext == QString::fromAscii("wmv"))			return QString::fromAscii("video/x-ms-wmv");
    if ( ext == QString::fromAscii("wmx"))			return QString::fromAscii("video/x-ms-wmx");
    //XML
    if ( ext == QString::fromAscii("xml"))			return QString::fromAscii("text/xml");
    if ( ext == QString::fromAscii("xsl"))			return QString::fromAscii("text/xsl");
    //Microsoft
    if ( ext == QString::fromAscii("doc") || ext == QString::fromAscii("rtf"))	return QString::fromAscii("application/msword");
    if ( ext == QString::fromAscii("xls"))			return QString::fromAscii("application/excel");
    if ( ext == QString::fromAscii("ppt") || ext == QString::fromAscii("pps"))	return QString::fromAscii("application/vnd.ms-powerpoint");
    //Adobe
    if ( ext == QString::fromAscii("pdf"))			return QString::fromAscii("application/pdf");
    if ( ext == QString::fromAscii("ai") || ext == QString::fromAscii("eps"))	return QString::fromAscii("application/postscript");
    if ( ext == QString::fromAscii("psd"))			return QString::fromAscii("image/psd");
    //Macromedia
    if ( ext == QString::fromAscii("swf"))			return QString::fromAscii("application/x-shockwave-flash");
    //Real
    if ( ext == QString::fromAscii("ra"))			return QString::fromAscii("audio/vnd.rn-realaudio");
    if ( ext == QString::fromAscii("ram"))			return QString::fromAscii("audio/x-pn-realaudio");
    if ( ext == QString::fromAscii("rm"))			return QString::fromAscii("application/vnd.rn-realmedia");
    if ( ext == QString::fromAscii("rv"))			return QString::fromAscii("video/vnd.rn-realvideo");
    //Other
    if ( ext == QString::fromAscii("exe"))			return QString::fromAscii("application/x-msdownload");
    if ( ext == QString::fromAscii("pls"))			return QString::fromAscii("audio/scpls");
    if ( ext == QString::fromAscii("m3u"))			return QString::fromAscii("audio/x-mpegurl");

    return QString::fromAscii("text/plain"); // default
  }

  void MailSender::errorReceived(QAbstractSocket::SocketError socketError)
  {
    QString msg;

    switch(socketError) {
      case QAbstractSocket::ConnectionRefusedError: msg = QString::fromAscii("ConnectionRefusedError"); break;
      case QAbstractSocket::RemoteHostClosedError: msg = QString::fromAscii("RemoteHostClosedError"); break;
      case QAbstractSocket::HostNotFoundError: msg = QString::fromAscii("HostNotFoundError"); break;
      case QAbstractSocket::SocketAccessError: msg = QString::fromAscii("SocketAccessError"); break;
      case QAbstractSocket::SocketResourceError: msg = QString::fromAscii("SocketResourceError"); break;
      case QAbstractSocket::SocketTimeoutError: msg = QString::fromAscii("SocketTimeoutError"); break;
      case QAbstractSocket::DatagramTooLargeError: msg = QString::fromAscii("DatagramTooLargeError"); break;
      case QAbstractSocket::NetworkError: msg = QString::fromAscii("NetworkError"); break;
      case QAbstractSocket::AddressInUseError: msg = QString::fromAscii("AddressInUseError"); break;
      case QAbstractSocket::SocketAddressNotAvailableError: msg = QString::fromAscii("SocketAddressNotAvailableError"); break;
      case QAbstractSocket::UnsupportedSocketOperationError: msg = QString::fromAscii("UnsupportedSocketOperationError"); break;
      case QAbstractSocket::ProxyAuthenticationRequiredError: msg = QString::fromAscii("ProxyAuthenticationRequiredError"); break;
      default: msg = QString::fromAscii("Unknown Error");
    }

    error(QString::fromAscii("Socket error [") + msg + QString::fromAscii("]"));
  }


  bool MailSender::send()
  {
    m_lastError = QString::fromAscii("");

    if(m_socket) {
      delete m_socket;
    }

    m_socket = m_ssl ? new QSslSocket(this) : new QTcpSocket(this); 

    connect( m_socket, SIGNAL( error( QAbstractSocket::SocketError) ), this, SLOT( errorReceived( QAbstractSocket::SocketError ) ) );
    connect( m_socket, SIGNAL( proxyAuthenticationRequired(const QNetworkProxy & , QAuthenticator *) ), this, SLOT(proxyAuthentication(const QNetworkProxy &, QAuthenticator * ) ) );

    bool auth = ! m_login.isEmpty();

    m_socket->connectToHost( m_smtpServer, m_port );

    if( !m_socket->waitForConnected( m_timeout ) ) {
      error(QString::fromAscii("Time out connecting host"));
      return false;
    }

    if(!read(QString::fromAscii("220"))) {
      return false;
    }

    if ( !sendCommand(QString::fromAscii("EHLO there"), QString::fromAscii("250")) ) {
      if ( !sendCommand(QString::fromAscii("HELO there"), QString::fromAscii("250")) ) {
        return false;
      }
    }

    if(m_ssl) {
      if ( !sendCommand(QString::fromAscii("STARTTLS"), QString::fromAscii("220")) ) {
        return false;
      }
      QSslSocket *pssl = qobject_cast<QSslSocket *>(m_socket);
      if(pssl == 0) {
        error(QString::fromAscii("internal error casting to QSslSocket"));
        return false;
      }
      pssl->startClientEncryption ();
    }


    if ( auth ) {
      if( !sendCommand(QString::fromAscii("AUTH LOGIN"), QString::fromAscii("334")) ) {
        return false;
      }
      if( !sendCommand(encodeBase64(m_login), QString::fromAscii("334")) ) {
        return false;
      }
      if( !sendCommand(encodeBase64(m_password), QString::fromAscii("235")) ) {
        return false;
      }
    }

    if( !sendCommand(QString::fromLatin1("MAIL FROM:<") +m_from + QString::fromLatin1(">"), QString::fromAscii("250")) ) {
      return false;
    }

    QStringList recipients = m_to + m_cc + m_bcc;
    for (int i=0; i< recipients.count(); i++) {
      if( !sendCommand(QString::fromLatin1("RCPT TO:<") + recipients.at(i) + QString::fromLatin1(">"), QString::fromAscii("250")) ) {
        return false;
      }
    }

    if( !sendCommand(QString::fromLatin1("DATA"), QString::fromAscii("354")) ) {
      return false;
    }
    if( !sendCommand(mailData() + QString::fromLatin1("\r\n."), QString::fromAscii("250")) ) {
      return false;
    }
    if( !sendCommand(QString::fromLatin1("QUIT"), QString::fromAscii("221")) ) {
      return false;
    }

    m_socket->disconnectFromHost();
    return true;
  }

  bool MailSender::read(const QString &waitfor)
  {
    if ( m_socket->state() != QAbstractSocket::ConnectedState ) return false;
    if( ! m_socket->waitForReadyRead( m_timeout ) ) {
      error(QString::fromAscii("Read timeout"));
      return false;
    }

    if( !m_socket->canReadLine() ) {
      error(QString::fromAscii("Can't read"));
      return false;
    }

    QString responseLine;

    do {
      responseLine = QString::fromAscii(m_socket->readLine());
    } while( m_socket->canReadLine() && responseLine[3] != QChar::fromAscii(' ') );

    m_lastResponse = responseLine;

    QString prefix = responseLine.left(3);
    bool isOk = (prefix == waitfor);
    if(!isOk) {
      error(QString::fromAscii("waiting for ") + waitfor + QString::fromAscii(", received ") + prefix);
    }

    return isOk;
  }


  bool MailSender::sendCommand(const QString &cmd, const QString &waitfor)
  {
    if ( m_socket->state() != QAbstractSocket::ConnectedState ) return false;
    QTextStream t(m_socket);
    t << cmd + QString::fromAscii("\r\n");
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
