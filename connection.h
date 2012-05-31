#ifndef CONNECTION_H
#define CONNECTION_H

#include <QTcpSocket>

class Connection : public QTcpSocket
{
    Q_OBJECT
public:
    explicit Connection(QObject *parent = 0);
    
signals:
    void logMessage(QString entry, Qt::GlobalColor backgroundColor = Qt::white);
    void receivedData(QVariant data);
    
public slots:
    void receiveData();
    void sendData(const QVariant& data);

private slots:

private:
    quint16 blockSize;
    static const QDataStream::Version streamVersion;
    
};

#endif // CONNECTION_H
