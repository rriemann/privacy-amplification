#ifndef QKDPROCESSOR_H
#define QKDPROCESSOR_H

#include <QObject>
#include <QVariant>

#include "measurement.h"

typedef quint32 Index;
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
        PT03blockParities
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
    void siftMeasurements(IndexList list);
    quint16 calculateInitialBlockSize(qreal errorProbability);
    bool calculateParity(Measurements::const_iterator begin, quint16 size);
    IndexList getRandomList(Index range);
    
signals:
    void sendData(quint8 type, QVariant data = QVariant());
    void logMessage(QString entry, Qt::GlobalColor backgroundColor = Qt::white);
    
public slots:
    void start(bool isMaster);
    void incomingData(quint8 type, QVariant data);

    void setMeasurements(Measurements* measurements);
    
};

#endif // QKDPROCESSOR_H
