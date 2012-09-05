#include "authenticator.h"
#include <qdebug.h>

Authenticator::Authenticator(QString fileName, QObject *parent) :
    QFile(fileName, parent), level(1)
{
    if(!this->open(QIODevice::ReadOnly)) {
        exit(12);
    }
    this->seek(0);
}

Authenticator::~Authenticator() {
    this->close();
}

QByteArray Authenticator::token(const QByteArray &data)
{
    QByteArray *outData = new QByteArray(data);
    QByteArray *inData  = new QByteArray;

    typedef quint8  bufferType;
    typedef quint16 bufferTypeBig;

    // static const quint8 bitLimit      = sizeof(bufferType)*8;
    // static const quint8 bitLimitSmall = bitLimit/2;

    bufferType privateKey;
    bufferType value;
    bufferType product;
    bufferType resultByteBuffer;
    do {
        qSwap(inData, outData);
        outData->clear();
        resultByteBuffer = 0;
        const int size = inData->size();
        this->getChar((char*)&privateKey);
        if(!(bool)(privateKey & 1)) {
            privateKey += 1; // make privateKey an odd number
        }
        for(int index = 0; index < size; index++) {
            value = (bufferType)inData->at(index);
            product = ((value*privateKey) & 240); //  & 240 implements modulo 2^8
            if(!(bool)(index & 1)) { // index is even
                resultByteBuffer  = product >> 4; // division by 2^4
            } else { // index is odd
                resultByteBuffer |= product;
                outData->append((char)resultByteBuffer);
                resultByteBuffer = 0;
            }
        }
        if((bool)(size & 1)) { // size is odd
            // in case of left data in buffer, append it now
            outData->append((char)resultByteBuffer);
        }

    } while(outData->size() > level);

    QByteArray result = QByteArray::fromRawData(outData->data(), outData->size());
    qDebug() << result.toPercentEncoding();

    delete inData;
    delete outData;

    return result;
}

QByteArray &Authenticator::tokenize(QByteArray &data)
{
    return data.append(token(data));
}

bool Authenticator::check(const QByteArray &data)
{
    QByteArray check = data.right(level);

    QByteArray result = token(data.left(data.size()-level));

    return (check == result);
}

bool Authenticator::authenticate(QByteArray &data)
{
    QByteArray check = data.right(level);
    data.chop(level);
    return (check == token(data));
}

quint16 Authenticator::getSecurityLevel()
{
    return level;
}


void Authenticator::setSecurityLevel(quint16 level)
{
    this->level = level;
}
