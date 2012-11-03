#include "rss_lite.h"
/**
 * Constructor de la clase RSS
 * @param values Lista de opciones guardadas
 * @param lista Lista de regexps
 * @param log Ruta absoluta del fichero log
 * @param parent Puntero a la clase padre
 */
Rss_lite::Rss_lite( Values* values, QList<regexp*>* lista, QFile* log, QHash<QString, auth> *auths, QObject* parent ) : QObject( parent ) {
  this->values = values;
  this->lista = lista;
  this->log = log;
  this->hashAuths = auths;
  prepareSignals();

  QFileInfo fi( *log );
  matches = new QFile( fi.canonicalPath() + QLatin1String( "/matches.log" ) );

  ultimoRss = QDateTime::currentDateTime();
  iniciaTrackers();
}

/**
 * Destructor por defecto
 */
Rss_lite::~Rss_lite() {
  delete url;
  delete matches;
}

/**
 * Muestra la fecha del ultimo get
 * @return El string con la fecha
 */
QDateTime Rss_lite::verUltimo() {
  return ultimoRss;
}

/**
 * Prepara las señales de las descargas RSS y Torrent. También los link correctos.
 * @see Rss()
 * @see ~Rss()
 */

void Rss_lite::prepareSignals() {
  connect( &httpRss, SIGNAL( readyRead( const QHttpResponseHeader & ) ),
      this, SLOT( readDataRSS( const QHttpResponseHeader & ) ) );

  connect( &httpTorrent, SIGNAL( readyRead( const QHttpResponseHeader & ) ),
      this, SLOT( readDataTorrent( const QHttpResponseHeader & ) ) );

  connect( this, SIGNAL( linkCorrecto( QString , QString ) ),
      this, SLOT( parseLink( QString , QString ) ) );

  connect( &httpRss, SIGNAL( requestFinished( int, bool ) ),
      this, SLOT( finishedRSS( int, bool ) ) );

  connect( &httpTorrent, SIGNAL( requestFinished( int, bool ) ),
      this, SLOT( finishedTorrent( int, bool ) ) );

}

// INICIO MANEJO DEL RSS

/**
 * Hace un get al RSS, y envia la señal de lectura a readDataRSS
 */

void Rss_lite::fetch() {
  descargas.clear();
  httpRss.abort();

  if ( !values->filledValues() ) {
    std::cerr << "Configuración errónea" << std::endl;
    exit( 1 );
  }

  tracker *trk = NULL;

  for ( int i = 0; i < listaTrackers.size(); ++i ) {
    trk = trackers.value( listaTrackers.at( i ) );
    if ( trk == NULL ) continue;
    url = new QUrl( trk->urlTracker );
    if ( xmls.contains( url->host() ) ) xmls.value( url->host() )->clear(); //TODO: mirar
    QHttpRequestHeader header( QLatin1String( "GET" ), trk->urlRss );
    header.setValue( QLatin1String( "Host" ), url->host() );
    header.setValue( QLatin1String( "Cookie" ), trk->cookie );
    header.setValue( QLatin1String( "Referer" ), trk->urlTracker + trk->referer );

    httpRss.setHost( url->host() );
    qDebug() << "+ Me bajo" << url->host() << trk->urlRss;
    connectionIdRSS = httpRss.request( header );
    trk = NULL;
    delete url;
  }

  ultimoRss = QDateTime::currentDateTime();
}

/**
 * Parsea el XML conforme le va llegando
 * @param &resp Encabezado de respuesta HTTP
 */

void Rss_lite::readDataRSS( const QHttpResponseHeader &resp ) {
  QString host = httpRss.currentRequest().value( QLatin1String( "Host" ) );
  QString xml;

  if ( resp.statusCode() != 200 )
    httpRss.abort();
  else {
    if ( ! xmls.count( host ) ) {
      xmls.insert( host, new QXmlStreamReader() );
    }
    xml = QLatin1String( httpRss.readAll() );
    xmls.value( host )->addData( xml );
    parseXml( xmls.value( host ) );
  }
}

/**
 * Cuando finaliza habilitamos el botón
 * @param id Id de la conexion RSS
 * @param error Bool que indica error de la conexion
 */

