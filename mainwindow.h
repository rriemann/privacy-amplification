#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"

#include <QLabel>

#include "client.h"

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);

signals:
    void initiateConnection(QString adress, int port, bool isMaster);

private:
    QLabel *labelPort;
    Client client;

private slots:
    void connectClicked();
    void push2Log(QString entry, Qt::GlobalColor backgroundColor = Qt::white);
};

#endif // MAINWINDOW_H
