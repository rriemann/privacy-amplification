#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include <QFile>
#include <QString>
#include <QByteArray>

class Authenticator : public QFile
{
    Q_OBJECT
public:
    explicit Authenticator(QString fileName, QObject *parent = 0);
    ~Authenticator();

    QByteArray* token (const QByteArray &data);
    bool authenticate(const QByteArray &data);
    
signals:
    
public slots:

    void setSecurityLevel(quint16 level);

private:

    quint16 level;
};

#endif // AUTHENTICATOR_H
