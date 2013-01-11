#ifndef DEMOMODE_H
#define DEMOMODE_H

#include <QObject>
#include <QString>
#include <QThread>
#include "qkdprocessor.h"

class DemoMode : public QObject
{
    Q_OBJECT
public:
    explicit DemoMode(QString fileName, QObject *parent = 0);
    
signals:
    void finished();
    
public slots:
    void logMessage(QString entry);
    void logMessageAlice(QString entry);
    void logMessageBob(QString entry);

private:


    QKDProcessor alice;
    QKDProcessor bob;
    QThread aliceThread;
    QThread bobThread;
    
};

#endif // DEMOMODE_H
