#ifndef RSSANI_LITE_H
#define RSSANI_LITE_H

#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include "rss_lite.h"
#include "myircsession.h"
#ifdef __unix__
#include <sys/signal.h>
#endif

/**
 * Clase que maneja la aplicación
 */
class rssani_lite : public QObject {
  Q_OBJECT

  public:
    /**
     * Construye la clase principal de la aplicación
     * @param parent Clase padre
     */
    rssani_lite ( QObject* parent = 0);
    /**
     * Destructor por defecto
     */
    ~rssani_lite();

    public slots:
      /**
       * Añade una regexp
       * @param nombre Nombre de la regexp
       * @param fecha Fecha de vencimiento de la regexp
       * @param mail Solo mail son descargar
       * @param tracker Tracker
       * @param dias Dias entre descargas
       */
      void anadirRegexp(std::wstring nombre, std::wstring fecha,bool mail,std::wstring tracker, int dias);

    /**
     * Edita una regexp
     * @param regexpOrig Regexp a cambiar
     * @param regexpDest Regexp destino
     * @return Devuelve 0 si tiene exito
     */
    bool editarRegexp ( std::wstring regexpOrig, std::wstring regexpDest );
    /**
     * Edita una regexp
     * @param regexpOrig Regexp a cambiar
     * @param regexpDest Regexp destino
     * @return Devuelve 0 si tiene exito
     */
    bool editarRegexp ( int regexpOrig, std::wstring regexpDest );
    /**
     * Edita una regexp
     * @param regexpOrig Regexp a cambiar
     * @param regexpDest Regexp destino
     * @return Devuelve 0 si tiene exito
     */
    bool activarRegexp ( int regexpOrig );

    /**
     * Mueve una regexp de sitio en la lista
     * @param from Posicion inicial de la regexp
     * @param to Posicion final de la regexp
     */
    void moverRegexp ( int from, int to );

    /**
     * Devuelve el log en una lista
     * @return La lista con el log
     */
    QStringList* verLog();

    /**
     * Devuelve la lista de regexp con sus atributos
     * @return La lista de regexp
     */
    QList<regexp*>* listaRegexp();

    /**
     * Borra la regexp de una posicion
     * @param pos La posicion a borrar
     */
    void borrarRegexp(int pos);

    /**
     * Borra la regexp
     * @param cad  La regexp a borrar
     */
    void borrarRegexp(std::wstring cad);

    /**
     * Cambia el timer del get del rss
     * @param tiempo El valor en minutos del timer
     */
    void cambiaTimer(int tiempo);

    /**
     * Muestra el valor del timer
     * @return El valor del timer
     */
    int verTimer();

    /**
     * Muestra la fecha del ultimo get
     * @return El string con la fecha
     */
    std::wstring verUltimo();

    /**
     * Guarda la configuración
     */
    void guardar();

    /**
     * Sale de la aplicación
     */
    void salir();

    /**
     * Cambia el usuario del xml-rpc
     * @param theValue El nuevo nombre del usuario
     */
    void setRpcUser ( std::wstring theValue );

    /**
     * Devuelve el usuario del xml-rpc
     * @return El nombre del usuario
     */
    QString getRpcUser();

    /**
     * Cambia la contraseña del xml-rpc
     * @param theValue La nueva contraseña del usuario
     */
    void setRpcPass ( std::wstring theValue );

    /**
     * Devuelve la contraseña del xml-rpc
     * @return La contraseña del usuario
     */
    QString getRpcPass();

    /**
     * Añade las credenciales del tracker
     * @param tracker 
     * @param uid 
     * @param pass 
     * @param passkey 
     */
    void anadirAuth ( std::wstring tracker, std::wstring uid, std::wstring pass, std::wstring passkey);

    /**
     * Borra las credenciales del tracker
     * @param tracker 
     */
    void borrarAuth ( std::wstring tracker );
    /**
     * Devuelve los auth de los trackers
     * @return Lista de auths de los trackers
     */
    QList<auth>* listaAuths();

    /**
     * 
     * @param theValue Valores de configuracion
     */
    void setValues( Values* theValue );
    /**
     * 
     * @return Valores de configuracion
     */
    Values* getValues() const;
    void debugea();	
  protected:
    /**
     * Prepara las señales y los slots
     */
    void prepareSignals();

    protected slots:
      /**
       * Escribe la configuración a disco
       */
      void writeSettings();
    private slots:
      /**
       * Sale inmediatamente
       */
      void salYa();
    void miraSubida( QString msg);
  private:
    /**
     * Lee la configuración del disco
     */
    void readSettings();
    Rss_lite *rss;
    QSettings *settings;
    Values *values;

    int tiempo;
    QList<regexp*> *lista;
    QList<auth> *listAuths;
    QHash<QString,auth> *hashAuths;
    QTimer timer;
    QTimer shutdown;
    QFile *flog;
    QString rpcUser,rpcPass;
    MyIrcSession *session;
    datosIrc misdatos;
signals:
    void nuevaSubida ( QString seccion, QString titulo, QString url);
    void timeout();
};

#endif
