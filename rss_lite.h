#ifndef RSS_LITE_H
#define RSS_LITE_H

#include <QtCore/QFile>
#include <QtCore/QDateTime>
#include <QtCore/QStack>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QHostInfo>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QDateTime>

#include <iostream>
#include <cstdlib>
#include <memory>
#include "mailsender.h"
#include "values.h"

struct regexp {
  QString nombre;
  QString vencimiento;
  bool mail;
  QString tracker;
  int diasDescarga;
  QDateTime fechaDescarga;
  bool activa;
};

struct tracker {
  QString urlTracker;
  QString referer;
  QString cookie;
  QString urlRss;
  QString urlDownload;
  QString id;
  bool esRss;
};

/**
 * Clase que maneja los RSS
 */
class Rss_lite : public QObject {
  Q_OBJECT

  public:
    /**
     * Constructor de la clase RSS
     * @param values Lista de opciones guardadas
     * @param lista Lista de regexps
     * @param log Ruta absoluta del fichero log
     * @param auths Tracker con sus auths
     * @param parent Puntero a la clase padre
     */
    Rss_lite ( Values* values, QList<regexp*>* lista, QFile* log, QHash<QString,auth> *auths, QObject* parent );

    /**
     * Desructor por defecto
     */
    ~Rss_lite();

    public slots:
      /**
       * 
       * @return 
       */
      QDateTime verUltimo();
    /**
     * 
     * @param seccion 
     * @param titulo 
     * @param link 
     * @param fromIrc
     */
    void miraTitulo(QString seccion, QString titulo, QString link, bool fromIrc = true);
  protected:
    /**
     * Prepara las señales de las descargas RSS y Torrent. También los link correctos.
     * @see Rss()
     * @see ~Rss()
     */
    void prepareSignals();

    public slots:
      /**
       * Hace un get al RSS, y envia la señal de lectura a readDataRSS
       */
      void fetch();

    /**
     * Parsea el XML conforme le va llegando
     * @param resp Encabezado de respuesta HTTP
     */
    void readDataRSS (QNetworkReply *reply);

    /**
     * Mira el nombre del fichero y graba el torrent
     * @param resp Encabezado HTTP del torrent
     */
    void readDataTorrent (QNetworkReply *reply );

    /**
     * Prepara el GET del .torrent en cuestión
     * @param linkString Enlace del torrent
     */
    void parseLink ( QString linkString, QString title);

    /**
     * Graba el torrent al log
     * @param fichero Ruta absoluta del fichero
     * @return Indica si ha grabado bien el log
     */
    int saveLog ( QString fichero);
  private:
    /**
     * Parsea el XML y ve si algo nos interesa
     */
    void parseXml(QXmlStreamReader *xml);

    /**
     * Mira a ver si coincide con alguna regexp y devuelve true
     * @param seccion Seccion de la descarga
     * @param titleString Nombre de la descarga
     * @param linkString Link de la descarga
     * @param fromIrc Indica si la llamada proviene del IRC
     * @return Si hace match con alguna regexp de la lista
     */
    int parseTitle ( QString seccion, QString titleString, QString linkString, bool fromIrc);

    /**
     * Inicializa los trackers con sus variables
     */
    void iniciaTrackers ();

    /**
     * Envia un mail con el asunto y el mensaje
     * @param asunto El asunto del mail
     * @param mensaje El mensaje del mail
     * @return -1 si falla, 0 si va bien
     */
    int sendMail( QString asunto, QString mensaje );

    QHash<QString, std::shared_ptr<QXmlStreamReader>> xmls;

    QHash<QString,QString> ficheros; /**< Hash con el id y el nombre del torrent */
    QHash<QString, std::shared_ptr<QByteArray>> datos; /**< hash con el id y los datos del torrent */
    QHash<QString,QString> sites; /**< hash con el id y los site del torrent */
    QHash<QString,QString> posts;
    QList<regexp*> *lista;
    QString fecha;
    QString descargas;
    Values *values;
    QStack<int> pila;
    QString currentTag;
    QString linkString; /**< Link al torrent */
    QString titleString; /**< Titulo del elemento */
    QString pubDate; /**< Fecha del ultimo elemento revisado */
    QString description;
    QString locale;
    QTimer timer; /**< Timer que actua para la descarga */
    QDateTime ultimoRss;
    QStringList recientes;

    QNetworkAccessManager httpRss,httpTorrent;
    QFile *log; std::unique_ptr<QFile> matches;
    QHash<QString, std::shared_ptr<tracker>> trackers; /**< hash con tracker y su configuracion */
    QHash<QString,auth> *hashAuths;
    QStringList listaTrackers;
signals:
    /**
     * Señal que se emite cuando el titulo hace match con una regexp
     * @param link Enlace del torrent
     */
    void linkCorrecto ( QString link, QString title );
};

#endif
