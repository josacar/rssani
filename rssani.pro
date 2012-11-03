HEADERS += rssani_lite.h \
 rss_lite.h \
 values.h \
 xmlrpc.h \
 myircsession.h \
 mailsender.h
SOURCES += main.cpp \
 rssani_lite.cpp \
 rss_lite.cpp \
 xmlrpc.cpp \
 myircsession.cpp \
 mailsender.cpp

QT -= gui 
QT += network xml

TEMPLATE = app



LIBS += -L/usr/lib \
  -lircclient-qt \
  -lulxmlrpcpp

TARGET = rssani_console


DEFINES += QT_NO_CAST_FROM_ASCII

INCLUDEPATH += /usr/include/ircclient-qt/

