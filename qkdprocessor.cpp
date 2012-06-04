#include "qkdprocessor.h"

#include <limits>
using std::max;


typedef QList<quint32> Quint32List;
Q_DECLARE_METATYPE(Quint32List)
static int id5 = qRegisterMetaType<Quint32List>();
static int id6 = qRegisterMetaTypeStreamOperators<Quint32List>();

QKDProcessor::QKDProcessor(QObject *parent) :
    QObject(parent), measurements(0), isMaster(false), state(CSready)
{
}

QKDProcessor::~QKDProcessor()
{
    if(measurements)
        delete measurements;
}

void QKDProcessor::incomingData(quint8 type, QVariant data)
{
    if(type <= (quint8)PTprepareQKDProcessor)
        return;

    if(isMaster) {
        incomingDataAlice(type, data);
    } else {
        incomingDataBob(type, data);
    }
}

void QKDProcessor::setMeasurements(Measurements *measurements)
{
    if(measurements)
        delete measurements;

    this->measurements = measurements;
}

void QKDProcessor::start(bool isMaster)
{
    this->isMaster = isMaster;

    if(isMaster) { // this is Alice
        // #1 we kindly ask Bob for a list a received bits in first range from 0-quint32
        state = CS01;
        emit sendData(PT01sendReceivedList);
        emit sendData(PT01sendReceivedList);
    } else { // this is Bob
        state = CSstarted;
    }
}

void QKDProcessor::incomingDataAlice(quint8 type, QVariant data)
{
    switch((PackageType)type) {
    case PTprepareQKDProcessor:
    case PT01sendReceivedList: {
        Quint32List list = data.value<Quint32List>();

        emit logMessage(QString("#01: got list of size %1").arg(list.size()));
    }
        break;
    }
}

void QKDProcessor::incomingDataBob(quint8 type, QVariant data)
{
    switch((PackageType)type) {
    case PTprepareQKDProcessor:
    case PT01sendReceivedList: {
        Quint32List list;
        // std::numeric_limits
        for(int index = 0; index < measurements->size(); index++) {
            if(measurements->at(index).photon)
                list.append(index);
        }
        emit logMessage(QString("#01: send list of size %1").arg(list.size()));
        emit sendData(PT01sendReceivedList, QVariant::fromValue(list));
    }
        break;
    }
}
