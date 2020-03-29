#include "xmlrpc.h"

#ifdef __WIN32__
const unsigned num_threads = 2;
#else
const unsigned num_threads = 2;
#endif

ulxr::HttpServer *httpServer = 0;

Metodos::Metodos ( rssani_lite *rss ) {
  this->rss = rss;
}

ulxr::MethodResponse Metodos::verUltimo ( const ulxr::MethodCall &/*calldata*/ ){
  ulxr::MethodResponse resp;
  ulxr::RpcString fecha = rss->verUltimo();

  resp.setResult( fecha );

  return resp;
}

ulxr::MethodResponse Metodos::verTimer ( const ulxr::MethodCall &/*calldata*/ ){
  ulxr::MethodResponse resp;
  ulxr::Integer timer = rss->verTimer();

  resp.setResult( timer );

  return resp;
}

ulxr::MethodResponse Metodos::verLog ( const ulxr::MethodCall &calldata ) {
  ulxr::MethodResponse resp;
  ulxr::Array valores;
  int ini,fin;

  ulxr::Integer iniaux = calldata.getParam ( 0 );
  ulxr::Integer finaux = calldata.getParam ( 1 );

  QStringList * log = rss->verLog();

  if ( finaux.getInteger() == 0 )
    fin = log->count();
  else
    fin = finaux.getInteger();

  ini = iniaux.getInteger();

  for ( int i = log->count()- ini -1 ;i >= std::max( log->count() - fin,0) ; i-- ) {
    valores << ulxr::RpcString(  (const char*)log->at ( i ).toUtf8() ); // Asi por los acentos
  }

  resp.setResult ( valores );

  return resp;
}

ulxr::MethodResponse Metodos::listaExpresiones ( const ulxr::MethodCall &/*calldata*/ ) {
  ulxr::MethodResponse resp;
  ulxr::Array valores;
  ulxr::Struct valor;
  QList<regexp*> * log = rss->listaRegexp();

  for ( int i = 0;i < log->count(); i++ ) {
    valor.addMember( ULXR_PCHAR("nombre"), ulxr::RpcString( log->at ( i )->nombre.toStdString() ) );
    valor.addMember( ULXR_PCHAR("vencimiento"), ulxr::RpcString( log->at ( i )->vencimiento.toStdString() ) );
    valor.addMember( ULXR_PCHAR("mail"), ulxr::Boolean( log->at ( i )->mail ) );
    valor.addMember( ULXR_PCHAR("tracker"), ulxr::RpcString( log->at ( i )->tracker.toStdString() ) );
    valor.addMember( ULXR_PCHAR("dias"), ulxr::Integer( log->at ( i )->diasDescarga ) );
    valor.addMember( ULXR_PCHAR("activa"), ulxr::Boolean( log->at ( i )->activa ) );
    valores << ulxr::Struct ( valor ); 
    valor.clear();
  }

  resp.setResult ( valores );

  return resp;
}

ulxr::MethodResponse Metodos::editarRegexp ( const ulxr::MethodCall &calldata ) {
  ulxr::MethodResponse resp;
  ulxr::RpcString orig = calldata.getParam ( 0 );
  ulxr::RpcString dest = calldata.getParam ( 1 );

  bool error = rss->editarRegexp( orig.getString(), dest.getString() );
  resp.setResult ( ulxr::Boolean ( error ) );

  return resp;
}

ulxr::MethodResponse Metodos::editarRegexpI ( const ulxr::MethodCall &calldata ) {
  ulxr::MethodResponse resp;
  ulxr::Integer orig = calldata.getParam ( 0 );
  ulxr::RpcString dest = calldata.getParam ( 1 );

  bool error = rss->editarRegexp( orig.getInteger(), dest.getString() );
  resp.setResult ( ulxr::Boolean ( error ) );

  return resp;
}

ulxr::MethodResponse Metodos::activarRegexp ( const ulxr::MethodCall &calldata ) {
  ulxr::MethodResponse resp;
  ulxr::Integer orig = calldata.getParam ( 0 );

  bool error = rss->activarRegexp( orig.getInteger());
  resp.setResult ( ulxr::Boolean ( error ) );

  return resp;
}

ulxr::MethodResponse Metodos::anadirRegexp ( const ulxr::MethodCall &calldata ) {
  ulxr::MethodResponse resp;
  ulxr::RpcString S = calldata.getParam ( 0 ); // Regexp
  ulxr::RpcString V = calldata.getParam ( 1 ); // Vencimiento
  ulxr::Boolean M = calldata.getParam ( 2 ); // Solo mail
  ulxr::RpcString T = calldata.getParam ( 3 ); // Tracker
  ulxr::Integer D =  calldata.getParam ( 4 ); // Dias

  rss->anadirRegexp ( S.getString(), V.getString(), M.getBoolean(), T.getString(), D.getInteger() );
  resp.setResult ( ulxr::Boolean ( true ) );

  return resp;
}

