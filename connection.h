#ifndef CONNECTION_H
#define CONNECTION_H

#include <QTcpSocket>

class Connection : public QTcpSocket
{
    Q_OBJECT
public:

    enum PackageType {
        PTroleDefinition,
        PTchatMessage,
        PThasFile,
        PTcustomData
    };

    explicit Connection(QObject *parent = 0);
    
signals:
    void logMessage(QString entry, Qt::GlobalColor backgroundColor = Qt::white);
    void receivedData(Connection::PackageType type, QVariant data);
    
public slots:
    void receiveData();
    void sendData(const PackageType type, const QVariant& data);

private slots:

private:
    qint64 blockSize;
    static const QDataStream::Version streamVersion;
    
};

#endif // CONNECTION_H
