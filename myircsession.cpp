#include <unistd.h>
#include "myircsession.h"
#include <libircclient/network.h>
#include <libircclient/parser.h>
#include <libircclient/channel.h>
#include <libircclient/user.h>
#include <libirc/serveraddress.h>

QString MyIrcSession::irc_color_strip_from_mirc( const QString &source ) {
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
        if ( i + 1 < source.size() && source.at( i + 1 ) == QChar(',') && i + 2 < source.size() && source.at( i + 2 ).isDigit() ) {
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

MyIrcSession::MyIrcSession( QObject* parent, datosIrc *datos, bool depurar ) : QObject( parent ) {
  misdatos = datos;
  debug = depurar;

  qDebug() << "Debug is : " << debug;

  libirc::ServerAddress addr(misdatos->server, false, misdatos->port,
                             misdatos->nick, QString(), misdatos->user);
  addr.SetRealname(misdatos->name);

  network = new libircclient::Network(addr, QStringLiteral("rssani"), libircclient::EncodingUTF8);

  connect( network, &libircclient::Network::Event_Connected, this, &MyIrcSession::on_connected );
  connect( network, &libircclient::Network::Event_PRIVMSG, this, &MyIrcSession::on_privmsg );
  connect( network, &libircclient::Network::Event_SelfKick, this, &MyIrcSession::on_kick );
  connect( network, &libircclient::Network::Event_MOTDEnd, this, &MyIrcSession::on_numericMessage );

  timer = new QTimer(this);
  connect( timer, &QTimer::timeout, this, &MyIrcSession::on_timeout );
  timer->start(120000);

  network->Connect();
}

MyIrcSession::~MyIrcSession() {
  delete network;
}

void MyIrcSession::joinChannels() {
  for ( const auto &ch : misdatos->channels )
    network->RequestJoin(ch);
}

void MyIrcSession::on_timeout() {
  network->TransferRaw(QStringLiteral("PING ") + QString::number(QRandomGenerator::global()->bounded(1000000)));
}

void MyIrcSession::on_connected() {
  qDebug() << "Conected !!!";
  joinChannels();
}

void MyIrcSession::on_numericMessage(libircclient::Parser *parser) {
  Q_UNUSED(parser);
  // MOTD end (376) — join channels as fallback
  joinChannels();
}

void MyIrcSession::on_privmsg(libircclient::Parser *parser) {
  if ( !parser || !parser->GetSourceUserInfo() ) return;

  QString nick = parser->GetSourceUserInfo()->GetNick();
  QList<QString> params = parser->GetParameters();
  QString channel = params.isEmpty() ? QString() : params.first();
  QString message = parser->GetText();

  if ( debug ) qDebug() << "message:" << nick << channel << message;

  if ( !channel.startsWith( QChar('#') ) ) return;

  QString ssubida = QStringLiteral("[Nueva Subida] ");
  QString mensaje = irc_color_strip_from_mirc( message );

  if ( nick == misdatos->botNick ) {
    if ( mensaje.startsWith( ssubida ) ) {
      emit nuevaSubida( mensaje.mid( ssubida.length() ) );
    }
  }
}

void MyIrcSession::on_kick(libircclient::Parser *parser, libircclient::Channel *chan) {
  Q_UNUSED(parser);
  if ( !chan ) return;
  QString channel = chan->GetName();
  qDebug() << "Kicked from channel:" << channel;
  sleep( 5 );
  network->RequestJoin(channel);
}
