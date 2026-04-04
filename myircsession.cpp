#include <unistd.h>
#include "myircsession.h"
#include <Irc>

static QString irc_color_strip_from_mirc( const QString &source ) {
  QString result;
  result.reserve( source.size() );
  for ( int i = 0; i < source.size(); ++i ) {
    QChar c = source.at( i );
    ushort u = c.unicode();
    if ( u == 0x02 || u == 0x1F || u == 0x16 || u == 0x0F ) continue;
    if ( u == 0x03 ) {
      if ( i + 1 < source.size() && source.at( i + 1 ).isDigit() ) {
        i++;
        if ( i + 1 < source.size() && source.at( i + 1 ).isDigit() ) i++;
        if ( i + 1 < source.size() && source.at( i + 1 ) == QLatin1Char(',') && i + 2 < source.size() && source.at( i + 2 ).isDigit() ) {
          i += 2;
          if ( i + 1 < source.size() && source.at( i + 1 ).isDigit() ) i++;
        }
      }
      continue;
    }
    result.append( c );
  }
  return result;
}

void MyIrcSession::connectSlotsByName(QObject *source) {
  connect(source, SIGNAL(receiverChanged(QString)), SLOT(on_receiverChanged(QString)));
  connect(source, SIGNAL(joined(QString)), SLOT(on_joined(QString)));
  connect(source, SIGNAL(parted(QString, QString)), SLOT(on_parted(QString, QString)));
  connect(source, SIGNAL(quit(QString, QString)), SLOT(on_quit(QString, QString)));
  connect(source, SIGNAL(nickChanged(QString, QString)), SLOT(on_nickChanged(QString, QString)));
  connect(source, SIGNAL(modeChanged(QString, QString, QString)), SLOT(on_modeChanged(QString, QString, QString)));
  connect(source, SIGNAL(topicChanged(QString, QString)), SLOT(on_topicChanged(QString, QString)));
  connect(source, SIGNAL(invited(QString, QString, QString)), SLOT(on_invited(QString, QString, QString)));
  connect(source, SIGNAL(kicked(QString, QString, QString)), SLOT(on_kicked(QString, QString, QString)));
  connect(source, SIGNAL(messageReceived(QString, QString)), SLOT(on_messageReceived(QString, QString)));
  connect(source, SIGNAL(noticeReceived(QString, QString)), SLOT(on_noticeReceived(QString, QString)));
  connect(source, SIGNAL(ctcpRequestReceived(QString, QString)), SLOT(on_ctcpRequestReceived(QString, QString)));
  connect(source, SIGNAL(ctcpReplyReceived(QString, QString)), SLOT(on_ctcpReplyReceived(QString, QString)));
  connect(source, SIGNAL(ctcpActionReceived(QString, QString)), SLOT(on_ctcpActionReceived(QString, QString)));
  connect(source, SIGNAL(numericMessageReceived(QString, uint, QStringList)), SLOT(on_numericMessageReceived(QString, uint, QStringList)));
  connect(source, SIGNAL(unknownMessageReceived(QString, QStringList)), SLOT(on_unknownMessageReceived(QString, QStringList)));
}

MyIrcSession::MyIrcSession( QObject* parent, datosIrc *datos, bool depurar ) : IrcConnection( parent ) {
  misdatos = datos;
  connectSlotsByName( this );
  debug = depurar;
  timer = new QTimer();
  connect( timer, &QTimer::timeout, this, &MyIrcSession::on_timeout );
  timer->start(120000);

  qDebug() << "Debug is : " << debug;

  setEncoding( "iso-8859-1" );
  setReconnectDelay(15);
  setNickName(misdatos->nick);
  setRealName( misdatos->name );
  setUserName( misdatos->user );
  setHost(misdatos->server);
  setPort(misdatos->port);
  open();
}

void MyIrcSession::on_timeout() {
  sendRaw(QLatin1String("PING ") + QString::number(QRandomGenerator::global()->bounded(1000000)));
}

void MyIrcSession::on_connected() {
  qDebug() << "Conected !!!";
  for ( int i = 0; i < misdatos->channels.size(); i++) {
    sendCommand(IrcCommand::createJoin(misdatos->channels.at(i)));
  }
}

void MyIrcSession::on_msgNickChanged( const QString& origin, const QString& nick ) {
  if ( debug ) qDebug() << "nick:" << origin << nick;
}