void Rss_lite::finishedRSS( int id, bool error ) {
  QTextStream out( stdout );
  pila.clear();
  if ( error ) {
    qWarning( "Received error during HTTP fetch rss." );
  } else {
    if ( connectionIdRSS == id ) {
      out << QDateTime::currentDateTime().toString( QLatin1String( " dd/MM/yyyy hh:mm:ss " ) ) << "RSS bajado !!!\n";
    }
  }
}

/**
 * Slot que mira la regexp
 * @param seccion La seccion de la descarga
 * @param titulo El titulo de la descarga
 * @param link El enlace de la descarga
 */
void Rss_lite::miraTitulo( QString seccion, QString titulo, QString link, bool fromIrc ) {
  switch ( parseTitle( seccion , titulo , link, fromIrc ) ) {
    case 1:
      emit linkCorrecto( link, titulo );
    case 2:
      if ( recientes.size() > 30 ) recientes.removeFirst();
      recientes.append( titulo );
  }
}

/**
 * Parsea el XML y ve si algo nos interesa
 * @param xml Buffer con el XML de cada site
 */

void Rss_lite::parseXml( QXmlStreamReader *xml ) {
  QString dateS;
  QRegExp rxlen( QLatin1String( "Category: (\\S+)" ) );
  QRegExp syl( QLatin1String( "\\s\\[S\\(\\d+\\)/L\\(\\d+\\)\\]" ) );
  QRegExp titulo( QLatin1String( "^\\[([^]]+)\\]\\s" ) );
  QTextStream out( stdout );

  int pos = -1;
  QString value;
  QTextCodec *codec;
  if ( locale.isEmpty() )
    codec = QTextCodec::codecForName( "iso-8859-1" );
  else
    codec = QTextCodec::codecForName( locale.toUtf8() );
  QTextCodec::setCodecForCStrings( codec );

  while ( !xml->atEnd() ) {
    xml->readNext();

    if ( xml->isStartElement() ) {
      if ( xml->name() == "item" ) {
        linkString = xml->attributes().value( QLatin1String( "rss:about" ) ).toString();
        pila.push( 0 );
      }
      if ( xml->name() == "enclosure" ) { // Con esto machaco
        linkString = xml->attributes().value( QLatin1String( "url" ) ).toString();
        // 				qDebug() << "Enclosure:" << linkString;
      }
      currentTag = xml->name().toString();
    } else if ( xml->isEndElement() ) {
      if ( xml->name() == "item" ) {
        QDateTime date = QDateTime::fromString( pubDate.section( QLatin1Char( ' ' ), 1, 4 ), QLatin1String( "dd MMM yyyy hh:mm:ss" ) );
        dateS = date.toString( QLatin1String( "yyyyMMddhhmmss" ) );

        pos = rxlen.indexIn( description );
        if ( pos > -1 ) value = rxlen.cap( 1 );

        pos = -1;
        int pos = titulo.indexIn( titleString );
        if ( pos > -1 ) value = titulo.cap( 1 );

        titleString.remove( syl ); // Quitamos los putos peers y leechers
        titleString.remove( titulo );

        miraTitulo( value, titleString, linkString, false );

        titleString.clear();
        linkString.clear();
        pubDate.clear();
        description.clear();
        pila.pop();
      }
    } else if ( xml->isCharacters() && !xml->isWhitespace() && !pila.isEmpty() ) {
      if ( currentTag == QLatin1String( "title" ) ) titleString += xml->text().toString();
      else if ( currentTag == QLatin1String( "link" ) ) linkString += xml->text().toString();
      else if ( currentTag == QLatin1String( "pubDate" ) ) pubDate += xml->text().toString();
      else if ( currentTag == QLatin1String( "description" ) ) description += xml->text().toString();
    } else if ( xml->isStartDocument() ) {
      out << "\nXML Enc:" << xml->documentEncoding().toString() << " ";
      locale = xml->documentEncoding().toString();
    }
  }

  if ( xml->error() && xml->error() != QXmlStreamReader::PrematureEndOfDocumentError ) {
    qWarning() << "XML ERROR:" << xml->lineNumber() << ": " << xml->errorString() << " con id " << xml->error();
    titleString.clear();
    linkString.clear();
    pubDate.clear();
    description.clear();
    pila.pop();
    httpRss.abort();
    xml->clear();
  }

  // 	if ( xml->atEnd() ) {
  // 		qDebug() << "Limpio el stream XML";
  // 		xml->clear();
  // 	}
}

