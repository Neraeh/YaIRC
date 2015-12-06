#-------------------------------------------------
#
# Project created by QtCreator 2015-12-05T11:43:28
#
#-------------------------------------------------

QT       += core gui widgets network
CONFIG += communi
COMMUNI += core model util

QMAKE_CFLAGS_RELEASE += -O2 -pipe
QMAKE_CXXFLAGS_RELEASE += $$QMAKE_CFLAGS_RELEASE
QMAKE_LFLAGS_RELEASE += -s

TARGET = YaIRC
TEMPLATE = app

SOURCES += main.cpp \
    yairc.cpp \
    ircmessageformatter.cpp \
    inputhistory.cpp

HEADERS  += \
    yairc.h \
    ircmessageformatter.h \
    inputhistory.h

RESOURCES += \
    resources.qrc

FORMS += \
    mainwindow.ui

DISTFILES += \
    resources.rc
