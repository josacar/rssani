#ifndef XMLRPC_H
#define XMLRPC_H

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>
#include <QThread>
#include "rssani_lite.h"

/**
 * Clase con el hilo de peticiones XML-RPC (xmlrpc-c / Abyss)
 */
class rssxmlrpc : public QThread {
public:
    rssxmlrpc(rssani_lite *rss);

protected:
    void run() override;

private:
    rssani_lite *rss;
};

#endif
