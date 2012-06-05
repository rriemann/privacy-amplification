#include "qkdprocessor.h"

#include <limits>
using std::max;

typedef quint32 Index;

typedef QList<Index> IndexList;
Q_DECLARE_METATYPE(IndexList)
static int id5 = qRegisterMetaType<IndexList>();
static int id6 = qRegisterMetaTypeStreamOperators<IndexList>();

typedef QPair<quint32,bool> IndexBoolPair;
Q_DECLARE_METATYPE(IndexBoolPair)
static int id7 = qRegisterMetaType<IndexBoolPair>();
static int id8 = qRegisterMetaTypeStreamOperators<IndexBoolPair>();

typedef QList<IndexBoolPair> IndexBoolPairList;
Q_DECLARE_METATYPE(IndexBoolPairList)
static int id9  = qRegisterMetaType<IndexBoolPairList>();
static int id10 = qRegisterMetaTypeStreamOperators<IndexBoolPairList>();

QKDProcessor::QKDProcessor(QObject *parent) :
    QObject(parent), measurements(0), isMaster(false), state(CSready)
{
}

QKDProcessor::~QKDProcessor()
{
    /* TODO FIXME
    if(measurements)
        delete measurements;
        */
}

void QKDProcessor::incomingData(quint8 type, QVariant data)
{
    switch((PackageType)type) {
    case PT01sendReceivedList:
        IndexList list;
        emit logMessage(QString("Sifting Procedure started"), Qt::green);
        emit logMessage(QString("#01: File contains %1 measurements").arg(measurements->size()));
        if(isMaster) {
            list = data.value<IndexList>();
        } else {
            Q_ASSERT((qint64)std::numeric_limits<Index>::max() >= measurements->size());
            for(int index = 0; index < measurements->size(); index++) {
                if(measurements->at(index).valid)
                    list.append(index);
            }
            emit sendData(PT01sendReceivedList, QVariant::fromValue(list));
        }
        emit logMessage(QString("#01: Valid measurements (containing 1 received photons): %1 (%2%)").arg(list.size()).arg((double)list.size()*100/measurements->size()));

        return;
    }
}

void QKDProcessor::setMeasurements(Measurements *measurements)
{
    if(this->measurements)
        delete this->measurements;

    this->measurements = measurements;
}

void QKDProcessor::start(bool isMaster)
{
    this->isMaster = isMaster;

    if(isMaster) { // this is Alice
        // #1 we kindly ask Bob for a list a received bits in first range from 0-quint32
        state = CS01;
        emit sendData(PT01sendReceivedList);
    } else { // this is Bob
        state = CSstarted;
    }
}
