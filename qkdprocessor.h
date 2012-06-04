#ifndef QKDPROCESSOR_H
#define QKDPROCESSOR_H

#include <QObject>
#include <QVariant>

#include "measurement.h"

class QKDProcessor : public QObject
{
    Q_OBJECT


public:
    explicit QKDProcessor(QObject *parent = 0);
    ~QKDProcessor();

    enum PackageType {
        PT01sendReceivedList = 50
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
    void incomingDataAlice(quint8 type, QVariant data);
    void incomingDataBob(quint8 type, QVariant data);
    
signals:
    void sendData(quint8 type, QVariant data = QVariant());
    void logMessage(QString entry, Qt::GlobalColor backgroundColor = Qt::white);
    
public slots:
    void start(bool isMaster);
    void incomingData(quint8 type, QVariant data);

    void setMeasurements(Measurements* measurements);
    
};

#endif // QKDPROCESSOR_H
