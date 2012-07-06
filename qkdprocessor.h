#ifndef QKDPROCESSOR_H
#define QKDPROCESSOR_H

#include <QObject>
#include <QVariant>

#include "measurement.h"

typedef quint32 Index;
typedef qint64 SIndex;
typedef QList<Index> IndexList;

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

    enum ConnectionState {
        CSready,
        CSstarted,
        CS01
    };

private:
    Measurements* measurements;
    bool isMaster;
    ConnectionState state;
    quint16 calculateInitialBlockSize(qreal errorProbability);
    bool calculateParity(const Measurements measurements, const Index index, const quint16 &size) const;
    inline IndexList getOrderedList(Index range);
    IndexList getRandomList(Index range);
    static Measurements reorderMeasurements(const Measurements measurements, const IndexList order);
    const static quint8 runCount = 4;
    const static qreal errorEstimationSampleRatio = 0.02;
    void clearMeasurements();
    static QByteArray privacyAmplification(const Measurements measurements, const qreal ratio);
    
signals:
    void sendData(quint8 type, QVariant data = QVariant());
    void logMessage(QString entry, Qt::GlobalColor backgroundColor = Qt::white);
    
public slots:
    void start(bool isMaster);
    void incomingData(quint8 type, QVariant data = QVariant());

    void setMeasurements(Measurements* measurements);
    
};

#endif // QKDPROCESSOR_H
