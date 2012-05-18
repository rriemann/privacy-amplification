#include "client.h"
#include "mainwindow.h"

#include "connection.h"

Client::Client(QObject *parent) :
    QObject(parent)
{
    connection = 0;
    connect(&server, SIGNAL(newConnection(Connection*)), this, SLOT(incomingConnection(Connection*)));
}

Client::~Client()
{
    removeConnection();
}

int Client::getServerPort()
{
    return server.serverPort();
}

Connection *Client::setupConnection(Connection *connection)
{
    if(!connection)
        connection = new Connection(this);
    connect(connection, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(removeConnection()));
    connect(connection, SIGNAL(disconnected()), this, SLOT(removeConnection()));

    return connection;
}

void Client::incomingConnection(Connection *incomingConnection)
{
    if(!connection) {
        connection = setupConnection(incomingConnection);
        emit logMessage(QString("Waiting for Handshake from %1:%2").arg(connection->peerAddress().toString()).arg(connection->peerPort()), Qt::yellow);
        connect(connection, SIGNAL(readyRead()), connection, SLOT(wait4HandShake()));
    } else {
        qDebug() << "dismiss incoming connection from " << incomingConnection->peerAddress() << incomingConnection->peerPort();
        incomingConnection->deleteLater();
    }
}

void Client::initiateConnection(QString host, int port)
{
    if(!connection) {
        connection = setupConnection();
        connection->connectToHost(host, port);
        connect(connection, SIGNAL(connected()), connection, SLOT(startHandShake()));
    }
}

void Client::removeConnection()
{
    if(connection) {
        emit logMessage(QString("Removing Connection... (%1)").arg(connection->errorString()), Qt::yellow);
        connection->deleteLater();
    }
    connection = 0;
}
