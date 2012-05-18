#ifndef CONNECTION_H
#define CONNECTION_H

#include <QTcpSocket>

class Connection : public QTcpSocket
{
    Q_OBJECT
public:
    explicit Connection(QObject *parent = 0);
    
signals:
    
public slots:
    void startHandShake();
    void wait4HandShake();

private slots:
    
};

#endif // CONNECTION_H
