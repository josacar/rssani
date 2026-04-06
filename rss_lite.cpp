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
  matches = std::make_unique<QFile>( fi.canonicalPath() + QStringLiteral("/matches.log") );

  ultimoRss = QDateTime::currentDateTime();
  iniciaTrackers();
}

/**
 * Destructor por defecto
 */
Rss_lite::~Rss_lite() = default;

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


  for ( const auto &trkUrl : listaTrackers ) {
    auto *trk = trackers[trkUrl].get();

    if ( trk == nullptr ) continue;

    QUrl urlTracker( trk->urlTracker );

    if ( xmls.contains( urlTracker.host() ) )
        xmls[urlTracker.host()]->clear(); //TODO: mirar

    QNetworkRequest request;
    request.setUrl(trk->urlRss);
    request.setRawHeader("Host", urlTracker.host().toUtf8() );
    request.setRawHeader("Cookie", trk->cookie.toUtf8() );
    request.setRawHeader("Referer", (trk->urlTracker + trk->referer).toUtf8() );

    qDebug() << "+ Me bajo" << urlTracker.host() << trk->urlRss;

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
      xmls.insert( host, std::make_shared<QXmlStreamReader>() );
  }

  xmls[host]->addData( xml );
  parseXml( xmls[host].get() );

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
  QRegularExpression rxlen( QStringLiteral("Category: (\\S+)") );
  QRegularExpression syl( QStringLiteral("\\s\\[S\\(\\d+\\)/L\\(\\d+\\)\\]") );
  QRegularExpression titulo( QStringLiteral("^\\[([^]]+)\\]\\s") );
  QTextStream out( stdout );

  QString value;

  while ( !xml->atEnd() ) {
    xml->readNext();

    if ( xml->isStartElement() ) {
      if ( xml->name() == "item" ) {
        linkString = xml->attributes().value( QStringLiteral("rss:about") ).toString();
        pila.push( 0 );
      }
      if ( xml->name() == "enclosure" ) { // Con esto machaco
        linkString = xml->attributes().value( QStringLiteral("url") ).toString();
        // 				qDebug() << "Enclosure:" << linkString;
      }
      currentTag = xml->name().toString();
    } else if ( xml->isEndElement() ) {
      if ( xml->name() == "item" ) {
        QDateTime date = QDateTime::fromString( pubDate.section( QChar(' '), 1, 4 ), QStringLiteral("dd MMM yyyy hh:mm:ss") );
        dateS = date.toString( QStringLiteral("yyyyMMddhhmmss") );

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
        if ( currentTag == QStringLiteral("title") ) titleString += xml->text().toString();
        else if ( currentTag == QStringLiteral("link") ) linkString += xml->text().toString();
        else if ( currentTag == QStringLiteral("pubDate") ) pubDate += xml->text().toString();
        else if ( currentTag == QStringLiteral("description") ) description += xml->text().toString();
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
  if ( !seccion.isEmpty() ) subida = QChar('(') + seccion + QStringLiteral(") ") + titulo;

  if ( values->Debug() )
    out << "Analiz. : " << subida << "\n";//<< enlace;

  if ( ! fromIrc ) out << ".";
  if ( recientes.contains( titulo ) ) return 0;

  for ( int i = 0;i < lista->size();i++ ) {
    // Quitamos lo que haya vencido
    if ( !lista->at( i )->vencimiento.isEmpty() )
      if ( QDate::currentDate() >= QDate::fromString( lista->at( i )->vencimiento, QStringLiteral("dd-MM-yyyy") ) ) {
        out  << "Borrado caducado" << lista->at( i )->nombre << "\n";
        delete lista->at( i );
        lista->removeAt( i );
        i--;
        continue;
      }

    if ( lista->at( i )->activa && subida.contains( QRegularExpression( lista->at( i )->nombre, QRegularExpression::CaseInsensitiveOption ) ) ) {
      if ( !lista->at( i )->tracker.isEmpty() ) { // Si tiene tracker especifico miro a ver y si no drop
        urlRegexp = QUrl( lista->at( i )->tracker );
        if ( values->Debug() )
          qDebug() << "Tracker" << urlRegexp.host() << urlLink.host();
        if ( urlLink.host() != urlRegexp.host() ) continue;
      }

      if ( matches->open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append ) ) {  // Lo escribimos en el log
        QTextStream outFile( matches.get() );
        outFile << subida << " --> " << lista->at( i )->nombre << Qt::endl;
        matches->close();
      }

      // Miramos si solo mail
      if ( lista->at( i )->mail == 1 ) {
        out << "\nINFO: " << titulo << "\n";
        exito = sendMail(
            QStringLiteral("RSSINFO ") + QHostInfo::localHostName() + QDateTime::currentDateTime().toString( QStringLiteral(" dd/MM/yyyy hh:mm:ss") ),
            subida + QStringLiteral("\nLINK   : ") + enlace );

        if ( exito == 0 )
          return 2;// Si es igual pasando, que ya hemos avisado...
        else
          return 3;
      }

      // Vemos si han pasado los dias minimos entre descargas


      if ( !lista->at( i )->fechaDescarga.isNull() ) {
        if ( values->Debug() )
          qDebug() << "diasDescarga:" << lista->at( i )->fechaDescarga.daysTo( QDateTime::currentDateTime() ) << "Dias:" << lista->at( i )->diasDescarga;
        if ( lista->at( i )->fechaDescarga.daysTo( QDateTime::currentDateTime() ) < lista->at( i )->diasDescarga ) {
          continue;
        }
      }

      lista->at( i )->fechaDescarga = QDateTime::currentDateTime();

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
  QString urlTracker( url.scheme() + QStringLiteral("://") + url.host() );
  QString path;

  auto *trk = trackers[urlTracker].get();

  if ( trk == nullptr )
      return;

  if ( linkString.contains( QStringLiteral("download") ) ) { // Si el link tiene download lo bajo tal cual
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
      QUrl url( reply->url().scheme() + QStringLiteral("://") + reply->rawHeader( QString("Host").toUtf8() ) + replyUrl.path() );

      if ( !fichero.endsWith( QString( ".torrent" ) ) ) { // Si el header no me dice el nombre del fichero
        // METODO NUEVO (poner el titulo)
        qDebug() << "URL dwnld:" << url.toString();
        fichero = posts.take( url.toString() );
        // METODO VIEJO (poner el id)
        if ( fichero.isEmpty() ) {
          auto *trk = trackers[reply->url().scheme() + QStringLiteral("://") + url.host()].get();
          if ( trk == nullptr ) return;
          fichero = QUrlQuery(url).queryItemValue( trk->id );
        }
        fichero += QStringLiteral(".torrent");
        qDebug() << "Fichero: " << fichero;
      }

      fichero.replace( QChar('/'), QChar('-') );
      fichero.replace( QChar('\\'), QChar('-') );
      QFile file( values->Ruta() + QChar('/') + fichero );

      if ( file.exists() ) {
        reply->abort();
        reply->close();
        return;
      }

      sites.insert( downloadKey, url.host() );
      ficheros.insert( downloadKey, fichero );
      datos.insert( downloadKey, std::make_shared<QByteArray>() );
    }

    datos[downloadKey]->append( reply->readAll() );

    // Write torrent to disk and clean up
    QString fichero = ficheros.value( downloadKey );
    QString fullPath = values->Ruta() + QChar('/') + fichero;
    QFile file( fullPath );
    if ( file.open( QIODevice::WriteOnly ) ) {
      file.write( *datos[downloadKey] );
      file.close();
      qDebug() << "Grabado:" << fichero;
      saveLog( fichero );
      sendMail(
          QStringLiteral("RSSANI ") + QHostInfo::localHostName() + QDateTime::currentDateTime().toString( QStringLiteral(" dd/MM/yyyy hh:mm:ss") ),
          fichero );
    }
    ficheros.remove( downloadKey );
    datos.remove( downloadKey );
    sites.remove( downloadKey );
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

  out << "- " << QDateTime::currentDateTime().toString( QStringLiteral("dd/MM/yyyy hh:mm:ss") ) << " | " << fichero << Qt::endl;

  log->close();

  return 0;
}

void Rss_lite::iniciaTrackers() {
  auth au;
  for ( auto it = hashAuths->constBegin(); it != hashAuths->constEnd(); ++it ) {
    au = it.value();
    auto trk = std::make_shared<tracker>();
    trk->urlTracker = au.tracker;
    trk->cookie = QStringLiteral("pass=") + au.pass + QStringLiteral("; uid=") + au.uid;
    trk->referer = au.referer;
    trk->id = au.idField;
    trk->urlDownload = au.urlDownload;
    trk->urlRss = au.urlRss;
    if ( !au.passkey.isEmpty() && trk->urlRss.contains( QStringLiteral("pid=") ) )
      trk->urlRss += au.passkey;
    trk->esRss = true;
    QString url = trk->urlTracker;
    listaTrackers.append( url );
    qDebug() << "+" << "Añadido tracker" << url;
    trackers.insert( url, std::move(trk) );
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
