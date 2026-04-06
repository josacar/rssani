#include "rssani_lite.h"
#include "rss_lite.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#ifdef __unix__
#include <unistd.h>
#include <sys/socket.h>
#endif

rssani_lite *gRss = nullptr;

#ifdef __unix__
int rssani_lite::sigFd[2] = {0, 0};

void sigHandler( int ) {
  char a = 1;
  ::write(rssani_lite::sigFd[0], &a, sizeof(a));
}
#endif

void rssani_lite::handleSigTerm() {
  snTerm->setEnabled(false);
  char tmp;
  ::read(sigFd[1], &tmp, sizeof(tmp));

  guardar();
  salir();

  snTerm->setEnabled(true);
}

void rssani_lite::debugea(){
  if ( values->Debug() == false ) {
    values->setDebug(true);
    emit timeout();
  } else {
    values->setDebug(false);
  }
}

rssani_lite::rssani_lite( QObject* parent ) : QObject( parent ) {
  gRss = this;
#ifdef __unix__
  ::socketpair(AF_UNIX, SOCK_STREAM, 0, sigFd);
  snTerm = std::make_unique<QSocketNotifier>(sigFd[1], QSocketNotifier::Read, this);
  connect(snTerm.get(), &QSocketNotifier::activated, this, &rssani_lite::handleSigTerm);

  struct sigaction sigTermAction;
  memset( &sigTermAction, 0, sizeof( sigTermAction ) );
  sigTermAction.sa_handler = sigHandler;
  sigemptyset(&sigTermAction.sa_mask);
  sigTermAction.sa_flags = SA_RESTART;

  sigaction( SIGTERM, &sigTermAction, nullptr );
  sigaction( SIGINT, &sigTermAction, nullptr );
  sigaction( SIGUSR1, &sigTermAction, nullptr );
#endif
  tiempo = 10;
  rpcUser = QStringLiteral("rssani-rpc");
  rpcPass = QStringLiteral("rssanipass-rpc");
  values = std::make_unique<Values>();

  lista = std::make_unique<QList<regexp*>>();

  listAuths = std::make_unique<QList<auth>>();
  hashAuths = std::make_unique<QHash<QString, auth>>();

  settings = std::make_unique<QSettings>();
  misdatos.activo = false;

  readSettings();
  QFileInfo fi( settings->fileName() );
  flog = std::make_unique<QFile>( fi.canonicalPath() + QStringLiteral("/rssani.log") );

  timer.start( tiempo * 60 * 1000 );

  rss  = new Rss_lite( values.get(), lista.get(), flog.get(), hashAuths.get(), this );

  if ( misdatos.activo ) {
    session = new MyIrcSession( this,&misdatos,misdatos.debug);
    connect( session, &MyIrcSession::nuevaSubida, this, &rssani_lite::miraSubida );
  } else {
    qDebug() << "***** IRC deshabilitado *****";
  }

  prepareSignals();
}

void rssani_lite::miraSubida( QString subida ) {
  QString url, seccion, titulo;
  int primero = subida.indexOf( QChar('-') );
  int ultimo = subida.lastIndexOf( QChar('-') );
  QRegularExpression rehttp( QStringLiteral("\\s(https?://[\\S]*)") );
  if ( values->Debug() ) qDebug() << "SUBIDA:" << subida;
  QRegularExpressionMatch match = rehttp.match( subida );
  if ( match.hasMatch() ) {
    url = match.captured( 1 );


    seccion = subida.mid( 0, primero - 1 );
    titulo = subida.mid( primero + 2 , ultimo - primero - 3 );
    if ( values->Debug() ) {
      qDebug() << "URL: " << url;
      qDebug() << "TITULO:" << titulo;
      qDebug() << "SECCION:" << seccion;
    }
    emit nuevaSubida ( seccion, titulo.trimmed(), url);
  }
}

rssani_lite::~rssani_lite() {
  qDeleteAll(*lista);
#ifdef __unix__
  ::close(sigFd[0]);
  ::close(sigFd[1]);
#endif
}

// INICIO METODOS RPC

