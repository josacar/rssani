#include "rss_lite.h"
#include <QRegularExpression>
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
  connect( &httpRss, &QNetworkAccessManager::finished,
      this, &Rss_lite::readDataRSS );

  connect( &httpTorrent, &QNetworkAccessManager::finished,
      this, &Rss_lite::readDataTorrent );

  connect( this, &Rss_lite::linkCorrecto,
      this, &Rss_lite::parseLink );
}

// INICIO MANEJO DEL RSS

/**
 * Hace un get al RSS, y envia la señal de lectura a readDataRSS
 */

void Rss_lite::fetch() {
  descargas.clear();
  httpRss.disconnect();

  if ( !values->filledValues() ) {
    std::cerr << "Configuración errónea" << std::endl;
    exit( 1 );
  }

  tracker *trk = nullptr;

  for ( int i = 0; i < listaTrackers.size(); ++i ) {
    trk = trackers.value( listaTrackers.at( i ) );

    if ( trk == nullptr ) continue;

    url = new QUrl( trk->urlTracker );

    if ( xmls.contains( url->host() ) )
        xmls.value( url->host() )->clear(); //TODO: mirar

    QNetworkRequest request;
    request.setUrl(trk->urlRss);
    request.setRawHeader("Host", url->host().toUtf8() );
    request.setRawHeader("Cookie", trk->cookie.toUtf8() );
    request.setRawHeader("Referer", (trk->urlTracker + trk->referer).toUtf8() );

    qDebug() << "+ Me bajo" << url->host() << trk->urlRss;

    trk = nullptr;
    delete url;

    httpRss.get(request);
  }

  ultimoRss = QDateTime::currentDateTime();
}

/**
 * Parsea el XML conforme le va llegando
 * @param &resp Encabezado de respuesta HTTP
 */

