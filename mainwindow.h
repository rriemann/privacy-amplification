#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"

#include <QLabel>

#include "client.h"

class File;

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void initiateConnection(QString adress, int port, bool isMaster);

private:
    QLabel *labelPort;
    Client client;

    File *file;
    bool connectionEstablished;
    bool isMaster;

private slots:
    void connectClicked();
    void push2Log(QString entry, Qt::GlobalColor backgroundColor = Qt::white);
    void establishedConnection(bool isMaster);
    void closedConnection();

    void fileClose();
    void processStart();
    void processStop();

public slots:
    void fileOpen(QString fileName = QString());
};

#endif // MAINWINDOW_H
