#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"

#include <QLabel>

#include "client.h"
#include "qkdprocessor.h"

class File;

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    enum PackageType {
        PTtextMessage = 0,
        PThaveFile,
        PTask4File,
        PTprepareQKDProcessor
    };

    enum ConnectionState {
        CSready,
        CScontinueProcess,
        CSwait4Preparation,
        CSprocessing
    };

signals:
    void initiateConnection(QString adress, int port, bool isMaster);

private:
    QLabel *labelPort;
    Client client;
    QKDProcessor qkdp;

    File *file;
    bool connectionEstablished;
    bool isMaster;
    bool clientReady; // file loaded
    ConnectionState state;

private slots:
    void connectClicked();
    void logMessage(QString entry, Qt::GlobalColor backgroundColor = Qt::white);
    void establishedConnection(bool isMaster);
    void closedConnection();
    void incomingData(quint8 type, QVariant data);
    void sendTextMessage();
    inline void sendHaveFile() {client.sendData(PThaveFile, (bool)file);}

    void fileClose();
    void processStart();
    void processStop();

public slots:
    void fileOpen(QString fileName = QString());
};

#endif // MAINWINDOW_H
