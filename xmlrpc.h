#ifndef XMLRPC_H
#define XMLRPC_H

#ifndef INADDR_ANY
  #define INADDR_ANY ((unsigned long int) 0x00000000)
#endif

#include <ulxmlrpcpp/ulxmlrpcpp.h>

#include <cassert>
#include <iostream>
#include "rssani_lite.h"
#include <ulxmlrpcpp/ulxr_value.h>
#include <ulxmlrpcpp/ulxr_response.h>
#include <ulxmlrpcpp/ulxr_call.h>
#include <ulxmlrpcpp/ulxr_http_protocol.h>
#include <ulxmlrpcpp/ulxr_http_server.h>
#include <ulxmlrpcpp/ulxr_dispatcher.h>
#include <ulxmlrpcpp/ulxr_tcpip_connection.h>
#include <ulxmlrpcpp/ulxr_except.h>
#include <ulxmlrpcpp/ulxr_signature.h>

#ifdef ULXR_MULTITHREADED

#ifdef __unix__
#include <pthread.h>
#endif

#endif
/**
 * Clase con los metodos XML-RPC
 */
class Metodos {
  rssani_lite *rss;

  public:
  /**
   * 
   * @param rss 
   */
  Metodos ( rssani_lite *rss );
  /**
   * 
   * @return
   */
  ulxr::MethodResponse verUltimo ( const ulxr::MethodCall &/*calldata*/ );
  /**
   * 
   * @return
   */
  ulxr::MethodResponse verTimer ( const ulxr::MethodCall &/*calldata*/ );
  /**
   * 
   * @param calldata 
   * @return 
   */
  ulxr::MethodResponse verLog ( const ulxr::MethodCall &calldata );
  /**
   * 
   * @return
   */
  ulxr::MethodResponse listaExpresiones ( const ulxr::MethodCall &/*calldata*/ );
  /**
   * 
   * @param calldata 
   * @return 
   */
  ulxr::MethodResponse anadirRegexp ( const ulxr::MethodCall &calldata );
  /**
   * 
   * @param calldata 
   * @return 
   */
  ulxr::MethodResponse editarRegexp ( const ulxr::MethodCall &calldata );
  /**
   * 
   * @param calldata 
   * @return 
   */
  ulxr::MethodResponse editarRegexpI ( const ulxr::MethodCall &calldata );
  /**
   *
   * @param calldata
   * @return
   */
  ulxr::MethodResponse activarRegexp ( const ulxr::MethodCall &calldata );
  /**
   *
   * @param calldata
   * @return
   */
  ulxr::MethodResponse moverRegexp ( const ulxr::MethodCall &calldata );
  /**
   * 
   * @param calldata 
   * @return 
   */
  ulxr::MethodResponse borrarRegexpI ( const ulxr::MethodCall &calldata );
  /**
   * 
   * @param calldata 
   * @return 
   */
  ulxr::MethodResponse borrarRegexpS ( const ulxr::MethodCall &calldata );
  /**
   * 
   * @param calldata 
   * @return 
   */
  ulxr::MethodResponse cambiaTimer ( const ulxr::MethodCall &calldata );

  /**
   * 
   * @param calldata 
   * @return 
   */
  ulxr::MethodResponse anadirAuth ( const ulxr::MethodCall &calldata );
  /**
   * 
   * @param calldata 
   * @return 
   */
  ulxr::MethodResponse borrarAuth ( const ulxr::MethodCall &calldata );
  /**
   * 
   * @param calldata 
   * @return 
   */
  ulxr::MethodResponse listaAuths ( const ulxr::MethodCall &/*calldata*/ );
  /**
   * 
   * @param calldata 
   * @return 
   */
  ulxr::MethodResponse verOpciones ( const ulxr::MethodCall &/*calldata*/ );
  /**
   * 
   * @param calldata 
   * @return 
   */
  ulxr::MethodResponse ponerOpciones ( const ulxr::MethodCall &calldata );
  /**
   * 
   * @param calldata 
   * @return 
   */
  ulxr::MethodResponse ponerCredenciales ( const ulxr::MethodCall &calldata );
  /**
   * 
   * @return
   */
  ulxr::MethodResponse guardar ( const ulxr::MethodCall &/*calldata*/ );
  /**
   * 
   * @return
   */
  ulxr::MethodResponse numThreads ( const ulxr::MethodCall &/*calldata*/ );
  /**
   * 
   * @return
   */
  ulxr::MethodResponse shutdown ( const ulxr::MethodCall &/*calldata*/ );
};

/**
 * Clase con el hilo de peticiones
 */
class rssxmlrpc : public QThread {

  public:
    /**
     * 
     * @param rss 
     */
    rssxmlrpc ( rssani_lite *rss );

  private:
    rssani_lite *rss;
  protected:
    /**
     *  Clase que implementa el loop del hilo
     */
    void run();

};

#endif