std::string rssani_lite::verUltimo() {
  QMutexLocker<QMutex> locker(&mutex);
  return rss->verUltimo().toString( QStringLiteral("dd/MM/yyyy hh:mm:ss") ).toStdString();
}

int rssani_lite::verTimer() {
  QMutexLocker<QMutex> locker(&mutex);
  return timer.interval();
}

QList<regexp*>* rssani_lite::listaRegexp() {
  QMutexLocker<QMutex> locker(&mutex);
  return lista.get();
}

bool rssani_lite::editarRegexp( std::string regexpOrig, std::string regexpDest ) {
  QMutexLocker<QMutex> locker(&mutex);
  int pos = -1;
  for ( int i = 0; i < lista->size(); ++i ) {
    if ( lista->at( i )->nombre == QString::fromStdString( regexpOrig ) ) {
      pos = i;
      break;
    }
  }
  if ( pos != -1 ) {
    regexp *re = lista->at( pos );
    re->nombre = QString::fromStdString( regexpDest );
    // 		lista->replace( pos, re );
    qDebug()  << "Cambiado" << QString::fromStdString( regexpOrig ) << "--> " << re->nombre;
    return 0;
  } else {
    return 1;
  }
}

bool rssani_lite::editarRegexp( int pos, std::string regexpDest ) {
  QMutexLocker<QMutex> locker(&mutex);
  if ( pos != -1 ) {
    regexp *re = lista->at( pos );
    QString regexpOrig = re->nombre;
    re->nombre = QString::fromStdString( regexpDest );
    qDebug()  << "Cambiado" << regexpOrig << "--> " << re->nombre;
    return 0;
  } else {
    return 1;
  }
}

bool rssani_lite::activarRegexp( int pos ) {
  QMutexLocker<QMutex> locker(&mutex);
  if ( pos != -1 ) {
    regexp *re = lista->at( pos );
    re->activa = ! re->activa;
    qDebug()  << "Cambiado" << !re->activa << "--> " << re->activa;
    return 0;
  } else {
    return 1;
  }
}

void rssani_lite::moverRegexp( int from, int to ) {
  QMutexLocker<QMutex> locker(&mutex);
  if ( from >= 0 && from < lista->size() && to >= 0 && to < lista->size() ) {
    auto *item = lista->takeAt( from );
    lista->insert( to, item );
    qDebug() << "Movido el item" << from << "al" << to;
  }
}

void rssani_lite::anadirRegexp( std::string nombre, std::string fecha, bool mail, std::string tracker, int dias ) {
  QMutexLocker<QMutex> locker(&mutex);
  regexp *re = new regexp();
  re->nombre = QString::fromStdString( nombre );
  re->vencimiento = QString::fromStdString( fecha );
  re->tracker = QString::fromStdString( tracker );
  re->mail = mail;
  re->diasDescarga = dias;
  re->fechaDescarga = QDateTime();
  lista->append( re ) ;
  qDebug()  << "Añadida regexp" << re->nombre << re->vencimiento << re->mail << re->tracker << re->diasDescarga;
}

void rssani_lite::borrarRegexp( int pos ) {
  QMutexLocker<QMutex> locker(&mutex);
  delete( lista->at(pos) );
  lista->removeAt( pos );
}

void rssani_lite::borrarRegexp( std::string cad ) {
  QMutexLocker<QMutex> locker(&mutex);
  for ( int i = 0; i < lista->size(); ++i ) {
    if ( QString::compare( lista->at( i )->nombre , QString::fromStdString( cad ) ) == 0 ) {
      delete lista->at( i );
      lista->removeAt( i );
      qDebug()  << "Borrado regexp" << QString::fromStdString( cad );
    }
  }
}

void rssani_lite::anadirAuth( std::string tracker, std::string uid, std::string pass, std::string passkey ) {
  QMutexLocker<QMutex> locker(&mutex);
  auth au;
  au.tracker = QString::fromStdString( tracker );
  au.uid = QString::fromStdString( uid );
  au.pass = QString::fromStdString( pass );
  au.passkey = QString::fromStdString( passkey );
  listAuths->append( au ) ;
  hashAuths->insert( au.tracker, au );
  qDebug()  << "Añadido auth" << au.tracker << au.uid << au.pass << au.passkey;
}

