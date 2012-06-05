#include "qkdprocessor.h"

#include <limits>
using std::max;
#include <math.h>

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

typedef QList<bool> BoolList;
Q_DECLARE_METATYPE(BoolList)
static int id11 = qRegisterMetaType<BoolList>();
static int id12 = qRegisterMetaTypeStreamOperators<BoolList>();

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

void QKDProcessor::siftMeasurements(IndexList list)
{
    Measurements *siftedMeasurements = new Measurements;

    Index index;
    foreach(index, list) {
        siftedMeasurements->append(measurements->at(index));
    }

    delete measurements;

    measurements = siftedMeasurements;
}

void QKDProcessor::incomingData(quint8 type, QVariant data)
{
    static IndexBoolPairList list;
    static IndexList remainingList;
    static Index errorEstimationSampleSize;
    static BoolList boolList;
    static Index errorCounter;
    static qreal error;

    switch((PackageType)type) {
    // Sifting Procedure
    case PT01sendReceivedList: {
        emit logMessage(QString("Sifting Procedure started"), Qt::green);
        emit logMessage(QString("#01: File contains %1 measurements").arg(measurements->size()));
        if(isMaster) {
            list = data.value<IndexBoolPairList>();
        } else {
            list.clear();
            Q_ASSERT((qint64)std::numeric_limits<Index>::max() >= measurements->size());
            for(int index = 0; index < measurements->size(); index++) {
                if(measurements->at(index).valid)
                    list.append(IndexBoolPair(index,measurements->at(index).base));
            }
            emit sendData(PT01sendReceivedList, QVariant::fromValue(list));
        }
        emit logMessage(QString("#01: Valid measurements (containing 1 received photons): %1 (%2%)").arg(list.size()).arg((double)list.size()*100/measurements->size()));


        if(isMaster) {
            IndexBoolPair pair;
            remainingList.clear();
            foreach(pair, list) {
                if(measurements->at(pair.first).base == pair.second)
                    remainingList.append(pair.first);
            }
            emit sendData(PT01sendRemainingList, QVariant::fromValue(remainingList));
        }

        return;
    }
    case PT01sendRemainingList: {
        if(isMaster) {
        } else {
            remainingList = data.value<IndexList>();
            emit sendData(PT01sendRemainingList);
        }
        emit logMessage(QString("#02: Remaining measurements after sifting (same base): %1 (%2%)").arg(remainingList.size()).arg((double)remainingList.size()*100/list.size()));
        siftMeasurements(remainingList);
        list.clear();
        remainingList.clear();
        emit logMessage(QString("Sifting Procedure finished"), Qt::green);
        if(isMaster) {
            errorEstimationSampleSize = ceil(measurements->size()*0.05);
            emit sendData(PT02errorEstimationSendSample, errorEstimationSampleSize); // invoke Bob to send BitSample
        }
        return;
    }
    // Error Estimation
    case PT02errorEstimationSendSample: {
        emit logMessage(QString("Error Estimation started"), Qt::green);
        boolList.clear();
        if(!isMaster) {
            // prepare Bit Sample for error Estimation and send it to Bob
            // sample is taken from the end of list for memory efficiency (we are using QList)
            errorEstimationSampleSize = data.value<Index>();
            Q_ASSERT(measurements->size() >= (qint64)errorEstimationSampleSize);
            for(Index index = 0; index < errorEstimationSampleSize; index++) {
                boolList.append(measurements->takeLast().bit);
            }
            emit sendData(PT02errorEstimationSendSample, QVariant::fromValue(boolList));
        } else {
            boolList = data.value<BoolList>();
            errorCounter = 0;
            Q_ASSERT(measurements->size() >= (qint64)errorEstimationSampleSize);
            for(Index index = 0; index < errorEstimationSampleSize; index++) {
                if(measurements->takeLast().bit != boolList.at(index))
                    errorCounter++;
            }
            emit sendData(PT02errorEstimationReport, QVariant::fromValue<Index>(errorCounter));
        }
        return;
    }
    case PT02errorEstimationReport: {
        if(!isMaster) {
            errorCounter = data.value<Index>();
            emit sendData(PT02errorEstimationReport);
        } else {
        }
        error = (double)errorCounter/boolList.size();
        emit logMessage(QString("#01: Estimated error rate: %1 / %2 (%3%)").arg(errorCounter).arg(boolList.size()).arg(error*100));
        boolList.clear();
        emit logMessage(QString("Error Estimation finished"), Qt::green);

        return;
    }

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
