#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"
#include "connection.h"

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = 0);
    int getServerPort();
    ~Client();
    
signals:
    void establishedConnection(bool isMaster);
    void closedConnection();
    void logMessage(QString entry, Qt::GlobalColor backgroundColor = Qt::white);
    void receiveData(quint8 type, QVariant data = QVariant());
    
public slots:
    void initiateConnection(QString host, int port, bool isMaster);
    void removeConnection();
    void sendData(quint8 type, QVariant data = QVariant());

private slots:
    void incomingConnection(Connection *incomingConnection);
    void startHandShake();
    void handleData(Connection::PackageType type, QVariant data);

private:

    enum ConnectionState {
        CSunconnected,
        CSinitiatedConnection,
        CSwait4Handshake,
        CSready
    };

    Server server;
    /*
    Connection *receivingConnection;
    Connection *sendingConnection;
    */
    Connection *connection;
    Connection *setupConnection(Connection *connection = 0);
    bool isMaster;

    ConnectionState status;
};

#endif // CLIENT_H
