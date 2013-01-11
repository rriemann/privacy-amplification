#ifndef QKDPROCESSOR_H
#define QKDPROCESSOR_H

#include <QObject>
#include <QVariant>
#include <QImage>

#include "measurement.h"

typedef quint32 Index;
typedef qint64 SIndex;
typedef QList<Index> IndexList;
typedef QPair<Index,bool> IndexBoolPair;
typedef QList<IndexBoolPair> IndexBoolPairList;
typedef QList<bool> BoolList;

class QKDProcessor : public QObject
{
    Q_OBJECT

public:
    explicit QKDProcessor(QObject *parent = 0);
    ~QKDProcessor();

    enum PackageType {
        PT01sendReceivedList = 50,
        PT01sendRemainingList,
        PT02errorEstimationSendSample,
        PT02errorEstimationReport,
        PT03prepareBlockParities,
        PT04reportBlockParities,
        PT05startBinary,
        PT06finished,
        PT07evaluation
    };

private:
    quint16 calculateInitialBlockSize(qreal errorProbability);
    bool calculateParity(const Measurements measurements, const Index index, const quint16 &size) const;
    inline IndexList getOrderedList(Index range);
    IndexList getRandomList(Index range);
    static Measurements reorderMeasurements(const Measurements measurements, const IndexList order);
    void clearMeasurements();
    static QByteArray privacyAmplification(const Measurements measurements, const qreal ratio);

    Measurements* measurements;
    bool isMaster;
    const static quint8 runCount = 4;
    const static qreal errorEstimationSampleRatio = 0.02;
    IndexBoolPairList list;
    IndexList remainingList;
    Index errorEstimationSampleSize;
    BoolList boolList;
    Index errorCounter;
    qreal error;
    quint16 k1; // initial block size
    BoolList parities;
    IndexList corruptBlocks;
    QList<Measurements> reorderedMeasurements;
    quint16 blockSize;
    quint16 binaryBlockSize;
    Index blockCount;
    quint8 runIndex;
    Index transferedBitsCounter; // number of bits known to eve
    static const int idIndexList;
    
signals:
    void sendData(quint8 type, QVariant data = QVariant());
    void logMessage(QString entry, Qt::GlobalColor backgroundColor = Qt::white);
    void finished();
    void imageGenerated(QImage image);
    
public slots:
    void start();
    void incomingData(quint8 type, QVariant data = QVariant());

    void setMeasurements(Measurements* measurements);
    void setMaster(bool isMaster);
};

Q_DECLARE_METATYPE(IndexList)
Q_DECLARE_METATYPE(IndexBoolPair)
Q_DECLARE_METATYPE(IndexBoolPairList)
Q_DECLARE_METATYPE(BoolList)
Q_DECLARE_METATYPE(Qt::GlobalColor)

#endif // QKDPROCESSOR_H