void rssani_lite::borrarAuth( std::string tracker ) {
  QMutexLocker<QMutex> locker(&mutex);
  for ( int i = 0; i < listAuths->size(); ++i ) {
    if ( QString::compare( listAuths->at( i ).tracker , QString::fromStdString( tracker ) ) == 0 ) {
      listAuths->removeAt( i );
      qDebug()  << "Borrado auth" << QString::fromStdString( tracker );
    }
  }
  // 	delete hashAuths->value( tracker );
  hashAuths->remove( QString::fromStdString( tracker ) );
}

QList<auth>* rssani_lite::listaAuths() {
  QMutexLocker<QMutex> locker(&mutex);
  return listAuths.get();
}

QStringList rssani_lite::verLog() {
  QMutexLocker<QMutex> locker(&mutex);
  QStringList result;
  if ( flog->open( QIODevice::ReadOnly | QIODevice::Text ) ) {
    QTextStream in( flog.get() );
    while ( !in.atEnd() )
      result.append( in.readLine() );
    flog->close();
  }
  return result;
}

void rssani_lite::cambiaTimer( int tiempo ) {
  QMutexLocker<QMutex> locker(&mutex);
  this->tiempo = tiempo;
}

void rssani_lite::guardar() {
  QMutexLocker<QMutex> locker(&mutex);
  writeSettings();
  qDebug() << "Conf. guardada";
}

void rssani_lite::salir() {
  qDebug() << "intentando salir";
  QCoreApplication::quit();
}

void rssani_lite::salYa() {
  qDebug() << "saliendo";
  exit( 0 );
}

void rssani_lite::setRpcUser( std::string theValue ) {
  QMutexLocker<QMutex> locker(&mutex);
  rpcUser = QString::fromStdString( theValue );
}

QString rssani_lite::getRpcUser() {
  QMutexLocker<QMutex> locker(&mutex);
  return rpcUser;
}

void rssani_lite::setRpcPass( std::string theValue ) {
  QMutexLocker<QMutex> locker(&mutex);
  rpcPass = QString::fromStdString( theValue );
}

QString rssani_lite::getRpcPass() {
  QMutexLocker<QMutex> locker(&mutex);
  return rpcPass;
}

// FIN METODOS RPC

void rssani_lite::prepareSignals() {
  connect( &timer, &QTimer::timeout, rss, &Rss_lite::fetch );
  connect( this, &rssani_lite::timeout, rss, &Rss_lite::fetch );
  connect( this, &rssani_lite::nuevaSubida, rss, [this](const QString &s, const QString &t, const QString &u){ rss->miraTitulo(s, t, u); });
}

