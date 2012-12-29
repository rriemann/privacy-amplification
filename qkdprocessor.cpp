#include "qkdprocessor.h"

#include <limits>
#include <algorithm>
using std::max;
using std::random_shuffle;
#include <qmath.h>
#include <QTime>
#include <QRgb>
#include <QDataStream>
#include <QFile>
#include <QLabel>
#include <qdebug.h>

const int QKDProcessor::idIndexList = qRegisterMetaType<IndexList>();

QKDProcessor::QKDProcessor(QObject *parent) :
    QObject(parent), measurements(0), isMaster(false), state(CSready)
{
    qsrand(QTime::currentTime().msec());

    qRegisterMetaTypeStreamOperators<IndexList>();

    qRegisterMetaType<IndexBoolPair>();
    qRegisterMetaTypeStreamOperators<IndexBoolPair>();

    qRegisterMetaType<IndexBoolPairList>();
    qRegisterMetaTypeStreamOperators<IndexBoolPairList>();

    qRegisterMetaType<BoolList>();
    qRegisterMetaTypeStreamOperators<BoolList>();
}

void QKDProcessor::clearMeasurements()
{
    if(measurements) {
        qDeleteAll(*measurements);
        delete measurements;
    }
}

QByteArray QKDProcessor::privacyAmplification(const Measurements measurements, const qreal ratio)
{
    /* - count of bits to represent integer: N=ceil(log2(int))
     * - count of bits to represent squared integer: 2N
     * - must have double size of bufferTypeSmall to prevent integer overflow
     * - in any case only half of the bits of squared integer is used, so we can
     *   accept integer overflow and use the same bit size for bufferType, too.
     * - bigger types are better, because precision increasing in:
     *   floor(bitLimitSmall*ratio). step size: 100/64 => 1.56%
     */
    typedef quint64 bufferType;
    typedef quint64 bufferTypeSmall;

    static const quint8 bitLimitSmall = sizeof(bufferTypeSmall)*8;

    bufferTypeSmall bufferBits = 0;
    bufferTypeSmall bufferBase = 0;

    QByteArray finalKey;
    // only cstrings and char can be added to QByteArrays: we choose char
    typedef char finalKeyBufferType;

    finalKeyBufferType buffer = 0;
    const quint8 bufferSize = sizeof(finalKeyBufferType)*8;
    const quint8 bitCount = qFloor(bitLimitSmall*ratio);

    quint8 pos = 0;
    quint8 bufferPos = 0;
    const quint64 quint64_1 = Q_UINT64_C(1);
    foreach(Measurement *measurement, measurements) {
        bufferBase |= (bufferTypeSmall)measurement->base << pos;
        bufferBits |= (bufferTypeSmall)measurement->bit  << pos;
        pos++;
        if(pos == bitLimitSmall) {
            pos = 0;
            bufferType temp = (bufferType)bufferBase*(bufferType)bufferBits;
            for(quint8 bitPos = 0; bitPos < bitCount; bitPos++) {
                // http://stackoverflow.com/a/2249738/1407622
                buffer |= ((temp & ( quint64_1 << bitPos )) >> bitPos) << bufferPos;
                bufferPos++;
                if(bufferPos == bufferSize) {
                    bufferPos = 0;
                    finalKey.append(buffer);
                    buffer = 0;
                }
            }
            bufferBase = 0;
            bufferBits = 0;
        }
    }
    // there might be fewer bits in finalKey than ratio*measurements->size()
    // this is due remaining (unused) bits in buffers
    return finalKey;
}

void QKDProcessor::setMeasurements(Measurements *measurements)
{
    clearMeasurements();
    this->measurements = measurements;
}

void QKDProcessor::setMaster(bool isMaster)
{
    this->isMaster = isMaster;
}

QKDProcessor::~QKDProcessor()
{
    clearMeasurements();
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
        return 6;
    } else {
        emit logMessage(QString("EE: errorEstimation exceeded secure limit!"));
        return 4;
    }
}

