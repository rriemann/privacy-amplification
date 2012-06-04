#-------------------------------------------------
#
# Project created by QtCreator 2012-05-18T16:35:44
#
#-------------------------------------------------

QT       += core gui network

TARGET = privacy-amplification
TEMPLATE = app


SOURCES += main.cpp\
    mainwindow.cpp \
    client.cpp \
    server.cpp \
    connection.cpp \
    file.cpp \
    qkdprocessor.cpp

HEADERS  += mainwindow.h \
    client.h \
    server.h \
    connection.h \
    file.h \
    measurement.h \
    qkdprocessor.h

FORMS    += mainwindow.ui
