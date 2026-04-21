#ifndef RSSANI_LITE_H
#define RSSANI_LITE_H

#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QSocketNotifier>
#include "rss_lite.h"
#include "myircsession.h"
#include <array>
#include <memory>
#ifdef __unix__
#include <signal.h>
#endif

/**
 * @brief Core application class. Manages settings, regexps, IRC integration, and signal wiring.
 *
 * All public methods are mutex-protected for thread-safe access from the XML-RPC thread.
 */
class rssani_lite : public QObject {
  Q_OBJECT

  public:
    /// @brief File descriptors for the POSIX signal self-pipe trick.
    static std::array<int, 2> sigFd;

    /**
     * @brief Constructs the main application object.
     * @param parent Parent QObject.
     */
    rssani_lite ( QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~rssani_lite();

    public slots:
      /**
       * @brief Adds a new regexp rule.
       * @param nombre Regexp pattern string.
       * @param fecha Expiration date for the rule.
       * @param mail If true, only send email without downloading.
       * @param tracker Tracker name this rule applies to.
       * @param dias Minimum days between downloads.
       */
      void anadirRegexp(std::string nombre, std::string fecha,bool mail,std::string tracker, int dias);

    /**
     * @brief Edits an existing regexp rule by matching its pattern.
     * @param regexpOrig Original regexp pattern to find.
     * @param regexpDest New regexp pattern to replace it with.
     * @return True on success.
     */
    bool editarRegexp ( std::string regexpOrig, std::string regexpDest );

    /**
     * @brief Edits an existing regexp rule by index.
     * @param regexpOrig Index of the regexp to edit.
     * @param regexpDest New regexp pattern to replace it with.
     * @return True on success.
     */
    bool editarRegexp ( int regexpOrig, std::string regexpDest );

    /**
     * @brief Toggles the active state of a regexp rule.
     * @param regexpOrig Index of the regexp to toggle.
     * @return True on success.
     */
    bool activarRegexp ( int regexpOrig );

    /**
     * @brief Moves a regexp rule to a different position in the list.
     * @param from Current position.
     * @param to Target position.
     */
    void moverRegexp ( int from, int to );

    /**
     * @brief Returns the application log contents.
     * @return List of log lines.
     */
    QStringList verLog();

    /**
     * @brief Returns the list of regexp rules.
     * @return Pointer to the regexp list.
     */
    QList<regexp*>* listaRegexp();

    /**
     * @brief Deletes a regexp rule by position.
     * @param pos Index of the rule to delete.
     */
    void borrarRegexp(int pos);

    /**
     * @brief Deletes a regexp rule by pattern string.
     * @param cad Regexp pattern to delete.
     */
    void borrarRegexp(std::string cad);

    /**
     * @brief Changes the RSS fetch timer interval.
     * @param tiempo Interval in minutes.
     */
    void cambiaTimer(int tiempo);

    /**
     * @brief Returns the current RSS fetch timer interval.
     * @return Interval in minutes.
     */
    int verTimer();

    /**
     * @brief Returns the timestamp of the last RSS fetch.
     * @return Date/time string of the last fetch.
     */
    std::string verUltimo();

    /**
     * @brief Saves the current configuration to disk.
     */
    void guardar();

    /**
     * @brief Initiates a graceful application shutdown.
     */
    void salir();

    /**
     * @brief Sets the XML-RPC authentication username.
     * @param theValue New username.
     */
    void setRpcUser ( std::string theValue );

    /**
     * @brief Returns the XML-RPC authentication username.
     * @return Username string.
     */
    QString getRpcUser();

    /**
     * @brief Sets the XML-RPC authentication password.
     * @param theValue New password.
     */
    void setRpcPass ( std::string theValue );

    /**
     * @brief Returns the XML-RPC authentication password.
     * @return Password string.
     */
    QString getRpcPass();

    /**
     * @brief Adds tracker authentication credentials.
     * @param tracker Tracker name.
     * @param uid User ID.
     * @param pass Password.
     * @param passkey Passkey.
     */
    void anadirAuth ( std::string tracker, std::string uid, std::string pass, std::string passkey);

    /**
     * @brief Removes tracker authentication credentials.
     * @param tracker Tracker name to remove.
     */
    void borrarAuth ( std::string tracker );

    /**
     * @brief Returns the list of tracker authentication entries.
     * @return Pointer to the auth list.
     */
    QList<auth>* listaAuths();

    /**
     * @brief Returns the configuration values object.
     * @return Pointer to the Values instance.
     */
    Values* getValues() const;

    /**
     * @brief Toggles debug mode.
     */
    void debugea();

  protected:
    /**
     * @brief Sets up signal/slot connections.
     */
    void prepareSignals();

    protected slots:
      /**
       * @brief Writes the current configuration to disk.
       */
      void writeSettings();
    private slots:
      /**
       * @brief Performs an immediate shutdown.
       */
      void salYa();
    void miraSubida( QString msg);
    void handleSigTerm();
  private:
    /**
     * @brief Reads configuration from disk.
     */
    void readSettings();
    Rss_lite *rss;
    mutable QMutex mutex;
    std::unique_ptr<QSettings> settings;
    std::unique_ptr<Values> values;

    int tiempo;
    std::unique_ptr<QList<regexp*>> lista;
    std::unique_ptr<QList<auth>> listAuths;
    std::unique_ptr<QHash<QString,auth>> hashAuths;
    QTimer timer;
    QTimer shutdown;
    std::unique_ptr<QFile> flog;
    QString rpcUser,rpcPass;
    MyIrcSession *session;
    datosIrc misdatos;
    std::unique_ptr<QSocketNotifier> snTerm;
signals:
    /**
     * @brief Emitted when a new upload is detected.
     * @param seccion Section/category of the upload.
     * @param titulo Title of the upload.
     * @param url Download URL.
     */
    void nuevaSubida ( QString seccion, QString titulo, QString url);

    /**
     * @brief Emitted when the RSS fetch timer fires.
     */
    void timeout();
};

#endif
