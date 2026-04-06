#ifndef XMLRPC_H
#define XMLRPC_H

#include <atomic>
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
    ~rssxmlrpc() override;

protected:
    void run() override;

private:
    rssani_lite *rss;
    std::atomic<xmlrpc_c::serverAbyss *> server{nullptr};
};

#endif