ulxr::MethodResponse Metodos::moverRegexp ( const ulxr::MethodCall &calldata ) {
  ulxr::MethodResponse resp;
  ulxr::Integer from = calldata.getParam ( 0 );
  ulxr::Integer to = calldata.getParam ( 1 );

  rss->moverRegexp ( from.getInteger(), to.getInteger());
  resp.setResult ( ulxr::Boolean ( true ) );

  return resp;
}

ulxr::MethodResponse Metodos::borrarRegexpI ( const ulxr::MethodCall &calldata ) {
  ulxr::MethodResponse resp;
  ulxr::Integer I = calldata.getParam ( 0 );

  rss->borrarRegexp ( I.getInteger() );
  resp.setResult ( ulxr::Boolean ( true ) );

  return resp;
}

ulxr::MethodResponse Metodos::borrarRegexpS ( const ulxr::MethodCall &calldata ) {
  ulxr::MethodResponse resp;
  ulxr::RpcString S = calldata.getParam ( 0 );

  rss->borrarRegexp ( S.getString() );
  resp.setResult ( ulxr::Boolean ( true ) );

  return resp;
}

ulxr::MethodResponse Metodos::anadirAuth ( const ulxr::MethodCall &calldata ) {
  ulxr::MethodResponse resp;
  ulxr::RpcString T = calldata.getParam ( 0 );
  ulxr::RpcString U = calldata.getParam ( 1 );
  ulxr::RpcString P = calldata.getParam ( 2 );
  ulxr::RpcString PK = calldata.getParam ( 3 );

  rss->anadirAuth( T.getString(), U.getString(), P.getString(), PK.getString() );
  resp.setResult ( ulxr::Boolean ( true ) );

  return resp;
}

ulxr::MethodResponse Metodos::borrarAuth ( const ulxr::MethodCall &calldata ) {
  ulxr::MethodResponse resp;
  ulxr::RpcString T = calldata.getParam ( 0 );

  rss->borrarAuth( T.getString() );
  resp.setResult ( ulxr::Boolean ( true ) );

  return resp;
}

ulxr::MethodResponse Metodos::listaAuths ( const ulxr::MethodCall &/*calldata*/ ) {
  ulxr::MethodResponse resp;
  ulxr::Array valores;
  ulxr::Struct valor;
  QList<auth> * auths = rss->listaAuths();

  for ( int i = 0;i < auths->count(); i++ ) {
    valor.addMember( ULXR_PCHAR("tracker"), ulxr::RpcString( auths->at ( i ).tracker.toStdString() ) );
    valor.addMember( ULXR_PCHAR("uid"), ulxr::RpcString( auths->at ( i ).uid.toStdString() ) );
    valor.addMember( ULXR_PCHAR("pass"), ulxr::RpcString( auths->at ( i ).pass.toStdString() ) );
    valor.addMember( ULXR_PCHAR("passkey"), ulxr::RpcString( auths->at ( i ).passkey.toStdString() ) );

    valores << ulxr::Struct ( valor ); 
    valor.clear();
  }

  resp.setResult ( valores );

  return resp;
}

ulxr::MethodResponse Metodos::ponerCredenciales ( const ulxr::MethodCall &calldata ) {
  ulxr::MethodResponse resp;
  ulxr::RpcString U = calldata.getParam ( 0 );
  ulxr::RpcString P = calldata.getParam ( 1 );

  rss->setRpcUser( U.getString() );
  rss->setRpcPass( P.getString() );

  qDebug() << "USER:" <<rss->getRpcUser() << "PASS:" << rss->getRpcPass();

  resp.setResult ( ulxr::Boolean ( true ) );

  return resp;
}

ulxr::MethodResponse Metodos::verOpciones ( const ulxr::MethodCall &/*calldata*/ ) {
  ulxr::MethodResponse resp;
  ulxr::Struct valor;
  Values * opciones = rss->getValues();

  valor.addMember( ULXR_PCHAR("fromMail"), ulxr::RpcString( opciones->FromMail().toStdString() ) );
  valor.addMember( ULXR_PCHAR("toMail"), ulxr::RpcString( opciones->ToMail().toStdString() ) );
  valor.addMember( ULXR_PCHAR("path"), ulxr::RpcString( opciones->Ruta().toStdString() ) );

  resp.setResult ( valor );

  return resp;
}