/**
 * Mira a ver si coincide con alguna regexp y devuelve true
 * @param titleString Nombre de la descarga
 * @param linkString Link de la descarga
 * @return Si hace match con alguna regexp de la lista
 */

int Rss_lite::parseTitle( QString seccion, QString titulo,  QString enlace, bool fromIrc ) {
  QTextStream out( stdout );
  QUrl urlLink( enlace );
  QUrl urlRegexp;
  int exito = -1;

  QString subida = titulo;
  if ( !seccion.isEmpty() ) subida = QLatin1Char( '(' ) + seccion + QLatin1String( ") " ) + titulo;

  if ( values->Debug() )
    out << "Analiz. : " << subida << "\n";//<< enlace;

  if ( ! fromIrc ) out << ".";
  if ( recientes.contains( titulo ) ) return 0;

  for ( int i = 0;i < lista->count();i++ ) {
    // Quitamos lo que haya vencido
    if ( lista->at( i )->vencimiento != QLatin1String( "" ) )
      if ( QDate::currentDate() >= QDate::fromString( lista->at( i )->vencimiento, QLatin1String( "dd-MM-yyyy" ) ) ) {
        out  << "Borrado caducado" << lista->at( i )->nombre << "\n";
        lista->removeAt( i );
        i--;
        continue;
      }

    if ( lista->at( i )->activa && subida.contains( QRegExp( lista->at( i )->nombre, Qt::CaseInsensitive ) ) ) {
      if ( lista->at( i )->tracker != QLatin1String( "" ) ) { // Si tiene tracker especifico miro a ver y si no drop
        urlRegexp = QUrl( lista->at( i )->tracker );
        if ( values->Debug() )
          qDebug() << "Tracker" << urlRegexp.host() << urlLink.host();
        if ( urlLink.host() != urlRegexp.host() ) continue;
      }

      if ( matches->open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append ) ) {  // Lo escribimos en el log
        QTextStream outFile( matches );
        outFile << subida << " --> " << lista->at( i )->nombre << endl;
        matches->close();
      }

      // Miramos si solo mail
      if ( lista->at( i )->mail == 1 ) {
        out << "\nINFO: " << titulo << "\n";
        exito = sendMail(
            QLatin1String( "RSSINFO " ) + QHostInfo::localHostName() + QDateTime::currentDateTime().toString( QLatin1String( " dd/MM/yyyy hh:mm:ss" ) ),
            subida + QLatin1String( "\nLINK   : " ) + enlace );

        if ( exito == 0 )
          return 2;// Si es igual pasando, que ya hemos avisado...
        else
          return 3;
      }

      // Vemos si han pasado los dias minimos entre descargas


      if ( lista->at( i )->fechaDescarga != NULL ) {
        if ( values->Debug() )
          qDebug() << "diasDescarga:" << lista->at( i )->fechaDescarga->daysTo( QDateTime::currentDateTime() ) << "Dias:" << lista->at( i )->diasDescarga;
        if ( lista->at( i )->fechaDescarga->daysTo( QDateTime::currentDateTime() ) < lista->at( i )->diasDescarga ) {
          continue;
        }
      }

      delete( lista->at( i )->fechaDescarga );
      lista->at( i )->fechaDescarga = new QDateTime( QDateTime::currentDateTime() );

      out << "\nBAJANDO: " << titulo << "\n";
      return 1;
    }
  }

  return 0;
}

// FIN MANEJO DEL RSS

// INICIO MANEJO DEL TORRENT

/**
 * Prepara el GET del .torrent en cuestión
 * @param linkString Enlace del torrent
 */

void Rss_lite::parseLink( QString linkString, QString title = "" ) {
  QUrl url( linkString );
  QHttpRequestHeader header;
  QString urlTracker( QLatin1String( "http://" ) + url.host() );
  QString path;

  tracker *trk = trackers.value( urlTracker );

  if ( trk == NULL ) return;

  if ( linkString.contains( QLatin1String( "download" ) ) ) { // Si el link tiene download lo bajo tal cual
    path = linkString.mid( urlTracker.size() ); // Cojo el path de la url con los args
    // 		qDebug() << "Bajando:" << linkString.mid( urlTracker.size() );;
  } else { // Cojo la URL a partir de la cfg y del id
    path = trk->urlDownload + url.queryItemValue( trk->id ) ;
  }

  header.setRequest( QLatin1String( "GET" ), path );
  posts.insert( urlTracker + path, title );
  qDebug() << "Bajando:" << path;

  header.setValue( QLatin1String( "Host" ), url.host() );
  header.setValue( QLatin1String( "Cookie" ), trk->cookie );
  header.setValue( QLatin1String( "Referer" ), trk->urlTracker + trk->referer );

  httpTorrent.setHost( url.host() );
  connectionIdTorrent = httpTorrent.request( header );
}

