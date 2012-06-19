#include "qkdprocessor.h"

#include <limits>
#include <algorithm>
using std::max;
using std::random_shuffle;
#include <qmath.h>
#include <QTime>
#include <qdebug.h>

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
    // qsrand(QTime::currentTime().msec());
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

bool QKDProcessor::calculateParity(const Measurements measurements, const Index index, const quint16 &size) const
{
    Q_ASSERT(size > 0);
    const Index end = index + size;
    Q_ASSERT((qint64)end <= measurements.size());
    bool parity = false;
    for(Index i = index; i < end; i++) {
        parity = parity ^ measurements.at(i)->bit;
    }
    return parity;
}

IndexList QKDProcessor::getOrderedList(Index range)
{
    IndexList orderedList;
    for(Index i = 0; i < range; i++)
        orderedList.append(i);
    return orderedList;
}

IndexList QKDProcessor::getRandomList(Index range)
{
    IndexList list = getOrderedList(range);
    // http://www.cplusplus.com/reference/algorithm/random_shuffle/
    random_shuffle(list.begin(), list.end());
    return list;
}

Measurements QKDProcessor::reorderMeasurements(IndexList order)
{
    Measurements list;
    foreach(Index index, order) {
        list.append(measurements->at(index));
    }

    return list;
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
    static QList<IndexList> orders;
    static IndexList corruptBlocks;
    static QList<Measurements> reorderedMeasurements;
    static quint16 blockSize;
    static quint16 binaryBlockSize;
    static Index blockCount;

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
                if(measurements->at(index)->valid)
                    list.append(IndexBoolPair(index,measurements->at(index)->base));
            }
            emit sendData(PT01sendReceivedList, QVariant::fromValue(list));
        }
        emit logMessage(QString("#01: Valid measurements (containing 1 received photons): %1 (%2%)").arg(list.size()).arg((double)list.size()*100/measurements->size()));


        if(isMaster) {
            IndexBoolPair pair;
            remainingList.clear();
            foreach(pair, list) {
                if(measurements->at(pair.first)->base == pair.second)
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

        {
            int bit = 0;
            int base = 0;
            foreach(Measurement *measurement, *this->measurements) {
                bit += measurement->bit;
                base += measurement->base;
            }

            emit logMessage(QString("Stats ratio: bit = %1%, base = %2%").arg((double)bit/measurements->size()).arg((double)base/measurements->size()));
        }

        list.clear();
        remainingList.clear();
        emit logMessage(QString("Sifting Procedure finished"), Qt::green);
        if(isMaster) {
            errorEstimationSampleSize = qCeil(measurements->size()*0.05);
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
                boolList.append(measurements->takeLast()->bit);
            }

            emit sendData(PT02errorEstimationSendSample, QVariant::fromValue(boolList));
        } else {
            boolList = data.value<BoolList>();
            errorCounter = 0;
            Q_ASSERT(measurements->size() >= (qint64)errorEstimationSampleSize);
            for(Index index = 0; index < errorEstimationSampleSize; index++) {
                if(measurements->takeLast()->bit != boolList.at(index))
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
        quint16 removeBits = measurements->size() % k1;
        for(quint16 i = 0; i < removeBits; i++)
            measurements->removeLast();

        orders.clear();
        reorderedMeasurements.clear();

        QVariant data = QVariant::fromValue<IndexList>(getOrderedList(measurements->size()));
        this->incomingData(PT03prepareBlockParities, data);

        return;
    }

    case PT03prepareBlockParities: {
        emit logMessage("in PT03prepareBlockParities");
        blockSize = k1*pow(2,orders.size());
        blockCount = measurements->size()/blockSize;
        /*
         * BUG: Make sure that blockCount is always > 0
         * (interesting for measurements.size very small
         *
         * BUG: blockSize*blockCount<measurements->size()
         *      für größer werdende blockSize
         */
        emit logMessage(QString("ki: %1").arg(blockSize));
        binaryBlockSize = blockSize;
        orders.append(data.value<IndexList>());
        emit logMessage(QString("elements in orders.last: %1").arg(orders.last().size()));
        reorderedMeasurements.append(reorderMeasurements(orders.last()));
        parities.clear();
        Index lastIndex = measurements->size() - blockSize;
        for(Index index = 0; index <= lastIndex; index += blockSize) {
            parities.append(calculateParity(reorderedMeasurements.last(), index, blockSize));
        }
        emit logMessage(QString("elements in parities: %1").arg(parities.size()));

        if(isMaster) {
            emit sendData(PT04reportBlockParities, QVariant::fromValue<BoolList>(parities));
        }

        return;

    }
    case PT04reportBlockParities: {
        emit logMessage("in PT04reportBlockParities");
        if(!isMaster) {
            qint64 size = parities.size();
            Q_ASSERT(size > 0);
            BoolList compareParities = data.value<BoolList>();
            Q_ASSERT(compareParities.size() == size);
            corruptBlocks.clear();
            /*
            {
                QString tmp;
                tmp = "N=>";
                for(int j = 0; j < size; j++)
                    tmp += QString("%1 ").arg(j);
                emit logMessage(tmp);
                tmp = "A: ";
                foreach(bool p, compareParities)
                    tmp += QString("%1 ").arg((int)p);
                emit logMessage(tmp);
                tmp = "B: ";
                foreach(bool p, parities)
                    tmp += QString("%1 ").arg((int)p);
                emit logMessage(tmp);

            }
            */
            for(Index index = 0; index < size; index++) {
                if(compareParities.at(index) != parities.at(index))
                    corruptBlocks.append(index*blockSize);
            }
            emit sendData(PT04reportBlockParities, QVariant::fromValue<IndexList>(corruptBlocks));
        } else {
            corruptBlocks = data.value<IndexList>();
        }
        emit logMessage(QString("number of corrupt Blocks: %1/%2 (%3%)").arg(corruptBlocks.size())
                                                                        .arg(blockCount)
                                                                        .arg((double)corruptBlocks.size()/blockCount*100)
                        );
        /*
        {
            for(Index j = 0; (qint64)j < corruptBlocks.size(); j++) {
                Index index = corruptBlocks.at(j);
                QString strIndex = QString("%1: ").arg(index);
                Index end = index+binaryBlockSize;
                for(Index i = index; i < end; i++)
                    strIndex += QString("%1 ").arg((int)(reorderedMeasurements.last().at(i)->bit));
                emit logMessage(strIndex);
            }
        }
        */
        if(isMaster) {
            if(corruptBlocks.empty()) {
                if(orders.size() < 5) {
                    QVariant data = QVariant::fromValue<IndexList>(getRandomList(measurements->size()));
                    emit sendData(PT03prepareBlockParities, data);
                    this->incomingData(PT03prepareBlockParities, data);
                } else {
                    emit sendData(PT06finished);
                }
            } else { // start Binary
                this->incomingData(PT05startBinary);
            }
        }
        return;
    }

    case PT05startBinary: {
        binaryBlockSize = binaryBlockSize/2;
        emit logMessage(QString("in PT05startBinary with binaryBlockSize = %1").arg(binaryBlockSize));
        parities.clear();
        if(isMaster || (!isMaster && binaryBlockSize > 1)) {
            foreach(Index index, corruptBlocks) {
                parities.append(calculateParity(reorderedMeasurements.last(), index, binaryBlockSize));
            }
        }
        if(isMaster) {
            emit sendData(PT05startBinary, QVariant::fromValue<BoolList>(parities));
        } else {
            BoolList compareParities = data.value<BoolList>();
            quint32 size = compareParities.size();
            Q_ASSERT(corruptBlocks.size() == compareParities.size());
            if(binaryBlockSize > 1)  {
                Q_ASSERT((qint64)size == parities.size());
                for(Index i = 0; i < size; i++) {
                    if(compareParities.at(i) == parities.at(i)) {
                        // the 2nd half of the block must me corrupt
                        corruptBlocks.replace(i, corruptBlocks.at(i)+binaryBlockSize);
                    }
                }
            } else { // just toggle wrong bits
                for(Index i = 0; i < size; i++) {
                    reorderedMeasurements.last().at(corruptBlocks.at(i))->bit = compareParities.at(i);
                }

            }
        }
        if(binaryBlockSize == 1) {
            corruptBlocks.clear();
        }
        if(!isMaster) {
            emit sendData(PT04reportBlockParities, QVariant::fromValue<IndexList>(corruptBlocks));
        }
        return;
    }
    case PT06finished: {
        if(!isMaster)
            emit sendData(PT06finished);
        emit logMessage("fertig!");
        return;
    }

    }
}

void QKDProcessor::setMeasurements(Measurements *measurements)
{
    if(this->measurements) {
        qDeleteAll(*this->measurements);
        this->measurements->clear();
        delete this->measurements;
    }

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
