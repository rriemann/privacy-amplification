#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"

#include <QLabel>

#include "client.h"

class QFile;

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

    QFile *file;

private slots:
    void connectClicked();
    void push2Log(QString entry, Qt::GlobalColor backgroundColor = Qt::white);
    void establishedConnection();
    void closedConnection();

    void fileOpen(QString fileName = QString());
    void fileClose();
};

#endif // MAINWINDOW_H
