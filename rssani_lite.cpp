#include "rssani_lite.h"
#include "rss_lite.h"

rssani_lite *gRss = nullptr;

#ifdef __unix__
void sigHandler( int signo ) {
  if ( gRss != nullptr )
    switch ( signo ) {
      case SIGUSR1:
        if ( gRss ) gRss->debugea();
        break;
      case SIGINT:
      case SIGTERM:
        gRss->guardar();
        gRss->salir();
    }
}
#endif

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
  struct sigaction sigTermAction;
  memset( &sigTermAction, 0, sizeof( sigTermAction ) );

  sigTermAction.sa_handler = sigHandler;
  sigTermAction.sa_restorer = NULL;

  sigaction( SIGTERM, &sigTermAction, NULL );
  sigaction( SIGINT, &sigTermAction, NULL );
  sigaction( SIGUSR1, &sigTermAction, NULL );
#endif
  tiempo = 10;
  rpcUser = QLatin1String("rssani-rpc");
  rpcPass = QLatin1String("rssanipass-rpc");
  values = new Values();

  lista = new QList<regexp*>();

  listAuths = new QList<auth>;
  hashAuths = new QHash<QString, auth>;

  settings = new QSettings();
  misdatos.activo = false;

  readSettings();
  QFileInfo fi( settings->fileName() );
  flog = new QFile( fi.canonicalPath() + QLatin1String( "/rssani.log" ) );

  timer.start( tiempo * 60 * 1000 );

  rss  = new Rss_lite( values, lista, flog, hashAuths, this );

  if ( misdatos.activo ) {
    session = new MyIrcSession( this,&misdatos,misdatos.debug);
    connect( session, SIGNAL( nuevaSubida( QString ) ), this, SLOT( miraSubida( QString ) ) );
  } else {
    qDebug() << "***** IRC deshabilitado *****";
  }

  prepareSignals();
}