/**
 * Mira el nombre del fichero y graba el torrent
 * @param &resp Encabezado HTTP del torrent
 */

void Rss_lite::readDataTorrent( const QHttpResponseHeader &resp ) {
  if ( resp.statusCode() != 200 )
    httpTorrent.abort();
  else {
    if ( ! ficheros.contains( httpTorrent.currentId() ) ) {
      /*
       * Miramos a ver si existe el fichero y si existe abortamos
       * y si no asociamos el nombre y creamos un array para los datos
       * y lo rellenamos
       */
      QString content = resp.value( QLatin1String( "content-disposition" ) );
      QString fichero = content.section( QLatin1Char( '\"' ), 1, 1 );

      QUrl url( QLatin1String( "http://" ) + httpTorrent.currentRequest().value( QLatin1String( "Host" ) ) + httpTorrent.currentRequest().path() );

      if ( !fichero.endsWith( QLatin1String( ".torrent" ) ) ) { // Si el header no me dice el nombre del fichero
        // METODO NUEVO (poner el titulo)
        qDebug() << "URL dwnld:" << url.toString();
        fichero = posts.take( url.toString() );
        // METODO VIEJO (poner el id)
        if ( fichero.isEmpty() ) {
          tracker *trk = trackers.value( QLatin1String( "http://" ) + url.host() );
          if ( trk == NULL ) return;
          fichero = url.queryItemValue( trk->id );
        }
        fichero += QLatin1String( ".torrent" );
        qDebug() << "Fichero: " << fichero;
      }

      fichero.replace( QLatin1Char( '/' ), QLatin1Char( '-' ) );
      fichero.replace( QLatin1Char( '\\' ), QLatin1Char( '-' ) );
      QFile file( values->Ruta() + QLatin1Char( '/' ) + fichero );

      if ( file.exists() ) {
        // 				httpTorrent.abort();
        // 				httpTorrent.close();
        return;
      }

      sites.insert( httpTorrent.currentId(), url.host() );
      ficheros.insert( httpTorrent.currentId(), fichero );
      datos.insert( httpTorrent.currentId(), new QByteArray() );
    }

    datos.value( httpTorrent.currentId() )->append( httpTorrent.readAll() );
  }
}

/**
 * Guarda el torrent al disco si es necesario ,graba el log y manda un mail.
 * @param id Id de la conexion
 * @param error Indica si ha habido error
 */

void Rss_lite::finishedTorrent( int id, bool error ) {
  // 	qWarning() << "ID:" << id << "ficheros" << ficheros.size() << "datos" << datos.size();
  int exito = -1;
  if ( error ) {
    qWarning( "Received error during HTTP fetch torrent." );
  } else {
    if ( ficheros.contains( id ) ) {
      QFile file( values->Ruta() + QLatin1Char( '/' ) + ficheros.value( id ) );
      qWarning() << "Escribiendo :" << file.fileName();

      if ( !file.exists() ) {
        if ( !file.open( QIODevice::WriteOnly ) ) {
          qWarning() << "No se pudo abrir para escritura !!!";
          delete datos.value( id );
          ficheros.remove( id );
          datos.remove( id );
          sites.remove( id );
          // 					qWarning() << "DES. ficheros" << ficheros.size() << "datos" << datos.size();
          return;
        }

        file.write( *datos.value( id ) );

        file.close();


        saveLog( ficheros.value( id ).left( ficheros.value( id ).size() - 8 ) );
        descargas.append( QLatin1String( "- " ) + ficheros.value( id ).left( ficheros.value( id ).size() - 8 ) + QLatin1String( " de " ) + sites.value( id ) + QLatin1Char( '\n' ) );
      }

      if ( !httpTorrent.hasPendingRequests() ) {
        if ( !descargas.isEmpty() ) { //FIXME
          exito = sendMail(
              QLatin1String( "RSSANI " ) + QHostInfo::localHostName() + QDateTime::currentDateTime().toString( QLatin1String( " dd/MM/yyyy hh:mm:ss" ) ),
              descargas );
          if ( exito == 0) descargas.clear();
        }
      }

      delete datos.value( id );

      ficheros.remove( id );
      datos.remove( id );
      sites.remove( id );
    }
  }

  // 	qWarning() << "DES. ficheros" << ficheros.size() << "datos" << datos.size();
}

