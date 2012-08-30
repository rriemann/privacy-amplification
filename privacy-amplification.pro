#-------------------------------------------------
#
# Project created by QtCreator 2012-05-18T16:35:44
#
#-------------------------------------------------

QT       += core gui network thread

TARGET = privacy-amplification
TEMPLATE = app


SOURCES += main.cpp\
    mainwindow.cpp \
    client.cpp \
    server.cpp \
    connection.cpp \
    file.cpp \
    qkdprocessor.cpp \
    demomode.cpp \
    authenticator.cpp

HEADERS  += mainwindow.h \
    client.h \
    server.h \
    connection.h \
    file.h \
    measurement.h \
    qkdprocessor.h \
    demomode.h \
    authenticator.h

FORMS    += mainwindow.ui