ulxr::MethodResponse Metodos::ponerOpciones ( const ulxr::MethodCall &calldata ) {
  ulxr::MethodResponse resp;
  ulxr::RpcString F = calldata.getParam ( 0 ); // fromMail
  ulxr::RpcString T = calldata.getParam ( 1 ); // toMail
  ulxr::RpcString P = calldata.getParam ( 2 ); // Path

  Values * opciones = rss->getValues();
  opciones->setFromMail( QString::fromStdString( F.getString() ) );
  opciones->setToMail( QString::fromStdString( T.getString() ) );
  opciones->setRuta( QString::fromStdString( P.getString() ) );

  resp.setResult ( ulxr::Boolean ( true ) );

  return resp;
}

ulxr::MethodResponse Metodos::cambiaTimer ( const ulxr::MethodCall &calldata ) {
  ulxr::MethodResponse resp;
  ulxr::Integer I = calldata.getParam ( 0 );

  rss->cambiaTimer ( I.getInteger() );
  resp.setResult ( ulxr::Boolean ( true ) );

  return resp;
}

ulxr::MethodResponse Metodos::guardar ( const ulxr::MethodCall &/*calldata*/ ) {
  ulxr::MethodResponse resp;

  rss->guardar ( );
  resp.setResult ( ulxr::Boolean ( true ) );

  return resp;
}

ulxr::MethodResponse Metodos::numThreads ( const ulxr::MethodCall &/*calldata*/ ) {
  ulxr::MethodResponse resp;
  resp.setResult ( ulxr::Integer ( httpServer->numThreads() ) );
  return resp;
}

ulxr::MethodResponse Metodos::shutdown ( const ulxr::MethodCall &/*calldata*/ ) {
  ULXR_COUT << ULXR_PCHAR ( "Metodos ha recibido la señal de apagado\n" );
  ulxr::MethodResponse resp;
  resp.setResult ( ulxr::Boolean ( true ) );
  ULXR_COUT << ULXR_PCHAR ( "Terminando..\n" );

  // 	httpServer->shutdownAllThreads();
  rss->salir();
  ULXR_COUT << ULXR_PCHAR ( "Retornando..\n" );
  return resp;
}


rssxmlrpc::	rssxmlrpc ( rssani_lite *rss ) : QThread () {
  this->rss = rss;
}

