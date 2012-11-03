#include <QtCore/QThread>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QTextCodec>
#include <QtCore/QStringList>
#include <IrcSession>

struct datosIrc {
  bool	activo;
  QString nick;
  QString user;
  QString name;
  bool debug;
};

class MyIrcSession : public Irc::Session {
  Q_OBJECT

  public:
    MyIrcSession( QObject* parent = 0, datosIrc *datos = 0, bool depurar = 0);

    protected slots:
      void on_connected();
    void on_timeout();
    void connectSlotsByName(QObject *source);
    void on_msgNickChanged( const QString& origin, const QString& nick );
    void on_msgQuit( const QString& origin, const QString& message );
    void on_msgJoined( const QString& origin, const QString& channel );
    void on_msgParted( const QString& origin, const QString& channel, const QString& message );
    void on_userModeChanged( const QString& origin, const QString& mode );
    void on_msgModeChanged( const QString& origin, const QString& channel, const QString& mode, const QString& args );
    void on_msgTopicChanged( const QString& origin, const QString& channel, const QString& topic );
    void on_msgKicked( const QString& origin, const QString& channel, const QString& nick, const QString& message );
    void on_channelMessageReceived( const QString& origin, const QString& channel, const QString& message );
    void on_msgMessageReceived( const QString& origin, const QString& receiver, const QString& message );
    void on_msgNoticeReceived( const QString& origin, const QString& receiver, const QString& message );
    void on_msgInvited( const QString& origin, const QString& nick, const QString& channel );
    void on_msgCtcpRequestReceived( const QString& origin, const QString& message );
    void on_msgCtcpReplyReceived( const QString& origin, const QString& message );
    void on_msgCtcpActionReceived( const QString& origin, const QString& message );
    void on_msgUnknownMessageReceived( const QString& origin, const QStringList& params );
    void on_msgNumericMessageReceived( const QString& origin, uint code, const QStringList& params );
signals:
    void nuevaSubida( QString msg );
  private:
    bool debug;
    datosIrc *misdatos;
    QTimer *timer;
};
