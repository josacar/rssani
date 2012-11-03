#include <unistd.h>
#include "myircsession.h"
#include <Irc>

static char * irc_color_strip_from_mirc( const char * source ) {
  unsigned int destlen = 0;
  char * destline = 0, *d = 0;
  const char *p;

  while ( destline == 0 ) {
    if ( destlen > 0 ) {
      if (( destline = ( char * )malloc( destlen ) ) == 0 ) return 0;

      d = destline;
    }

    for ( p = source; *p; p++ ) {
      switch ( *p ) {
        case 0x02:      // bold
        case 0x1F:      // underline
        case 0x16:      // reverse
        case 0x0F:      // reset colors
          continue;
        case 0x03:      // set color
          if ( isdigit( p[1] ) ) {
            p++;
            if ( isdigit( p[1] ) ) p++;
            if ( p[1] == ',' && isdigit( p[2] ) ) {
              p += 2;
              if ( isdigit( p[1] ) ) p++;
            }
          }
          continue;
        default:
          if ( destline )
            *d++ = *p;
          else
            destlen++;
          break;
      }
    }

    destlen++; // for 0-terminator
  }
  *d = '\0';
  return destline;
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

MyIrcSession::MyIrcSession( QObject* parent, datosIrc *datos, bool depurar ) : Session( parent ) {
  misdatos = datos;
  connectSlotsByName( this );
  debug = depurar;
  timer = new QTimer();
  connect( timer, SIGNAL( timeout() ), this, SLOT( on_timeout() ) );
  timer->start(120000);

  qDebug() << "Debug is : " << debug;

  setEncoding( "iso-8859-1" );
  setAutoReconnectDelay(15);
  setNick(misdatos->nick);
  setRealName( misdatos->name );
  setIdent( misdatos->user );
  setAutoJoinChannels( QStringList( QLatin1String("#PuntoTorrent") ) );
  connectToServer( QLatin1String("irc.irc-hispano.org"), 6667 );
}

void MyIrcSession::on_timeout() {
  qsrand((unsigned) time(NULL));
  sendRaw(QLatin1String("PING ") +  qrand()%1000000);
}

void MyIrcSession::on_connected() {
  qDebug() << "Conected !!!";
  QStringList canales = autoJoinChannels();
  for ( int i=0; i < canales.size(); i++) {
    cmdJoin(canales.at(i));
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
  cmdJoin( channel );
}

void MyIrcSession::on_channelMessageReceived( const QString& origin, const QString& channel, const QString& message ) {

  Q_UNUSED( channel );
  setEncoding( "iso-8859-1" );
  QString nick = origin.left( origin.indexOf( QLatin1Char( '!' ) ) );
  QString ssubida = QLatin1String("[Nueva Subida] ");
  QString mensaje = QString::fromUtf8( irc_color_strip_from_mirc( message.toUtf8() ) );

  if ( debug ) qDebug() << "message:" << origin << channel << mensaje;

  if ( nick == QLatin1String("PuntoTorrent") ) {
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
    cmdJoin( QLatin1String("#PuntoTorrent") );

  if ( !debug ) return;
  QString stream;
  for (int i = 0; i < params.size(); ++i) {
    stream.append ( params.at(i) + QLatin1Char(' ') );
  }
  qDebug() << "numeric:" << origin << code << stream;
}
