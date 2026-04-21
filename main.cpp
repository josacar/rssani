#include <memory>
#include <QtCore/QCoreApplication>
#include "rssani_lite.h"
#include "grpc_server.h"

#ifdef __unix__
#include <signal.h>
#endif

int main ( int argc, char **argv ) {
#ifdef __unix__
	signal(SIGPIPE, SIG_IGN);
#endif
	QCoreApplication app(argc, argv);
	app.setOrganizationName(QString("Selu"));
	app.setApplicationName(QString("rssani"));
	auto rss = std::make_unique<rssani_lite>();
	auto grpc = std::make_unique<GrpcServer>(rss.get());
	grpc->start();
	return app.exec();
}
