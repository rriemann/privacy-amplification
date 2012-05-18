#include <QtNetwork>

#include "server.h"
#include "connection.h"

#include "qdebug.h"

Server::Server(QObject *parent) :
    QTcpServer(parent)
{
    listen(QHostAddress::Any);
}

void Server::incomingConnection(int socketDescriptor)
{
    Connection *connection = new Connection(this);
    connection->setSocketDescriptor(socketDescriptor);
    emit newConnection(connection);
}
