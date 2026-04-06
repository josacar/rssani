#ifndef VALUES_H
#define VALUES_H

#include <QString>

struct auth {
  QString tracker;
  QString uid;
  QString pass;
  QString passkey;
  QString referer;
  QString idField;
  QString urlDownload;
  QString urlRss;
};

class Values {
  public:
    Values() = default;
    ~Values() = default;

    void setRuta(const QString &theValue) { ruta = theValue; }
    QString Ruta() const { return ruta; }

    void setFromMail(const QString &theValue) { fromMail = theValue; }
    QString FromMail() const { return fromMail; }

    void setToMail(const QString &theValue) { toMail = theValue; }
    QString ToMail() const { return toMail; }

    bool filledValues() const { return !ruta.isEmpty(); }

    void setFecha(const QString &theValue) { fecha = theValue; }
    QString Fecha() const { return fecha; }

    void setDebug(bool theValue) { debug = theValue; }
    bool Debug() const { return debug; }

    void setSmtpServer(const QString &theValue) { smtpServer = theValue; }
    QString SmtpServer() const { return smtpServer; }

    void setSmtpLogin(const QString &theValue) { smtpLogin = theValue; }
    QString SmtpLogin() const { return smtpLogin; }

    void setSmtpPass(const QString &theValue) { smtpPass = theValue; }
    QString SmtpPass() const { return smtpPass; }

    void setSmtpPort(int theValue) { smtpPort = theValue; }
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
