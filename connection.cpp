#include "connection.h"
#include <QtNetwork>

#include <qdebug.h>

Connection::Connection(QObject *parent) :
    QTcpSocket(parent)
{
}

void Connection::startHandShake()
{
    qDebug() << "start HandShake with " << this->peerAddress() << this->peerPort();
    qDebug() << this->state();
}

void Connection::wait4HandShake()
{
}
