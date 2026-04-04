#include <memory>
#include <QtCore/QCoreApplication>
#include "rssani_lite.h"
#include "xmlrpc.h"

#ifdef __unix__
#include <sys/signal.h>
#endif

int main ( int argc, char **argv ) {
#ifdef __unix__
	signal(SIGPIPE, SIG_IGN);
#endif
	QCoreApplication app(argc, argv);
	app.setOrganizationName(QString("Selu"));
	app.setApplicationName(QString("rssani"));
	auto rss = std::make_unique<rssani_lite>();
	auto rpc = std::make_unique<rssxmlrpc>(rss.get());
	rpc->start();
	return app.exec();
}