void Rss_lite::readDataRSS(QNetworkReply *reply) {

  QString xml;

  if (reply->error() != QNetworkReply::NoError)
    return;

  xml = QString( reply->readAll() );

  QString host = reply->rawHeader("Host");

  if ( ! xmls.count( host ) ) {
      xmls.insert( host, new QXmlStreamReader() );
  }

  xmls.value( host )->addData( xml );
  parseXml( xmls.value( host ) );

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
  QRegularExpression rxlen( QLatin1String( "Category: (\\S+)" ) );
  QRegularExpression syl( QLatin1String( "\\s\\[S\\(\\d+\\)/L\\(\\d+\\)\\]" ) );
  QRegularExpression titulo( QLatin1String( "^\\[([^]]+)\\]\\s" ) );
  QTextStream out( stdout );

  QString value;

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

        QRegularExpressionMatch m = rxlen.match( description );
        if ( m.hasMatch() ) value = m.captured( 1 );

        QRegularExpressionMatch mt = titulo.match( titleString );
        if ( mt.hasMatch() ) value = mt.captured( 1 );

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
    }

    if ( xml->error() && xml->error() != QXmlStreamReader::PrematureEndOfDocumentError ) {
        qWarning() << "XML ERROR:" << xml->lineNumber() << ": " << xml->errorString() << " con id " << xml->error();
        titleString.clear();
        linkString.clear();
        pubDate.clear();
        description.clear();
        pila.pop();
        xml->clear();
    }
  }

  if ( xml->atEnd() ) {
      qDebug() << "Limpio el stream XML";
      xml->clear();
  }
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

    if ( lista->at( i )->activa && subida.contains( QRegularExpression( lista->at( i )->nombre, QRegularExpression::CaseInsensitiveOption ) ) ) {
      if ( lista->at( i )->tracker != QLatin1String( "" ) ) { // Si tiene tracker especifico miro a ver y si no drop
        urlRegexp = QUrl( lista->at( i )->tracker );
        if ( values->Debug() )
          qDebug() << "Tracker" << urlRegexp.host() << urlLink.host();
        if ( urlLink.host() != urlRegexp.host() ) continue;
      }

      if ( matches->open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append ) ) {  // Lo escribimos en el log
        QTextStream outFile( matches );
        outFile << subida << " --> " << lista->at( i )->nombre << Qt::endl;
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


      if ( lista->at( i )->fechaDescarga != nullptr ) {
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
  QUrlQuery query(url);
  QString urlTracker( QString( "http://" ) + url.host() );
  QString path;

  tracker *trk = trackers.value( urlTracker );

  if ( trk == nullptr )
      return;

  if ( linkString.contains( QLatin1String( "download" ) ) ) { // Si el link tiene download lo bajo tal cual
    path = linkString.mid( urlTracker.size() ); // Cojo el path de la url con los args
    qDebug() << "Bajando:" << linkString.mid( urlTracker.size() );;
  } else { // Cojo la URL a partir de la cfg y del id
    path = trk->urlDownload + query.queryItemValue( trk->id ) ;
  }

    QNetworkRequest request;
    request.setUrl(path);
    request.setRawHeader("Host", url.host().toUtf8() );
    request.setRawHeader("Cookie", trk->cookie.toUtf8() );
    request.setRawHeader("Referer", (trk->urlTracker + trk->referer).toUtf8() );

    httpTorrent.get(request);
    posts.insert( urlTracker + path, title );
    qDebug() << "Bajando:" << path;
}

/**
 * Mira el nombre del fichero y graba el torrent
 * @param &resp Encabezado HTTP del torrent
 */

void Rss_lite::readDataTorrent(QNetworkReply *reply) {

  QString downloadKey = reply->url().toString();

  if (reply->error() != QNetworkReply::NoError) {
    reply->abort();
  } else {
    if ( ! ficheros.contains(downloadKey) ) {
      /*
       * Miramos a ver si existe el fichero y si existe abortamos
       * y si no asociamos el nombre y creamos un array para los datos
       * y lo rellenamos
       */
      QString content = reply->rawHeader( QString( "content-disposition").toUtf8() );
      QString fichero = content.section( QChar( '\"' ), 1, 1 );

      QUrl replyUrl = reply->url();
      QUrl url( QString( "http://" ) + reply->rawHeader( QString("Host").toUtf8() ) + replyUrl.path() );

      if ( !fichero.endsWith( QString( ".torrent" ) ) ) { // Si el header no me dice el nombre del fichero
        // METODO NUEVO (poner el titulo)
        qDebug() << "URL dwnld:" << url.toString();
        fichero = posts.take( url.toString() );
        // METODO VIEJO (poner el id)
        if ( fichero.isEmpty() ) {
          tracker *trk = trackers.value( QString( "http://" ) + url.host() );
          if ( trk == nullptr ) return;
          fichero = QUrlQuery(url).queryItemValue( trk->id );
        }
        fichero += QLatin1String( ".torrent" );
        qDebug() << "Fichero: " << fichero;
      }

      fichero.replace( QLatin1Char( '/' ), QLatin1Char( '-' ) );
      fichero.replace( QLatin1Char( '\\' ), QLatin1Char( '-' ) );
      QFile file( values->Ruta() + QLatin1Char( '/' ) + fichero );

      if ( file.exists() ) {
        reply->abort();
        reply->close();
        return;
      }

      sites.insert( downloadKey, url.host() );
      ficheros.insert( downloadKey, fichero );
      datos.insert( downloadKey, new QByteArray() );
    }

    datos.value(downloadKey)->append( reply->readAll() );
  }
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

  out << "- " << QDateTime::currentDateTime().toString( QLatin1String( "dd/MM/yyyy hh:mm:ss" ) ) << " | " << fichero << Qt::endl;

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
  MailSender mail( values->SmtpServer(), values->FromMail(), QStringList( values->ToMail() ) );
  mail.setSubject( asunto ),
    mail.setBody( mensaje );
  mail.setContentType( MailSender::TextContent );
  mail.setLogin( values->SmtpLogin(), values->SmtpPass() );
  mail.setSsl(true);
  mail.setPort( values->SmtpPort() );
  if ( mail.send() ) {
    return 0;
  } else {
    return -1;  // failed to send mail, display test.lastError(), test.lastCmd(), test.lastResponse()
  }
}
