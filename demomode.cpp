#include "demomode.h"
#include <qdebug.h>
#include "file.h"
#include <QApplication>

DemoMode::DemoMode(QString fileName, QObject *parent) :
    QObject(parent)
{

    File file(fileName, this);
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug() << "file could not be opened";
        exit(1);
    }

    connect(&alice, SIGNAL(sendData(quint8,QVariant)), &bob,   SLOT(incomingData(quint8,QVariant)));
    connect(&bob,   SIGNAL(sendData(quint8,QVariant)), &alice, SLOT(incomingData(quint8,QVariant)));

    qRegisterMetaType<Qt::GlobalColor>();
    connect(&alice, SIGNAL(logMessage(QString)), this, SLOT(logMessageAlice(QString)));
    connect(&bob,   SIGNAL(logMessage(QString)), this, SLOT(logMessageBob(QString)));

    alice.setMaster(true);
    alice.setMeasurements(file.getMeasurements(true));

    bob.setMaster(false);
    bob.setMeasurements(file.getMeasurements(false));

    file.close();

    alice.moveToThread(&aliceThread);

    bob.moveToThread(&bobThread);

    connect(&aliceThread, SIGNAL(started()), &alice, SLOT(start()));
    connect(&aliceThread, SIGNAL(finished()), this, SIGNAL(finished()));

    connect(&alice, SIGNAL(finished()), &aliceThread, SLOT(quit()));
    connect(&bob, SIGNAL(finished()), &bobThread, SLOT(quit()));

    bobThread.start();
    aliceThread.start();
}

void DemoMode::logMessage(QString entry)
{
    qDebug("%s", qPrintable(entry));
}

void DemoMode::logMessageAlice(QString entry)
{
    static const QString alice("A: ");
    logMessage(alice + entry);
}

void DemoMode::logMessageBob(QString entry)
{
    static const QString bob("B: ");
    logMessage(bob + entry);
}
