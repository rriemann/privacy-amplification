#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = 0);
    int getServerPort();
    ~Client();
    
signals:
    void establishedConnection();
    void logMessage(QString entry, Qt::GlobalColor backgroundColor = Qt::white);
    
public slots:
    void initiateConnection(QString host, int port);

private slots:
    void incomingConnection(Connection *incomingConnection);
    void removeConnection();

private:

    Server server;
    /*
    Connection *receivingConnection;
    Connection *sendingConnection;
    */
    Connection *connection;
    Connection *setupConnection(Connection *connection = 0);
};

#endif // CLIENT_H
