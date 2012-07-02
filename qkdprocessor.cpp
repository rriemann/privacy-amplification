#include "qkdprocessor.h"

#include <limits>
#include <algorithm>
using std::max;
using std::random_shuffle;
#include <qmath.h>
#include <QTime>
#include <qdebug.h>

Q_DECLARE_METATYPE(IndexList)
static int idIndexList = qRegisterMetaType<IndexList>();
static int id6 = qRegisterMetaTypeStreamOperators<IndexList>();

typedef QPair<Index,bool> IndexBoolPair;
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

void QKDProcessor::clearMeasurements()
{
    if(measurements) {
        qDeleteAll(*measurements);
        delete measurements;
    }
}

void QKDProcessor::setMeasurements(Measurements *measurements)
{
    clearMeasurements();
    this->measurements = measurements;
}

QKDProcessor::~QKDProcessor()
{
    clearMeasurements();
}

void QKDProcessor::siftMeasurements(IndexList list)
{
    /*
     * 8 bytes in 1 blocks are definitely lost in loss record 140 of 861
     * in QKDProcessor::siftMeasurements(QList&lt;unsigned int&gt;) in qkdprocessor.cpp:54
     * 1: operator new(unsigned long) in /usr/lib64/valgrind/vgpreload_memcheck-amd64-linux.so
     *   |
     *   v
     */
    Measurements *siftedMeasurements = new Measurements;
    foreach(Index index, list) {
        siftedMeasurements->append(new Measurement(*(measurements->at(index))));
    }

    qDeleteAll(*measurements);

    /*
    10,275,208 (8 direct, 10,275,200 indirect) bytes in 1 blocks are definitely lost in loss record 491 of 491
      in QKDProcessor::siftMeasurements(QList&lt;unsigned int&gt;) in qkdprocessor.cpp:46
      1: operator new(unsigned long) in /usr/lib64/valgrind/vgpreload_memcheck-amd64-linux.so
      2: QKDProcessor::siftMeasurements(QList&lt;unsigned int&gt;) in <a href="file:///home/rriemann/Documents/Development/C++/Qt4/privacy-amplification/qkdprocessor.cpp:46" >qkdprocessor.cpp:46</a>
      3: QKDProcessor::incomingData(unsigned char, QVariant) in <a href="file:///home/rriemann/Documents/Development/C++/Qt4/privacy-amplification/qkdprocessor.cpp:181" >qkdprocessor.cpp:181</a>
      4: MainWindow::incomingData(unsigned char, QVariant) in <a href="file:///home/rriemann/Documents/Development/C++/Qt4/privacy-amplification/mainwindow.cpp:139" >mainwindow.cpp:139</a>
          |
          v
    */
    /*
    Index deleteIndex = 0;
    foreach(Index index, list) {
        // delete Measurement Objects with indexes not in list
        for(;deleteIndex < index; deleteIndex++)
            delete measurements->at(deleteIndex);
        // if Index is in list: copy pointer to new Measurement list
        siftedMeasurements->append(measurements->at(index));
        // make sure to not delete the indexed object afterwards
        deleteIndex = index+1;
    }
    // delete remaining elements between last index in list and end of list
    for(;(SIndex)deleteIndex < measurements->size(); deleteIndex++)
        delete measurements->at(deleteIndex);

    */

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
    static IndexList corruptBlocks;
    static QList<Measurements> reorderedMeasurements;
    static quint16 blockSize;
    static quint16 binaryBlockSize;
    static Index blockCount;
    static quint8 runIndex;
    static IndexList randomSample;

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
            foreach(pair, list) {
                if(measurements->at(pair.first)->base == pair.second)
                    remainingList.append(pair.first);
            }
            emit sendData(PT01sendRemainingList,
                          QVariant::fromValue(remainingList));
        }

        return;
    }
    case PT01sendRemainingList: {
        if(isMaster) {
        } else {
            remainingList = data.value<IndexList>();
            emit sendData(PT01sendRemainingList);
        }
        emit logMessage(QString("#02: Remaining measurements after sifting (same base): %1 (%2%)").
                        arg(remainingList.size()).
                        arg((double)remainingList.size()*100/list.size()));
        siftMeasurements(remainingList);

        {
            int bit = 0;
            int base = 0;
            foreach(Measurement *measurement, *this->measurements) {
                bit += measurement->bit;
                base += measurement->base;
            }

            emit logMessage(QString("Stats ratio: bit = %1%, base = %2%").
                            arg((double)bit/measurements->size()).
                            arg((double)base/measurements->size()));
        }

        list.clear();
        remainingList.clear();
        emit logMessage(QString("Sifting Procedure finished"), Qt::green);
        if(isMaster) {
            errorEstimationSampleSize = qCeil(measurements->size()*0.01);
            randomSample.clear();
            randomSample = getRandomList(measurements->size());
            randomSample.erase(randomSample.begin()+errorEstimationSampleSize,
                               randomSample.end());
            qSort(randomSample.begin(),randomSample.end());

            // invoke Bob to send BitSample
            emit sendData(PT02errorEstimationSendSample,
                          QVariant::fromValue<IndexList>(randomSample));
        }
        return;
    }
    // Error Estimation
    case PT02errorEstimationSendSample: {
        emit logMessage(QString("Error Estimation started"), Qt::green);
        if(!isMaster) {
            randomSample = data.value<IndexList>();
        }

        Index size = randomSample.size();
        Q_ASSERT(measurements->size() >= (SIndex)size);
        Measurement *measurement;
        boolList.clear();
        // http://qt-project.org/doc/qt-4.8/qlistiterator.html#details
        QListIterator<Index> randomSampleIterator(randomSample);
        randomSampleIterator.toBack();
        while(randomSampleIterator.hasPrevious() ) {
            // TODO: loop using takeAt(...) is quite time consuming :/
            measurement = measurements->takeAt(randomSampleIterator.previous());
            boolList.append(measurement->bit);
            delete measurement;
        }

        if(!isMaster) {
            emit sendData(PT02errorEstimationSendSample,
                          QVariant::fromValue(boolList));
        } else {
            BoolList compareBoolList = data.value<BoolList>();
            errorCounter = 0;
            for(Index index = 0; index < size; index++) {
                if(boolList.at(index) != compareBoolList.at(index))
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
        Index removeBits = measurements->size() % maximumBlockSize;
        for(quint16 i = 0; i < removeBits; i++)
            delete measurements->takeLast();
        Q_ASSERT(measurements->size()/(SIndex)maximumBlockSize > 0);

        reorderedMeasurements.clear();

        runIndex = 0;
        reorderedMeasurements.append(*measurements);
        this->incomingData(PT03prepareBlockParities, (uint)runIndex);

        return;
    }

    case PT03prepareBlockParities: {
        if(data.userType() == idIndexList) {
            reorderedMeasurements.append(reorderMeasurements(data.value<IndexList>()));
            runIndex = reorderedMeasurements.size() - 1;

        } else if(data.type() == (QVariant::Type)QMetaType::UInt) {
            runIndex = data.toUInt();
        }
        Q_ASSERT(runIndex < reorderedMeasurements.size());

        blockSize = k1*pow(2,runIndex);
        binaryBlockSize = blockSize;
        blockCount = measurements->size()/blockSize;
        emit logMessage(QString("in PT03prepareBlockParities, reorderedMeasurements.size = %1, runIndex = %2, blockSize = %3").
                        arg(reorderedMeasurements.size()).arg(runIndex).arg(blockSize));
        //emit logMessage(QString("ki: %1").arg(blockSize));
        /*
        orders.append(data.value<IndexList>());
        emit logMessage(QString("elements in orders.last: %1").
                        arg(orders.last().size()));
        reorderedMeasurements.append(reorderMeasurements(orders.last()));
        */
        parities.clear();
        Index lastIndex = measurements->size() - blockSize;
        for(Index index = 0; index <= lastIndex; index += blockSize) {
            parities.append(calculateParity(reorderedMeasurements.at(runIndex),
                                            index, blockSize));
        }
        // emit logMessage(QString("elements in parities: %1").arg(parities.size()));

        if(isMaster) {
            emit sendData(PT04reportBlockParities,
                          QVariant::fromValue<BoolList>(parities));
        }

        return;

    }
    case PT04reportBlockParities: {
        // emit logMessage("in PT04reportBlockParities", Qt::green);
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
                    tmp += QString("%1 ").arg(j,2,10,QLatin1Char(' '));
                emit logMessage(tmp);
                tmp = "A: ";
                foreach(bool p, compareParities)
                    tmp += QString("%1 ").arg((int)p,2,10,QLatin1Char(' '));
                emit logMessage(tmp);
                tmp = "B: ";
                foreach(bool p, parities)
                    tmp += QString("%1 ").arg((int)p,2,10,QLatin1Char(' '));
                emit logMessage(tmp);

            }
            */
            for(Index index = 0; index < size; index++) {
                if(compareParities.at(index) != parities.at(index))
                    corruptBlocks.append(index*blockSize);
            }
            emit sendData(PT04reportBlockParities,
                          QVariant::fromValue<IndexList>(corruptBlocks));
        } else {
            corruptBlocks = data.value<IndexList>();
        }
        if(blockSize == binaryBlockSize) {
            emit logMessage(QString("number of corrupt Blocks: %1/%2 (%3%)").
                            arg(corruptBlocks.size()).
                            arg(blockCount).
                            arg((double)corruptBlocks.size()/blockCount*100));
        }
        /*
        {
            for(Index j = 0; (SIndex)j < corruptBlocks.size(); j++) {
                Index index = corruptBlocks.at(j);
                QString strIndex = QString("%1: ").arg(index, 3, 10, QLatin1Char(' '));
                Index end = index+binaryBlockSize;
                for(Index i = index; i < end; i++)
                    strIndex += QString("%1 ").arg((int)(reorderedMeasurements.last().at(i)->bit));
                emit logMessage(strIndex);
            }
        }
        */
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
        // emit logMessage(QString("in PT05startBinary with binaryBlockSize = %1").arg(binaryBlockSize));
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
        }
        return;
    }
    case PT06finished: {
        if(!isMaster) {
            emit sendData(PT06finished);
        } else {
            this->sendData(PT07evaluation);
        }
        emit logMessage("fertig!");
        return;
    }

    case PT07evaluation: {
        parities.clear();
        Index size = measurements->size();
        for(Index index = 0; index < size; index++)
            parities.append(calculateParity(*measurements, index, 1));
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
        return;
    }

    }
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
