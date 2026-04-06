#ifndef MYIRCSESSION_H
#define MYIRCSESSION_H

#include <QtCore/QObject>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QRandomGenerator>
#include <QtCore/QStringList>

namespace libirc { class ServerAddress; }
namespace libircclient { class Network; class Parser; class Channel; }

struct datosIrc {
  bool	activo;
  QString nick;
  QString user;
  QString name;
  QString server;
  int port;
  QStringList channels;
  QString botNick;
  bool debug;
};

class MyIrcSession : public QObject {
  Q_OBJECT

  public:
    MyIrcSession( QObject* parent = nullptr, datosIrc *datos = nullptr, bool depurar = false);
    ~MyIrcSession() override;

signals:
    void nuevaSubida( QString msg );

  private slots:
    void on_connected();
    void on_timeout();
    void on_privmsg(libircclient::Parser *parser);
    void on_kick(libircclient::Parser *parser, libircclient::Channel *chan);
    void on_numericMessage(libircclient::Parser *parser);

  private:
    static QString irc_color_strip_from_mirc( const QString &source );
    void joinChannels();
    bool debug;
    datosIrc *misdatos;
    QTimer *timer;
    libircclient::Network *network;
};

#endif