// FIN MANEJO DEL TORRENT

/**
 * Graba el torrent al log
 * @param fichero Ruta absoluta del fichero
 * @return Indica si ha grabado bien el log
 */

int Rss_lite::saveLog( QString fichero ) {
  if ( !log->open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append ) )
    return 1;

  QTextStream out( log );

  out << "- " << QDateTime::currentDateTime().toString( QLatin1String( "dd/MM/yyyy hh:mm:ss" ) ) << " | " << fichero << endl;

  log->close();

  return 0;
}

void Rss_lite::iniciaTrackers() {
  // TODO: Poner esta configuracion en el CFG
  auth au;
  tracker *trk = new tracker;
  trk->urlTracker = QLatin1String( "http://www.hermeticos.org" );
  if ( hashAuths->contains( trk->urlTracker ) ) {
    au = hashAuths->value( trk->urlTracker );
    trk->cookie =  QLatin1String( "pass=" ) + au.pass + QLatin1String( "; uid=" ) + au.uid ;
    trk->referer = QLatin1String( "/browse.php" );
    trk->id = QLatin1String( "id" );
    trk->urlDownload = QLatin1String( "/download.php?" ) + trk->id + QLatin1Char( '=' ); // TODO: Poner id despues
    trk->urlRss = QLatin1String( "/rss.php" );
    trk->esRss = true;
    trackers.insert( trk->urlTracker, trk );
    listaTrackers.append( trk->urlTracker );
  }

  trk = new tracker();
  trk->urlTracker = QLatin1String( "http://btit.puntotorrent.com" );
  if ( hashAuths->contains( trk->urlTracker ) ) {
    au = hashAuths->value( trk->urlTracker );
    trk->cookie =  QLatin1String( "pass=" ) + au.pass + QLatin1String( "; uid=" ) + au.uid ;
    trk->referer = QLatin1String( "/torrents.php" );
    trk->id = QLatin1String( "id" );
    trk->urlDownload = QLatin1String( "/download.php?" ) + trk->id + QLatin1Char( '=' ); // TODO: Poner id despues
    trk->urlRss = QLatin1String( "/rss.php?pid=" ) + au.passkey;
    trk->esRss = true;
    trackers.insert( trk->urlTracker, trk );
    listaTrackers.append( trk->urlTracker );
  }

  trk = new tracker();
  trk->urlTracker = QLatin1String( "http://xbt.puntotorrent.com" );
  if ( hashAuths->contains( trk->urlTracker ) ) {
    au = hashAuths->value( trk->urlTracker );
    trk->cookie =  QLatin1String( "pass=" ) + au.pass + QLatin1String( "; uid=" ) + au.uid ;
    trk->referer = QLatin1String( "/torrents.php" );
    trk->id = QLatin1String( "id" );
    trk->urlDownload = QLatin1String( "/download.php?" ) + trk->id + QLatin1Char( '=' ); // TODO: Poner id despues
    trk->urlRss = QLatin1String( "/rss.php?pid=" ) + au.passkey;
    trk->esRss = true;
    trackers.insert( trk->urlTracker, trk );
    listaTrackers.append( trk->urlTracker );
  }

  for ( int i = 0; i < listaTrackers.size(); ++i ) {
    qDebug() << "+" << "Añadido tracker" << listaTrackers.value( i );
  }
}

int Rss_lite::sendMail( QString asunto, QString mensaje ) {
  MailSender mail( QLatin1String(""), values->FromMail(), QStringList( values->ToMail() ) ); //FIXME: Save this
  mail.setSubject( asunto ),
    mail.setBody( mensaje );
  mail.setContentType( MailSender::TextContent );
  mail.setLogin( QLatin1String(""), QLatin1String("")); //FIXME: Save this to settings
  mail.setSsl(true);
  mail.setPort( 587 );
  if ( mail.send() ) {
    return 0;
  } else {
    return -1;  // failed to send mail, display test.lastError(), test.lastCmd(), test.lastResponse()
  }
}
