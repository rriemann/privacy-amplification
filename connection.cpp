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

    char *buffer;
    quint32 length;
    in.readBytes(buffer, length);
    QByteArray block(buffer, length); // deep copy of buffer
    delete [] buffer;
    bool valid;
    authenticator.authenticate(block, valid);
    if(valid) {
        emit logMessage("authentication valid!");
    } else {
        emit logMessage("authentication failed!");
    }
    QDataStream bufferStream(&block, QIODevice::ReadOnly);
    bufferStream.setVersion(streamVersion);

    quint16 type;
    bufferStream >> type;

    QVariant data;
    bufferStream >> data;


    emit receivedData((PackageType)type, data);
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
    Q_ASSERT(out.device()->size() == block.size());
    out.device()->seek(block.size());
    out << authenticator.token(block);

    qint64 wsize = this->write(block);
    Q_UNUSED(wsize);
    Q_ASSERT(wsize == block.size());
    this->flush();
}
