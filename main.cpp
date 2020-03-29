#include <QtCore/QTextCodec>
#include <QtCore/QCoreApplication>
#include "rssani_lite.h"
#include "xmlrpc.h"

#ifdef __unix__
#include <sys/signal.h>
#endif

#include "mailsender.h"

int main ( int argc, char **argv ) {
#ifdef __unix__
	signal(SIGPIPE, SIG_IGN);
#endif
	QCoreApplication app(argc, argv);
    app.setOrganizationName(QString("Selu"));
    app.setApplicationName(QString("rssani"));
    rssani_lite *rss = new rssani_lite();
	rssxmlrpc * rpc = new rssxmlrpc(rss);
	rpc->start();
	return app.exec();
}