bool QKDProcessor::calculateParity(const Measurements measurements,
                                   const Index index, const quint16 &size) const
{
    Q_ASSERT(size > 0);
    const Index end = index + size;
    Q_ASSERT((SIndex)end <= measurements.size());
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

Measurements QKDProcessor::reorderMeasurements(const Measurements measurements, const IndexList order)
{
    Measurements list;
    foreach(Index index, order) {
        list.append(measurements.at(index));
    }

    return list;
}

void QKDProcessor::incomingData(quint8 type, QVariant data)
{

    switch((PackageType)type) {
    // Sifting Procedure
    case PT01sendReceivedList: {
        emit logMessage(QString("Sifting Procedure started"), Qt::green);
        emit logMessage(QString("#01: File contains %1 measurements").
                        arg(measurements->size()));
        if(isMaster) {
            list = data.value<IndexBoolPairList>();
        } else {
            list.clear();
            Q_ASSERT((SIndex)std::numeric_limits<Index>::max() >= measurements->size());
            for(int index = 0; index < measurements->size(); index++) {
                if(measurements->at(index)->valid)
                    list.append(IndexBoolPair(index,measurements->at(index)->base));
            }
            emit sendData(PT01sendReceivedList, QVariant::fromValue(list));
        }
        emit logMessage(QString("#01: Valid measurements (containing 1 received photons): %1 (%2%)").
                        arg(list.size()).arg((double)list.size()*100/measurements->size()));


        if(isMaster) {
            IndexBoolPair pair;
            remainingList.clear();
            BoolList basesList;
            foreach(pair, list) {
                const bool &ownBase = measurements->at(pair.first)->base;
                basesList.append(ownBase);
                if(ownBase == pair.second)
                    remainingList.append(pair.first);
            }
            emit sendData(PT01sendRemainingList,
                          QVariant::fromValue(basesList));
        }

        reorderedMeasurements.clear();

        return;
    }
    case PT01sendRemainingList: {
        if(isMaster) {
        } else {
            BoolList basesList = data.value<BoolList>();
            Q_ASSERT(list.size() == basesList.size());
            remainingList.clear();
            for(SIndex index = 0; index < list.size(); index++) {
                const IndexBoolPair &pair = list.at(index);
                if(basesList.at(index) == pair.second)
                    remainingList.append(pair.first);
            }
            emit sendData(PT01sendRemainingList);
        }
        emit logMessage(QString("#02: Remaining measurements after sifting (same base): %1 (%2%)").
                        arg(remainingList.size()).
                        arg((double)remainingList.size()*100/list.size()));
        {
            Measurements siftedMeasurements;
            foreach(Index index, remainingList) {
                siftedMeasurements.append(measurements->at(index));
            }
            reorderedMeasurements.append(siftedMeasurements);

        }

        {
            int bit = 0;
            int base = 0;
            Measurements measurements = reorderedMeasurements.first();
            foreach(Measurement *measurement, measurements) {
                bit += measurement->bit;
                base += measurement->base;
            }

            emit logMessage(QString("Stats ratio: bit = %1%, base = %2%").
                            arg(100.0*bit/measurements.size()).
                            arg(100.0*base/measurements.size()));
        }

        list.clear();
        remainingList.clear();
        emit logMessage(QString("Sifting Procedure finished"), Qt::green);

        if(isMaster) {
            IndexList order = getRandomList(reorderedMeasurements.first().size());
            reorderedMeasurements.replace(0, reorderMeasurements(reorderedMeasurements.last(), order));
            emit sendData(PT02errorEstimationSendSample,
                          QVariant::fromValue<IndexList>(order));
        }
        return;
    }
    // Error Estimation
    case PT02errorEstimationSendSample: {
        emit logMessage(QString("Error Estimation started"), Qt::green);
        if(!isMaster) {
            IndexList order = data.value<IndexList>();
            reorderedMeasurements.replace(0,reorderMeasurements(reorderedMeasurements.last(), order));
        }
        Q_ASSERT(reorderedMeasurements.size() == 1);
        Q_ASSERT(reorderedMeasurements.first().size() > 1);
        errorEstimationSampleSize = qCeil(reorderedMeasurements.first().size()*
                                          errorEstimationSampleRatio);

        boolList.clear();
        for(Index i = 0; i < errorEstimationSampleSize; i++) {
            /*
             * takeLast() removes elements from the list
             * new orderings will take the size of the last ordering into account
             */
            boolList.append(reorderedMeasurements.first().takeLast()->bit);
        }

        if(!isMaster) {
            emit sendData(PT02errorEstimationSendSample,
                          QVariant::fromValue(boolList));
        } else {
            BoolList compareBoolList = data.value<BoolList>();
            int size = boolList.size();
            Q_ASSERT(size == compareBoolList.size());
            errorCounter = 0;
            for(int i = 0; i < size; i++) {
                if(boolList.at(i) != compareBoolList.at(i))
                    errorCounter++;
            }
            emit sendData(PT02errorEstimationReport,
                          QVariant::fromValue<Index>(errorCounter));
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
        emit logMessage(QString("#01: Estimated error rate: %1 / %2 (%3%)").
                        arg(errorCounter).arg(boolList.size()).arg(error*100));
        boolList.clear();
        k1 = calculateInitialBlockSize(error);
        emit logMessage(QString("#02: resulting block size (k1): %1").arg(k1));
        emit logMessage(QString("Error Estimation finished"), Qt::green);

        emit logMessage("Initial Parity Check started", Qt::green);
        Index maximumBlockSize = k1*pow(2,runCount-1);
        Index removeBits = reorderedMeasurements.first().size() % maximumBlockSize;
        Measurements::iterator end = reorderedMeasurements.first().end();
        reorderedMeasurements.first().erase(end-removeBits,end);
        Q_ASSERT(reorderedMeasurements.first().size()/(SIndex)maximumBlockSize > 0);

        runIndex = 0;
        transferedBitsCounter = 0;
        this->incomingData(PT03prepareBlockParities, (uint)runIndex);

        return;
    }

    case PT03prepareBlockParities: {
        if(data.userType() == idIndexList) {
            reorderedMeasurements.append(reorderMeasurements(
                reorderedMeasurements.last(), data.value<IndexList>()));
            runIndex = reorderedMeasurements.size() - 1;

        } else if(data.type() == (QVariant::Type)QMetaType::UInt) {
            runIndex = data.toUInt();
        }
        Q_ASSERT(runIndex < reorderedMeasurements.size());

        blockSize = k1*pow(2,runIndex);
        binaryBlockSize = blockSize;
        blockCount = reorderedMeasurements.at(runIndex).size()/blockSize;
        emit logMessage(QString("in PT03prepareBlockParities, reorderedMeasurements.size = %1, runIndex = %2, blockSize = %3").
                        arg(reorderedMeasurements.size()).arg(runIndex).arg(blockSize));
        parities.clear();
        Index lastIndex = reorderedMeasurements.at(runIndex).size() - blockSize;
        for(Index index = 0; index <= lastIndex; index += blockSize) {
            parities.append(calculateParity(reorderedMeasurements.at(runIndex),
                                            index, blockSize));
        }

        if(isMaster) {
            emit sendData(PT04reportBlockParities,
                          QVariant::fromValue<BoolList>(parities));
        }

        return;

    }
    case PT04reportBlockParities: {
        if(!isMaster) {
            qint64 size = parities.size();
            Q_ASSERT(size > 0);
            BoolList compareParities = data.value<BoolList>();
            Q_ASSERT(compareParities.size() == size);
            corruptBlocks.clear();
            for(Index index = 0; index < size; index++) {
                if(compareParities.at(index) != parities.at(index))
                    corruptBlocks.append(index*blockSize);
            }
            emit sendData(PT04reportBlockParities,
                          QVariant::fromValue<IndexList>(corruptBlocks));
        } else {
            corruptBlocks = data.value<IndexList>();
        }
        transferedBitsCounter += corruptBlocks.size();
        if(blockSize == binaryBlockSize) {
            emit logMessage(QString("number of corrupt Blocks: %1/%2 (%3%)").
                            arg(corruptBlocks.size()).
                            arg(blockCount).
                            arg((double)corruptBlocks.size()/blockCount*100));
        }

        if(isMaster) {
            if(corruptBlocks.empty()) {
                runIndex++;
                // startBinary finished and there is another run
                // to return to (orders.size > 0)
                if((binaryBlockSize == 1) && (reorderedMeasurements.size() > 1) &&
                   (runIndex == reorderedMeasurements.size())) {
                    runIndex = 0;
                }
                if(runIndex < reorderedMeasurements.size()) {
                    emit sendData(PT03prepareBlockParities, (uint)runIndex);
                    this->incomingData(PT03prepareBlockParities);
                } else if(reorderedMeasurements.size() < runCount) {
                    QVariant data = QVariant::fromValue<IndexList>(
                        getRandomList(reorderedMeasurements.last().size()));
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
        parities.clear();
        if(isMaster || (!isMaster && binaryBlockSize > 1)) {
            foreach(Index index, corruptBlocks) {
                parities.append(calculateParity(reorderedMeasurements.at(runIndex),
                                                index, binaryBlockSize));
            }
        }
        if(isMaster) {
            emit sendData(PT05startBinary,
                          QVariant::fromValue<BoolList>(parities));
        } else {
            BoolList compareParities = data.value<BoolList>();
            Index size = compareParities.size();
            Q_ASSERT(corruptBlocks.size() == (SIndex)size);
            if(binaryBlockSize > 1)  {
                Q_ASSERT(parities.size() == (SIndex)size);
                for(Index i = 0; i < size; i++) {
                    if(compareParities.at(i) == parities.at(i)) {
                        // the 2nd half of the block must me corrupt
                        corruptBlocks.replace(i,corruptBlocks.at(i)+binaryBlockSize);
                    }
                }
            } else { // binaryBlockSize == 1: just toggle wrong bits
                for(Index i = 0; i < size; i++) {
                    // position in reorderedMeasurements: bit1 | bit2
                    // compareParities contains bit1 from Alice (master)
                    bool &bit1 = reorderedMeasurements.at(runIndex).
                            at(corruptBlocks.at(i))->bit;
                    bool &bit2 = reorderedMeasurements.at(runIndex).
                            at(corruptBlocks.at(i)+1)->bit;
                    if(compareParities.at(i) == bit1) {
                        bit2 = !bit2;
                        // corruptBlocks.replace(i, corruptBlocks.at(i)+1);
                    }
                    bit1 = compareParities.at(i);
                }
                corruptBlocks.clear();

            }
        }
        if(!isMaster) {
            emit sendData(PT04reportBlockParities,
                          QVariant::fromValue<IndexList>(corruptBlocks));
            transferedBitsCounter += corruptBlocks.size();
        }
        return;
    }
    case PT06finished: {
        if(!isMaster) {
            emit sendData(PT06finished);
        } else {
            this->sendData(PT07evaluation);
        }
        transferedBitsCounter += reorderedMeasurements.last().size()/
                      k1*(2-pow(2,1-runCount));
        emit logMessage(QString("bitCounter: %1 (%2%)").arg(transferedBitsCounter).
                        arg(100.0*transferedBitsCounter/reorderedMeasurements.last().size()));
        emit logMessage("fertig!");

        // An increasing security parameter s lowers the upper bound of Eve's
        // information on the key I <= 2^(-s)/ln(2); 2^(-7)/ln(2) = 0.01 bits
        const quint8 securityParameter = 7;
        qreal removeRatio = error*2
                          + (qreal)(transferedBitsCounter+securityParameter)/
                                        reorderedMeasurements.last().size();
        QByteArray finalKey = privacyAmplification(reorderedMeasurements.last(),
                                                   1-removeRatio);
        {
            QFile file(QString("outfile_%1.dat").arg(isMaster ? "alice" : "bob"));
            file.open(QIODevice::WriteOnly);
            QDataStream out(&file);
            out << finalKey;
            file.close();
        }
        int imageSize = qFloor(qSqrt(finalKey.size()/3));
        QImage image(imageSize, imageSize, QImage::Format_RGB888);
        int pos = 0;
        for(int x = 0; x < imageSize; x++) {
            for(int y = 0; y < imageSize; y++) {
                image.setPixel(x,y, qRgb(finalKey.at(pos),
                                         finalKey.at(pos+1),
                                         finalKey.at(pos+2)));
                pos+=3;
            }
        }
        image.save(QString("outfile_%1.png").arg(isMaster ? "alice" : "bob"));
        emit imageGenerated(image);
        return;
    }

    case PT07evaluation: {
        parities.clear();
        Index size = reorderedMeasurements.last().size();
        for(Index index = 0; index < size; index++)
            parities.append(calculateParity(reorderedMeasurements.last(), index, 1));
        if(!isMaster) {
            emit sendData(PT07evaluation, QVariant::fromValue<BoolList>(parities));
        } else {
            BoolList compareParities = data.value<BoolList>();
            Q_ASSERT(compareParities.size() ==(SIndex)size);
            Index errors = 0;
            for(Index index = 0; index < size; index++) {
                if(parities.at(index) != compareParities.at(index))
                    errors++;
            }
            emit logMessage(QString("Ergebnis: %1 (%2%) Bit-Fehler").arg(errors).
                            arg(100.0*errors/size));
        }

        emit finished();

        return;
    }

    }
}

void QKDProcessor::start()
{
    if(isMaster) { // this is Alice
        // #1 we kindly ask Bob for a list a received bits in first range from 0-quint32
        state = CS01;
        emit sendData(PT01sendReceivedList);
    } else { // this is Bob
        state = CSstarted;
    }
}
