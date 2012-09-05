#include "connection.h"
#include <QtNetwork>

const QDataStream::Version Connection::streamVersion = QDataStream::Qt_4_5;

Connection::Connection(QObject *parent) :
    QTcpSocket(parent), blockSize(0),
    authenticator("/home/rriemann/Documents/Backup/Studium/Masterarbeit/sampledata-600MB.bin")
{
}

void Connection::receiveData()
{
    QDataStream in(this);
    in.setVersion(streamVersion);
    if (blockSize == 0) {
        if (this->bytesAvailable() < (int)sizeof(qint64))
            return;
        in >> blockSize;
    }
    if (this->bytesAvailable() < blockSize)
        return;

    QByteArray block = this->read(blockSize);
    if(authenticator.authenticate(block)) {
        emit logMessage("authentication successfull", Qt::darkYellow);
        QDataStream bufferStream(&block, QIODevice::ReadOnly);
        bufferStream.setVersion(streamVersion);

        quint16 type;
        bufferStream >> type;

        QVariant data;
        bufferStream >> data;

        emit receivedData((PackageType)type, data);
    } else {
        emit logMessage("authentication failed", Qt::darkRed);
    }
    blockSize = 0;

}

void Connection::sendData(const PackageType type, const QVariant &data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(streamVersion);
    out << (qint64)0;
    out << (quint16)type;
    out << data;
    out.device()->seek(0);
    out << (quint64)(block.size() + authenticator.getSecurityLevel() - sizeof(qint64));
    out.device()->seek(block.size());
    out.writeRawData(authenticator.token(block), authenticator.getSecurityLevel());

    qint64 wsize = this->write(block);
    Q_UNUSED(wsize);
    Q_ASSERT(wsize == block.size());
    this->flush();
}