void rssani_lite::writeSettings() {
  QString dia = QDate::currentDate().toString( QStringLiteral("yyyyMMdd") );
  QFileInfo fi( settings->fileName() );
  QFile::remove( fi.canonicalPath() + QChar('/') + fi.fileName() + QChar('.') + dia );
  QFile::copy( settings->fileName(), fi.canonicalPath() + QChar('/') + fi.fileName() + QChar('.') + dia );
  settings->beginGroup( QStringLiteral("principal") );
  settings->setValue( QStringLiteral("fromMail"), values->FromMail() );
  settings->setValue( QStringLiteral("toMail"), values->ToMail() );
  settings->setValue( QStringLiteral("path"), values->Ruta() );
  settings->setValue( QStringLiteral("debug"), values->Debug() );
  settings->setValue( QStringLiteral("timer"), tiempo );
  settings->setValue( QStringLiteral("fecha"), values->Fecha() );
  settings->setValue( QStringLiteral("rpcUser"), rpcUser );
  settings->setValue( QStringLiteral("rpcPass"), rpcPass );
  settings->setValue( QStringLiteral("smtpServer"), values->SmtpServer() );
  settings->setValue( QStringLiteral("smtpLogin"), values->SmtpLogin() );
  settings->setValue( QStringLiteral("smtpPass"), values->SmtpPass() );
  settings->setValue( QStringLiteral("smtpPort"), values->SmtpPort() );
  settings->endGroup();

  settings->beginGroup( QStringLiteral("regexps") );
  settings->beginWriteArray( QStringLiteral("items") );
  settings->remove( QStringLiteral("") );
  regexp *re;
  qDebug() << "Num. regexps :" << lista->size();
  for ( int i = 0; i < lista->size(); ++i ) {
    settings->setArrayIndex( i );
    re = lista->at( i );
    settings->setValue( QStringLiteral("item"), re->nombre );
    settings->setValue( QStringLiteral("vencimiento"), re->vencimiento );
    settings->setValue( QStringLiteral("mail"), re->mail );
    settings->setValue( QStringLiteral("tracker"), re->tracker );
    settings->setValue( QStringLiteral("dias"), re->diasDescarga );
    if ( re->fechaDescarga.isValid() ) {
      settings->setValue( QStringLiteral("fecha"), re->fechaDescarga );
      qDebug() << "-" << re->nombre << re->vencimiento << re->mail << re->tracker << re->diasDescarga << re->fechaDescarga.toString( Qt::ISODate );
    } else {
      qDebug() << "-" << re->nombre << re->vencimiento << re->mail << re->tracker << re->diasDescarga;
    }
    settings->setValue( QStringLiteral("activa"), re->activa );
  }
  settings->endArray();
  settings->endGroup();

  // auths para los trackers
  settings->beginGroup( QStringLiteral("trackers") );
  settings->beginWriteArray( QStringLiteral("trackers") );
  settings->remove( QStringLiteral("") );
  qDebug() << "Num. trackers :" << listAuths->size();
  auth au;
  for ( int i = 0; i < listAuths->size(); ++i ) {
    settings->setArrayIndex( i );
    au = listAuths->at( i );
    settings->setValue( QStringLiteral("tracker"), au.tracker );
    settings->setValue( QStringLiteral("uid"), au.uid );
    settings->setValue( QStringLiteral("pass"), au.pass );
    settings->setValue( QStringLiteral("passkey"), au.passkey );
    settings->setValue( QStringLiteral("referer"), au.referer );
    settings->setValue( QStringLiteral("idField"), au.idField );
    settings->setValue( QStringLiteral("urlDownload"), au.urlDownload );
    settings->setValue( QStringLiteral("urlRss"), au.urlRss );
    qDebug() << "-" << au.tracker << au.uid << au.pass << au.passkey;
  }

  settings->endArray();
  settings->endGroup();

  settings->beginGroup( QStringLiteral("irc") );
  settings->setValue( QStringLiteral("nickIrc"), misdatos.nick );
  settings->setValue( QStringLiteral("userIrc"), misdatos.user );
  settings->setValue( QStringLiteral("nameIrc"), misdatos.name );
  settings->setValue( QStringLiteral("serverIrc"), misdatos.server );
  settings->setValue( QStringLiteral("portIrc"), misdatos.port );
  settings->setValue( QStringLiteral("channelsIrc"), misdatos.channels );
  settings->setValue( QStringLiteral("botNickIrc"), misdatos.botNick );
  settings->setValue( QStringLiteral("debugIrc"), misdatos.debug );
  settings->endGroup();

  settings->sync();
}

