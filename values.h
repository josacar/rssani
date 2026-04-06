#ifndef VALUES_H
#define VALUES_H

#include <QString>

/**
 * @brief Tracker authentication credentials.
 */
struct auth {
  QString tracker;     ///< Tracker name/URL.
  QString uid;         ///< User ID.
  QString pass;        ///< Password.
  QString passkey;     ///< Passkey.
  QString referer;     ///< HTTP Referer header value.
  QString idField;     ///< Query parameter name for the torrent ID.
  QString urlDownload; ///< Base URL for torrent downloads.
  QString urlRss;      ///< RSS feed URL.
};

/**
 * @brief Application configuration values (paths, mail settings, debug flag).
 */
class Values {
  public:
    Values() = default;
    ~Values() = default;

    /// @brief Sets the torrent download directory path.
    void setRuta(const QString &theValue) { ruta = theValue; }
    /// @brief Returns the torrent download directory path.
    QString Ruta() const { return ruta; }

    /// @brief Sets the sender email address.
    void setFromMail(const QString &theValue) { fromMail = theValue; }
    /// @brief Returns the sender email address.
    QString FromMail() const { return fromMail; }

    /// @brief Sets the recipient email address.
    void setToMail(const QString &theValue) { toMail = theValue; }
    /// @brief Returns the recipient email address.
    QString ToMail() const { return toMail; }

    /// @brief Returns true if the download path is configured.
    bool filledValues() const { return !ruta.isEmpty(); }

    /// @brief Sets the date string of the last RSS fetch.
    void setFecha(const QString &theValue) { fecha = theValue; }
    /// @brief Returns the date string of the last RSS fetch.
    QString Fecha() const { return fecha; }

    /// @brief Enables or disables debug mode.
    void setDebug(bool theValue) { debug = theValue; }
    /// @brief Returns whether debug mode is enabled.
    bool Debug() const { return debug; }

    /// @brief Sets the SMTP server hostname.
    void setSmtpServer(const QString &theValue) { smtpServer = theValue; }
    /// @brief Returns the SMTP server hostname.
    QString SmtpServer() const { return smtpServer; }

    /// @brief Sets the SMTP login username.
    void setSmtpLogin(const QString &theValue) { smtpLogin = theValue; }
    /// @brief Returns the SMTP login username.
    QString SmtpLogin() const { return smtpLogin; }

    /// @brief Sets the SMTP login password.
    void setSmtpPass(const QString &theValue) { smtpPass = theValue; }
    /// @brief Returns the SMTP login password.
    QString SmtpPass() const { return smtpPass; }

    /// @brief Sets the SMTP server port.
    void setSmtpPort(int theValue) { smtpPort = theValue; }
    /// @brief Returns the SMTP server port.
    int SmtpPort() const { return smtpPort; }

  private:
    QString fromMail;
    QString toMail;
    QString ruta;
    QString rpcUser;
    QString rpcPass;
    QString fecha;
    bool debug = false;
    QString smtpServer;
    QString smtpLogin;
    QString smtpPass;
    int smtpPort = 587;
};

#endif
