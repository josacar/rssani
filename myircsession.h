#ifndef MYIRCSESSION_H
#define MYIRCSESSION_H

#include <QtCore/QObject>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QRandomGenerator>
#include <QtCore/QStringList>

namespace libirc { class ServerAddress; }
namespace libircclient { class Network; class Parser; class Channel; }

/**
 * @brief IRC connection parameters.
 */
struct datosIrc {
  bool	activo;       ///< Whether the IRC connection is active.
  QString nick;       ///< Nickname used on the IRC server.
  QString user;       ///< Username (ident) for the IRC connection.
  QString name;       ///< Real name sent to the IRC server.
  QString server;     ///< IRC server hostname.
  int port;           ///< IRC server port.
  QStringList channels; ///< List of channels to join.
  QString botNick;    ///< Nick of the bot that announces uploads.
  bool debug;         ///< Enable debug logging for IRC.
};

/**
 * @brief IRC client that monitors channels for new upload announcements.
 */
class MyIrcSession : public QObject {
  Q_OBJECT

  friend class TestMyIrcSession;

  public:
    /**
     * @brief Constructs the IRC session.
     * @param parent Parent QObject.
     * @param datos IRC connection parameters.
     * @param depurar Enable debug logging.
     */
    MyIrcSession( QObject* parent = nullptr, datosIrc *datos = nullptr, bool depurar = false);

    /**
     * @brief Destructor.
     */
    ~MyIrcSession() override;

signals:
    /**
     * @brief Emitted when a new upload is announced on IRC.
     * @param msg The announcement message.
     */
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
