#ifndef VALUES_H
#define VALUES_H
/**
 * Clase con los valores de configuración
 */
struct auth {
  QString tracker;
  QString uid;
  QString pass;
  QString passkey;
};

class Values {

  private:
    QString fromMail;
    QString toMail;
    QString ruta;
    QString rpcUser;
    QString rpcPass;
    QString fecha;
    bool debug;

  public:
    Values() {};

    ~Values() {};

    void setRuta ( QString theValue ) {
      ruta = theValue;
    }

    QString Ruta() {
      return ruta;
    }

    void setFromMail ( QString theValue )	{
      fromMail = theValue;
    }

    QString FromMail() {
      return fromMail;
    }

    void setToMail ( QString theValue )	{
      toMail = theValue;
    }

    QString ToMail() {
      return toMail;
    }

    bool filledValues() {
      return ( !ruta.isEmpty() );
      // 		return ( !uid.isEmpty() && !pass.isEmpty() && !ruta.isEmpty() );
    }

    void setFecha ( QString theValue ) {
      fecha = theValue;
    }


    QString Fecha() {
      return fecha;
    }

    void setDebug( bool theValue ) {
      debug = theValue;
    }

    bool Debug() const {
      return debug;
    }
};

#endif