void rssxmlrpc::run() {
  // 	ulxr::CppString host = ULXR_PCHAR ( "localhost" );
  long host = INADDR_ANY;
  //in_addr host = inaddr_any;
  unsigned port = 8080;


  ulxr::TcpIpConnection conn ( true, host, port );
  ulxr::HttpProtocol prot ( &conn );
  ulxr::HttpServer http_server ( &prot, num_threads );
  ulxr::Dispatcher server;
  http_server.setRpcDispatcher ( &server );

  httpServer = &http_server;

  Metodos worker ( rss );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::verUltimo ),
      ulxr::Signature ( ulxr::RpcString() ),
      ULXR_PCHAR ( "rssani.verUltimo" ),
      ulxr::Signature(),
      ULXR_PCHAR ( "Devuelve el instante del ultimo get" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::verTimer ),
      ulxr::Signature ( ulxr::Integer() ),
      ULXR_PCHAR ( "rssani.verTimer" ),
      ulxr::Signature(),
      ULXR_PCHAR ( "Devuelve el timer restante en ms" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::shutdown ),
      ulxr::Signature ( ulxr::Boolean() ),
      ULXR_PCHAR ( "rssani.shutdown" ),
      ulxr::Signature(),
      ULXR_PCHAR ( "Cierra RSSANI" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::numThreads ),
      ulxr::Signature ( ulxr::Integer() ),
      ULXR_PCHAR ( "rssani.numHilos" ),
      ulxr::Signature(),
      ULXR_PCHAR ( "Devuelve el numero de hilos en el inicio" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::verLog ),
      ulxr::Signature ( ulxr::Array() ),
      ULXR_PCHAR ( "rssani.verLog" ),
      ulxr::Signature () <<  ulxr::Integer() << ulxr::Integer(),
      ULXR_PCHAR ( "Devuelve el log" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::listaExpresiones ),
      ulxr::Signature ( ulxr::Array() ),
      ULXR_PCHAR ( "rssani.listaExpresiones" ),
      ulxr::Signature(),
      ULXR_PCHAR ( "Devuelve las expresiones regulares" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::editarRegexp ),
      ulxr::Signature ( ulxr::Boolean() ),
      ULXR_PCHAR ( "rssani.editarRegexp" ),
      ulxr::Signature () <<  ulxr::RpcString() << ulxr::RpcString(),
      ULXR_PCHAR ( "Edita una regexp de la lista" ) );
  server.addMethod ( ulxr::make_method ( worker, &Metodos::editarRegexpI ),
      ulxr::Signature ( ulxr::Boolean() ),
      ULXR_PCHAR ( "rssani.editarRegexpI" ),
      ulxr::Signature () <<  ulxr::Integer() << ulxr::RpcString(),
      ULXR_PCHAR ( "Edita una regexp de la lista" ) );
  server.addMethod ( ulxr::make_method ( worker, &Metodos::activarRegexp ),
      ulxr::Signature ( ulxr::Boolean() ),
      ULXR_PCHAR ( "rssani.activarRegexp" ),
      ulxr::Signature () <<  ulxr::Integer(),
      ULXR_PCHAR ( "(Des)Activa una regexp de la lista" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::moverRegexp ),
      ulxr::Signature ( ulxr::Boolean() ),
      ULXR_PCHAR ( "rssani.moverRegexp" ),
      ulxr::Signature () <<  ulxr::Integer() << ulxr::Integer(),
      ULXR_PCHAR ( "Mueve una regexp de la lista" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::anadirRegexp ),
      ulxr::Signature ( ulxr::Boolean() ),
      ULXR_PCHAR ( "rssani.anadirRegexp" ),
      ulxr::Signature() << ulxr::RpcString() << ulxr::RpcString() << ulxr::Boolean() << ulxr::RpcString() << ulxr::Integer(),
      ULXR_PCHAR ( "Añade una regexp a la lista" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::borrarRegexpI ),
      ulxr::Signature ( ulxr::Boolean() ),
      ULXR_PCHAR ( "rssani.borrarRegexpI" ),
      ulxr::Signature ( ulxr::Integer() ),
      ULXR_PCHAR ( "Borra la expresion 'i' de la lista" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::borrarRegexpS ),
      ulxr::Signature ( ulxr::Boolean() ),
      ULXR_PCHAR ( "rssani.borrarRegexpS" ),
      ulxr::Signature ( ulxr::RpcString() ),
      ULXR_PCHAR ( "Borra la expresion de la lista" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::anadirAuth ),
      ulxr::Signature ( ulxr::Boolean() ),
      ULXR_PCHAR ( "rssani.anadirAuth" ),
      ulxr::Signature() << ulxr::RpcString() << ulxr::RpcString() << ulxr::RpcString() << ulxr::RpcString(),
      ULXR_PCHAR ( "Añade un tracker con su auth" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::borrarAuth ),
      ulxr::Signature ( ulxr::Boolean() ),
      ULXR_PCHAR ( "rssani.borrarAuth" ),
      ulxr::Signature ( ulxr::RpcString() ),
      ULXR_PCHAR ( "Borra el auth del tracker" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::ponerOpciones ),
      ulxr::Signature ( ulxr::Boolean() ),
      ULXR_PCHAR ( "rssani.ponerOpciones" ),
      ulxr::Signature() << ulxr::RpcString() << ulxr::RpcString() << ulxr::RpcString(),
      ULXR_PCHAR ( "Pone las opciones generales" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::verOpciones ),
      ulxr::Signature ( ulxr::Struct() ),
      ULXR_PCHAR ( "rssani.verOpciones" ),
      ulxr::Signature ( ),
      ULXR_PCHAR ( "Muestra las opciones generales" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::listaAuths ),
      ulxr::Signature ( ulxr::Array() ),
      ULXR_PCHAR ( "rssani.listaAuths" ),
      ulxr::Signature ( ),
      ULXR_PCHAR ( "Lista los auth de los trackers" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::ponerCredenciales ),
      ulxr::Signature ( ulxr::Boolean() ),
      ULXR_PCHAR ( "rssani.ponerCredenciales" ),
      ulxr::Signature ( ) << ulxr::RpcString() << ulxr::RpcString(),
      ULXR_PCHAR ( "Establece los credenciales RPC para rssani" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::cambiaTimer ),
      ulxr::Signature ( ulxr::Boolean() ),
      ULXR_PCHAR ( "rssani.cambiaTimer" ),
      ulxr::Signature ( ulxr::Integer() ),
      ULXR_PCHAR ( "Cambia el timer de descarga del RSS" ) );

  server.addMethod ( ulxr::make_method ( worker, &Metodos::guardar ),
      ulxr::Signature ( ulxr::Boolean() ),
      ULXR_PCHAR ( "rssani.guardar" ),
      ulxr::Signature ( ),
      ULXR_PCHAR ( "Guarda la configuración" ) );

  http_server.addRealm ( ULXR_PCHAR("/RPC2"), ULXR_PCHAR("rpc2-resource") );

  http_server.addAuthentication ( ULXR_GET_STRING(rss->getRpcUser().toStdString()),
      ULXR_GET_STRING(rss->getRpcPass().toStdString()),
      ULXR_PCHAR("rpc2-resource") );

  ulxr::CppString root_dir = ULXR_PCHAR("/usr/local/ulxmlrpcpp/public_html");

  http_server.setHttpRoot ( root_dir );

  http_server.runPicoHttpd();

  http_server.waitAsync ( false, true );

}
