#include "connection.h"
#include <QtNetwork>

#include <qdebug.h>

const QDataStream::Version Connection::streamVersion = QDataStream::Qt_4_5;

Connection::Connection(QObject *parent) :
    QTcpSocket(parent), blockSize(0)
{
}

void Connection::receiveData()
{
    QDataStream in(this);
    in.setVersion(streamVersion);
    if (blockSize == 0) {
        if (this->bytesAvailable() < (int)sizeof(quint16))
            return;
        in >> blockSize;
    }
    if (this->bytesAvailable() < blockSize)
        return;

    QVariant data;
    in >> data;
    emit receivedData(data);
    blockSize = 0;

}

void Connection::sendData(const QVariant &data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(streamVersion);
    out << (quint16)0;
    out << data;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));

    this->write(block);
}
