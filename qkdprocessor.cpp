#include "qkdprocessor.h"

#include <limits>
using std::max;
#include <math.h>
#include <QTime>

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
    qsrand(QTime::currentTime().msec());
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

quint16 QKDProcessor::calculateInitialBlockSize(qreal errorProbability)
{
    if(errorProbability < 0.02) {
        return 80;
    } else if(errorProbability < 0.05) {
        return 16;
    } else if(errorProbability < 0.07) {
        return 10;
    } else if(errorProbability < 0.1) {
        return 7;
    } else if(errorProbability < 0.15) {
        return 5;
    } else {
        emit logMessage(QString("EE: errorEstimation exceeded secure limit!"));
        return 5;
    }
}

bool QKDProcessor::calculateParity(Measurements::const_iterator begin, quint16 size)
{
    Measurements::const_iterator end = begin + size;
    bool parity = false;
    for(Measurements::const_iterator i = begin; i != end; i++)
        parity = parity ^ (*i).bit;
    return parity;
}

IndexList QKDProcessor::getRandomList(Index range)
{
    IndexList orderedList;
    for(Index i = 0; i < range; i++)
        orderedList.append(i);
    IndexList randomList;
    while(!orderedList.empty()) {
        Index randomIndex = qrand() % (orderedList.size()-1);
        randomList.append(orderedList.takeAt(randomIndex));
    }
    return randomList;
}

void QKDProcessor::incomingData(quint8 type, QVariant data)
{
    static IndexBoolPairList list;
    static IndexList remainingList;
    static Index errorEstimationSampleSize;
    static BoolList boolList;
    static Index errorCounter;
    static qreal error;
    static quint16 k1; // initial block size
    static BoolList parities;

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
        k1 = calculateInitialBlockSize(error);
        emit logMessage(QString("#02: resulting block size (k1): %1").arg(k1));
        emit logMessage(QString("Error Estimation finished"), Qt::green);

        emit logMessage("Initial Parity Check started", Qt::green);
        parities.clear();
        Measurements::const_iterator last = measurements->end() - k1; // there might be unhandled bits < k1; (TODO)
        for(Measurements::const_iterator index = measurements->begin(); index <= last; index+=k1) {
            bool parity = calculateParity(index, k1);
            parities.append(parity);
        }

        if(isMaster) {
            emit sendData(PT03blockParities, QVariant::fromValue<BoolList>(parities));
        }

        return;
    }

    case PT03blockParities: {
        if(!isMaster) {
            BoolList compareParities = data.value<BoolList>();
        } else {

        }
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