void rssani_lite::miraSubida( QString subida ) {
  QString url, seccion, titulo;
  int primero = subida.indexOf( QLatin1Char('-') );
  int ultimo = subida.lastIndexOf( QLatin1Char('-') );
  QRegExp rehttp = QRegExp( QLatin1String("\\s(http://[\\S]*)") );
  if ( values->Debug() ) qDebug() << "SUBIDA:" << subida;
  int pos = rehttp.indexIn( subida );
  if ( pos > -1 ) {
    url = rehttp.cap( 1 );


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
  delete rss;
  delete settings;
  delete values;
  delete lista;
  delete flog;
}

// INICIO METODOS RPC

std::string rssani_lite::verUltimo() {
  return rss->verUltimo().toString( QLatin1String( "dd/MM/yyyy hh:mm:ss" ) ).toStdString();
}

int rssani_lite::verTimer() {
  return timer.interval();
}

QList<regexp*>* rssani_lite::listaRegexp() {
  return lista;
}

bool rssani_lite::editarRegexp( std::string regexpOrig, std::string regexpDest ) {
  int pos = -1; // FIXME: Asegurarse que cambia la del tracker y no otra
  for ( int i = 0; i < lista->size(); ++i ) {
    if ( QString::compare( lista->at( i )->nombre , QString::fromStdString( regexpOrig ) ) == 0 ) {
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
  if ( from >= 0 && from < lista->size() && to >= 0 && to < lista->size() ) {
    lista->move( from, to );
    qDebug() << "Movido el item" << from << "al" << to;
  }
}

void rssani_lite::anadirRegexp( std::string nombre, std::string fecha, bool mail, std::string tracker, int dias ) {
  regexp *re = new regexp();
  re->nombre = QString::fromStdString( nombre );
  re->vencimiento = QString::fromStdString( fecha );
  re->tracker = QString::fromStdString( tracker );
  re->mail = mail;
  re->diasDescarga = dias;
  re->fechaDescarga = NULL;
  lista->append( re ) ;
  qDebug()  << "Añadida regexp" << re->nombre << re->vencimiento << re->mail << re->tracker << re->diasDescarga;
}

void rssani_lite::borrarRegexp( int pos ) {
  delete( lista->at(pos) );
  lista->removeAt( pos );
}

void rssani_lite::borrarRegexp( std::string cad ) {
  for ( int i = 0; i < lista->size(); ++i ) {
    if ( QString::compare( lista->at( i )->nombre , QString::fromStdString( cad ) ) == 0 ) {
      lista->removeAt( i );
      qDebug()  << "Borrado regexp" << QString::fromStdString( cad );
    }
  }
}

void rssani_lite::anadirAuth( std::string tracker, std::string uid, std::string pass, std::string passkey ) {
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
  return listAuths;
}

QStringList* rssani_lite::verLog() {
  if ( flog->open( QIODevice::ReadOnly | QIODevice::Text ) ) {
    QTextStream in( flog );
    QStringList *lista = new QStringList();

    while ( !in.atEnd() )
      lista->append( in.readLine() );

    flog->close();

    return lista;
  } else {
    return new QStringList();
  }
}

void rssani_lite::cambiaTimer( int tiempo ) {
  this->tiempo = tiempo;
}

void rssani_lite::guardar() {
  writeSettings();
  qDebug() << "Conf. guardada";
}

void rssani_lite::salir() {
  qDebug() << "intentando salir";
  // 	shutdown.start( 5000 );
  // 	connect ( &shutdown, SIGNAL ( timeout() ), this, SLOT ( salYa() ) );
  exit( 0 );
}

void rssani_lite::salYa() {
  qDebug() << "saliendo";
  exit( 0 );
}

void rssani_lite::setRpcUser( std::string theValue ) {
  rpcUser = QString::fromStdString( theValue );
}

QString rssani_lite::getRpcUser() {
  return rpcUser;
}

void rssani_lite::setRpcPass( std::string theValue ) {
  rpcPass = QString::fromStdString( theValue );
}

QString rssani_lite::getRpcPass() {
  return rpcPass;
}

// FIN METODOS RPC

void rssani_lite::prepareSignals() {
  connect( &timer, SIGNAL( timeout() ), rss, SLOT( fetch() ) );
  connect( this, SIGNAL( timeout() ), rss, SLOT( fetch() ) );
  connect( this, SIGNAL(nuevaSubida ( QString, QString, QString) ), rss, SLOT( miraTitulo(QString, QString, QString)) );
}

void rssani_lite::writeSettings() {
  QString dia = QDate::currentDate().toString( QLatin1String("yyyyMMdd") );
  QFileInfo fi( settings->fileName() );
  QFile::remove( fi.canonicalPath() + QLatin1Char('/') + fi.fileName() + QLatin1Char('.') + dia );
  QFile::copy( settings->fileName(), fi.canonicalPath() + QLatin1Char('/') + fi.fileName() + QLatin1Char('.') + dia );
  settings->beginGroup( QLatin1String("principal") );
  settings->setValue( QLatin1String("fromMail"), values->FromMail() );
  settings->setValue( QLatin1String("toMail"), values->ToMail() );
  settings->setValue( QLatin1String("path"), values->Ruta() );
  settings->setValue( QLatin1String("debug"), values->Debug() );
  settings->setValue( QLatin1String("timer"), tiempo );
  settings->setValue( QLatin1String("fecha"), values->Fecha() );
  settings->setValue( QLatin1String("rpcUser"), rpcUser );
  settings->setValue( QLatin1String("rpcPass"), rpcPass );
  settings->endGroup();

  settings->beginGroup( QLatin1String("regexps") );
  settings->beginWriteArray( QLatin1String("items") );
  settings->remove( QLatin1String("") );
  regexp *re;
  qDebug() << "Num. regexps :" << lista->count();
  for ( int i = 0; i < lista->count(); ++i ) {
    settings->setArrayIndex( i );
    re = lista->at( i );
    settings->setValue( QLatin1String("item"), re->nombre );
    settings->setValue( QLatin1String("vencimiento"), re->vencimiento );
    settings->setValue( QLatin1String("mail"), re->mail );
    settings->setValue( QLatin1String("tracker"), re->tracker );
    settings->setValue( QLatin1String("dias"), re->diasDescarga );
    if ( re->fechaDescarga ) {
      settings->setValue( QLatin1String("fecha"), *re->fechaDescarga );
      qDebug() << "-" << re->nombre << re->vencimiento << re->mail << re->tracker << re->diasDescarga << re->fechaDescarga->toString( Qt::DefaultLocaleShortDate );
    } else {
      qDebug() << "-" << re->nombre << re->vencimiento << re->mail << re->tracker << re->diasDescarga;
    }
    settings->setValue( QLatin1String("activa"), re->activa );
  }
  settings->endArray();
  settings->endGroup();

  // auths para los trackers
  settings->beginGroup( QLatin1String("trackers") );
  settings->beginWriteArray( QLatin1String("trackers") );
  settings->remove( QLatin1String("") );
  qDebug() << "Num. trackers :" << listAuths->count();
  auth au;
  for ( int i = 0; i < listAuths->count(); ++i ) {
    settings->setArrayIndex( i );
    au = listAuths->at( i );
    settings->setValue( QLatin1String("tracker"), au.tracker );
    settings->setValue( QLatin1String("uid"), au.uid );
    settings->setValue( QLatin1String("pass"), au.pass );
    settings->setValue( QLatin1String("passkey"), au.passkey );
    qDebug() << "-" << au.tracker << au.uid << au.pass << au.passkey;
  }

  settings->endArray();
  settings->endGroup();

  settings->beginGroup( QLatin1String("irc") );
  settings->setValue( QLatin1String("nickIrc"), misdatos.nick );
  settings->setValue( QLatin1String("userIrc"), misdatos.user );
  settings->setValue( QLatin1String("nameIrc"), misdatos.name );
  settings->setValue( QLatin1String("debugIrc"), misdatos.debug );
  settings->endGroup();

  settings->sync();
}

void rssani_lite::readSettings() {
  settings->beginGroup( QLatin1String("principal") );
  values->setFromMail( settings->value( QLatin1String("fromMail") ).toString() );
  values->setToMail( settings->value( QLatin1String("toMail") ).toString() );
  values->setRuta( settings->value( QLatin1String("path") ).toString() );
  values->setDebug( settings->value( QLatin1String("debug"), false ).toBool() );

  tiempo = settings->value( QLatin1String("timer") ).toInt();
  values->setFecha( settings->value( QLatin1String("fecha") ).toString() );

  rpcUser = settings->value( QLatin1String("rpcUser"), QLatin1String("rssani-rpc") ).toString();
  rpcPass = settings->value( QLatin1String("rpcPass"), QLatin1String("rssanipass-rpc") ).toString();
  settings->endGroup();

  settings->beginGroup( QLatin1String("regexps") );
  int size = settings->beginReadArray( QLatin1String("items") );
  regexp *re;
  qDebug() << "Num. regexps :" << size;
  for ( int i = 0; i < size; ++i ) {
    settings->setArrayIndex( i );
    re = new regexp();
    re->nombre = settings->value( QLatin1String("item") ).toString();
    re->vencimiento = settings->value( QLatin1String("vencimiento") ).toString();
    re->mail = settings->value( QLatin1String("mail") ).toBool();
    re->tracker = settings->value( QLatin1String("tracker") ).toString();
    re->diasDescarga = settings->value( QLatin1String("dias"), 0 ).toInt();
    if ( settings->value( QLatin1String("fecha") ).toDateTime().isNull() ) {
      re->fechaDescarga = NULL; // TODO: Ver estooooo
    } else {
      re->fechaDescarga = new QDateTime(settings->value( QLatin1String("fecha")).toDateTime() );
    }
    re->activa = settings->value( QLatin1String("activa"), true ).toBool();
    lista->append( re );

    if ( re->fechaDescarga )
      qDebug() << "-" << re->nombre << re->vencimiento << re->mail << re->tracker << re->diasDescarga << re->fechaDescarga->toString( QLatin1String("dd-MM-yyyy") );
    else 
      qDebug() << "-" << re->nombre << re->vencimiento << re->mail << re->tracker << re->diasDescarga;
  }

  settings->endArray();
  settings->endGroup();

  settings->beginGroup( QLatin1String("trackers") );
  // auths para los trackers
  size = settings->beginReadArray( QLatin1String("trackers") );
  auth au;
  qDebug() << "Num. trackers :" << size;
  for ( int i = 0; i < size; ++i ) {
    settings->setArrayIndex( i );
    au.tracker =	settings->value( QLatin1String("tracker") ).toString();
    au.uid = settings->value( QLatin1String("uid") ).toString();
    au.pass = settings->value( QLatin1String("pass") ).toString();
    au.passkey = settings->value( QLatin1String("passkey") ).toString();
    listAuths->append( au );
    hashAuths->insert( au.tracker, au );
    qDebug() << "-" << au.tracker << au.uid << au.pass << au.passkey;
  }

  settings->endArray();
  settings->endGroup();

  settings->beginGroup( QLatin1String("irc") );
  // auths para el irc
  misdatos.nick = settings->value( QLatin1String("nickIrc") ).toString();
  misdatos.user = settings->value( QLatin1String("userIrc") ).toString();
  misdatos.name = settings->value( QLatin1String("nameIrc") ).toString();
  misdatos.debug = settings->value( QLatin1String("debugIrc"), false ).toBool();
  if ( !misdatos.nick.isEmpty() && !misdatos.nick.startsWith( QLatin1String("OFF:") ) ) misdatos.activo = true;

  settings->endGroup();
}


Values* rssani_lite::getValues() const {
  return values;
}


void rssani_lite::setValues( Values* theValue ) {
  values = theValue;
}
