#ifndef XMLRPC_H
#define XMLRPC_H

#include <atomic>
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>
#include <QThread>
#include "rssani_lite.h"

/**
 * @brief Thread that runs the XML-RPC server (xmlrpc-c / Abyss).
 */
class rssxmlrpc : public QThread {
public:
    /**
     * @brief Constructs the XML-RPC server thread.
     * @param rss Pointer to the core application instance.
     */
    rssxmlrpc(rssani_lite *rss);

    /**
     * @brief Destructor. Terminates the Abyss server and waits for the thread to finish.
     */
    ~rssxmlrpc() override;

protected:
    void run() override;

private:
    rssani_lite *rss;
    std::atomic<xmlrpc_c::serverAbyss *> server{nullptr};
};

#endif
