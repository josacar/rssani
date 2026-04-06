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

/**
 * @brief Regexp matching rule for torrent downloads.
 */
struct regexp {
  QString nombre;        ///< Regexp pattern string.
  QString vencimiento;   ///< Expiration date for the rule.
  bool mail;             ///< If true, only send email without downloading.
  QString tracker;       ///< Tracker this rule applies to.
  int diasDescarga;      ///< Minimum days between downloads.
  QDateTime fechaDescarga; ///< Timestamp of the last download for this rule.
  bool activa;           ///< Whether the rule is active.
};

/**
 * @brief Tracker configuration.
 */
struct tracker {
  QString urlTracker;   ///< Tracker base URL.
  QString referer;      ///< HTTP Referer header value.
  QString cookie;       ///< Authentication cookie.
  QString urlRss;       ///< RSS feed URL.
  QString urlDownload;  ///< Base URL for torrent downloads.
  QString id;           ///< Query parameter name for the torrent ID.
  bool esRss;           ///< Whether this tracker uses RSS (vs. IRC only).
};

/**
 * @brief RSS feed fetcher, XML parser, torrent downloader, and regexp matcher.
 */
class Rss_lite : public QObject {
  Q_OBJECT

  public:
    /**
     * @brief Constructs the RSS handler.
     * @param values Configuration values.
     * @param lista List of regexp matching rules.
     * @param log Log file handle.
     * @param auths Tracker authentication credentials.
     * @param parent Parent QObject.
     */
    Rss_lite ( Values* values, QList<regexp*>* lista, QFile* log, QHash<QString,auth> *auths, QObject* parent );

    /**
     * @brief Destructor.
     */
    ~Rss_lite();

    public slots:
      /**
       * @brief Returns the timestamp of the last RSS fetch.
       * @return Date/time of the last fetch.
       */
      QDateTime verUltimo();

    /**
     * @brief Checks a title from IRC against the regexp list.
     * @param seccion Section/category.
     * @param titulo Title of the upload.
     * @param link Download link.
     * @param fromIrc Whether the call originates from IRC (default true).
     */
    void miraTitulo(QString seccion, QString titulo, QString link, bool fromIrc = true);
  protected:
    /**
     * @brief Sets up signal/slot connections for RSS and torrent downloads.
     */
    void prepareSignals();

    public slots:
      /**
       * @brief Fetches the RSS feed and triggers XML parsing via readDataRSS.
       */
      void fetch();

    /**
     * @brief Parses incoming RSS XML data.
     * @param reply HTTP reply containing the RSS data.
     */
    void readDataRSS (QNetworkReply *reply);

    /**
     * @brief Reads the torrent file name from the response and saves it to disk.
     * @param reply HTTP reply containing the torrent data.
     */
    void readDataTorrent (QNetworkReply *reply );

    /**
     * @brief Prepares and issues the HTTP GET for a .torrent file.
     * @param linkString Torrent download URL.
     * @param title Title of the torrent.
     */
    void parseLink ( QString linkString, QString title);

    /**
     * @brief Appends a downloaded torrent filename to the log.
     * @param fichero Absolute path of the downloaded file.
     * @return 0 on success.
     */
    int saveLog ( QString fichero);
  private:
    /**
     * @brief Parses RSS XML and checks items against regexp rules.
     */
    void parseXml(QXmlStreamReader *xml);

    /**
     * @brief Checks a title against all regexp rules.
     * @param seccion Section/category.
     * @param titleString Title of the download.
     * @param linkString Download link.
     * @param fromIrc Whether the call originates from IRC.
     * @return Index of the matching regexp, or -1 if none.
     */
    int parseTitle ( QString seccion, QString titleString, QString linkString, bool fromIrc);

    /**
     * @brief Initializes tracker configurations from settings.
     */
    void iniciaTrackers ();

    /**
     * @brief Sends an email notification.
     * @param asunto Email subject.
     * @param mensaje Email body.
     * @return 0 on success, -1 on failure.
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
     * @brief Emitted when a title matches a regexp rule.
     * @param link Torrent download URL.
     * @param title Title of the matched torrent.
     */
    void linkCorrecto ( QString link, QString title );
};

#endif