void rssani_lite::readSettings() {
  settings->beginGroup( QStringLiteral("principal") );
  values->setFromMail( settings->value( QStringLiteral("fromMail") ).toString() );
  values->setToMail( settings->value( QStringLiteral("toMail") ).toString() );
  values->setRuta( settings->value( QStringLiteral("path") ).toString() );
  values->setDebug( settings->value( QStringLiteral("debug"), false ).toBool() );

  tiempo = settings->value( QStringLiteral("timer") ).toInt();
  values->setFecha( settings->value( QStringLiteral("fecha") ).toString() );

  rpcUser = settings->value( QStringLiteral("rpcUser"), QStringLiteral("rssani-rpc") ).toString();
  rpcPass = settings->value( QStringLiteral("rpcPass"), QStringLiteral("rssanipass-rpc") ).toString();
  values->setSmtpServer( settings->value( QStringLiteral("smtpServer") ).toString() );
  values->setSmtpLogin( settings->value( QStringLiteral("smtpLogin") ).toString() );
  values->setSmtpPass( settings->value( QStringLiteral("smtpPass") ).toString() );
  values->setSmtpPort( settings->value( QStringLiteral("smtpPort"), 587 ).toInt() );
  settings->endGroup();

  settings->beginGroup( QStringLiteral("regexps") );
  int size = settings->beginReadArray( QStringLiteral("items") );
  regexp *re;
  qDebug() << "Num. regexps :" << size;
  for ( int i = 0; i < size; ++i ) {
    settings->setArrayIndex( i );
    re = new regexp();
    re->nombre = settings->value( QStringLiteral("item") ).toString();
    re->vencimiento = settings->value( QStringLiteral("vencimiento") ).toString();
    re->mail = settings->value( QStringLiteral("mail") ).toBool();
    re->tracker = settings->value( QStringLiteral("tracker") ).toString();
    re->diasDescarga = settings->value( QStringLiteral("dias"), 0 ).toInt();
    if ( settings->value( QStringLiteral("fecha") ).toDateTime().isNull() ) {
      re->fechaDescarga = QDateTime();
    } else {
      re->fechaDescarga = settings->value( QStringLiteral("fecha")).toDateTime();
    }
    re->activa = settings->value( QStringLiteral("activa"), true ).toBool();
    lista->append( re );

    if ( re->fechaDescarga.isValid() )
      qDebug() << "-" << re->nombre << re->vencimiento << re->mail << re->tracker << re->diasDescarga << re->fechaDescarga.toString( QStringLiteral("dd-MM-yyyy") );
    else 
      qDebug() << "-" << re->nombre << re->vencimiento << re->mail << re->tracker << re->diasDescarga;
  }

  settings->endArray();
  settings->endGroup();

  settings->beginGroup( QStringLiteral("trackers") );
  // auths para los trackers
  size = settings->beginReadArray( QStringLiteral("trackers") );
  auth au;
  qDebug() << "Num. trackers :" << size;
  for ( int i = 0; i < size; ++i ) {
    settings->setArrayIndex( i );
    au.tracker =	settings->value( QStringLiteral("tracker") ).toString();
    au.uid = settings->value( QStringLiteral("uid") ).toString();
    au.pass = settings->value( QStringLiteral("pass") ).toString();
    au.passkey = settings->value( QStringLiteral("passkey") ).toString();
    au.referer = settings->value( QStringLiteral("referer"), QStringLiteral("/browse.php") ).toString();
    au.idField = settings->value( QStringLiteral("idField"), QStringLiteral("id") ).toString();
    au.urlDownload = settings->value( QStringLiteral("urlDownload"), QStringLiteral("/download.php?id=") ).toString();
    au.urlRss = settings->value( QStringLiteral("urlRss"), QStringLiteral("/rss.php") ).toString();
    listAuths->append( au );
    hashAuths->insert( au.tracker, au );
    qDebug() << "-" << au.tracker << au.uid << au.pass << au.passkey;
  }

  settings->endArray();
  settings->endGroup();

  settings->beginGroup( QStringLiteral("irc") );
  // auths para el irc
  misdatos.nick = settings->value( QStringLiteral("nickIrc") ).toString();
  misdatos.user = settings->value( QStringLiteral("userIrc") ).toString();
  misdatos.name = settings->value( QStringLiteral("nameIrc") ).toString();
  misdatos.server = settings->value( QStringLiteral("serverIrc"), QStringLiteral("irc.irc-hispano.org") ).toString();
  misdatos.port = settings->value( QStringLiteral("portIrc"), 6667 ).toInt();
  misdatos.channels = settings->value( QStringLiteral("channelsIrc") ).toStringList();
  if ( misdatos.channels.isEmpty() ) misdatos.channels << QStringLiteral("#PuntoTorrent");
  misdatos.botNick = settings->value( QStringLiteral("botNickIrc"), QStringLiteral("PuntoTorrent") ).toString();
  misdatos.debug = settings->value( QStringLiteral("debugIrc"), false ).toBool();
  if ( !misdatos.nick.isEmpty() && !misdatos.nick.startsWith( QStringLiteral("OFF:") ) ) misdatos.activo = true;

  settings->endGroup();
}


Values* rssani_lite::getValues() const {
  QMutexLocker<QMutex> locker(&mutex);
  return values.get();
}