void MyIrcSession::on_msgQuit( const QString& origin, const QString& message ) {
  if ( debug ) qDebug() << "quit:" << origin << message;
}

void MyIrcSession::on_msgJoined( const QString& origin, const QString& channel ) {
  if ( debug ) qDebug() << "join:" << origin << channel;
}

void MyIrcSession::on_msgParted( const QString& origin, const QString& channel, const QString& message ) {
  if ( debug ) qDebug() << "part:" << origin << channel << message;
}

void MyIrcSession::on_msgModeChanged( const QString& origin, const QString& receiver, const QString& mode, const QString& args ) {
  // 	if ( !receiver.startsWith( '#' ) ) {
  if ( debug ) qDebug() << "channel_mode:" << origin << receiver << mode << args;
  // 	} else
  // 		on_userModeChanged(origin,mode);
}

void MyIrcSession::on_userModeChanged( const QString& origin, const QString& mode ) {
  if ( debug ) qDebug() << "user_mode:" << origin << mode;
}

void MyIrcSession::on_msgTopicChanged( const QString& origin, const QString& channel, const QString& topic ) {
  if ( debug ) qDebug() << "topic:" << origin << channel << topic;
}

void MyIrcSession::on_msgKicked( const QString& origin, const QString& channel, const QString& nick, const QString& message ) {
  Q_UNUSED( origin );
  Q_UNUSED( nick );
  Q_UNUSED( message );
  qDebug() << "Kicked from channel:" << channel;
  sleep( 5 );
  sendCommand(IrcCommand::createJoin(( channel )));
}

void MyIrcSession::on_channelMessageReceived( const QString& origin, const QString& channel, const QString& message ) {

  Q_UNUSED( channel );
  setEncoding( "iso-8859-1" );
  QString nick = origin.left( origin.indexOf( QLatin1Char( '!' ) ) );
  QString ssubida = QLatin1String("[Nueva Subida] ");
  QString mensaje = irc_color_strip_from_mirc( message );

  if ( debug ) qDebug() << "message:" << origin << channel << mensaje;

  if ( nick == misdatos->botNick ) {
    if ( mensaje.startsWith( ssubida ) ) {
      emit nuevaSubida( mensaje.mid( ssubida.length() ) );
    }
  }
}

void MyIrcSession::on_msgMessageReceived( const QString& origin, const QString& receiver, const QString& message ) {
  if ( receiver.startsWith( QLatin1Char('#') ) )
    on_channelMessageReceived( origin, receiver, message );
  else
    if ( debug ) qDebug() << "private:" << origin << receiver << message;
}

void MyIrcSession::on_msgNoticeReceived( const QString& origin, const QString& receiver, const QString& message ) {
  if ( debug ) qDebug() << "notice:" << origin << receiver << message;
}

void MyIrcSession::on_msgInvited( const QString& origin, const QString& nick, const QString& channel ) {
  if ( debug ) qDebug() << "invite:" << origin << nick << channel;
}

void MyIrcSession::on_msgCtcpRequestReceived( const QString& origin, const QString& message ) {
  if ( debug ) qDebug() << "ctcp_request:" << origin << message;
}

void MyIrcSession::on_msgCtcpReplyReceived( const QString& origin, const QString& message ) {
  if ( debug ) qDebug() << "ctcp_reply:" << origin << message;
}

void MyIrcSession::on_msgCtcpActionReceived( const QString& origin, const QString& message ) {
  if ( debug ) qDebug() << "ctcp_action:" << origin << message;
}

void MyIrcSession::on_msgUnknownMessageReceived( const QString& origin, const QStringList& params ) {
  if ( !debug ) return;

  QString stream;
  for (int i = 0; i < params.size(); ++i) {
    stream.append ( params.at(i) + QLatin1Char(' ') );
  }
  qDebug() << "unknown:" << origin << stream;
}

void MyIrcSession::on_msgNumericMessageReceived( const QString& origin, uint code, const QStringList& params ) {
  if ( code == 376 )
    for ( int i = 0; i < misdatos->channels.size(); i++ )
      sendCommand(IrcCommand::createJoin(misdatos->channels.at(i)));

  if ( !debug ) return;
  QString stream;
  for (int i = 0; i < params.size(); ++i) {
    stream.append ( params.at(i) + QLatin1Char(' ') );
  }
  qDebug() << "numeric:" << origin << code << stream;
}
