#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    setupUi(this);
    labelPort = new QLabel(this);
    QMainWindow::statusBar()->addPermanentWidget(labelPort);

    labelPort->setText(QString("Listening on port %1").arg(client.getServerPort()));
    push2Log(QString("Server started at port %1").arg(client.getServerPort()), Qt::yellow);

    connect(pushButtonConnect, SIGNAL(clicked()), this, SLOT(connectClicked()));
    connect(this, SIGNAL(initiateConnection(QString,int)), &client, SLOT(initiateConnection(QString,int)));

    connect(&client, SIGNAL(logMessage(QString,Qt::GlobalColor)), this, SLOT(push2Log(QString,Qt::GlobalColor)));
}

void MainWindow::connectClicked()
{
    emit initiateConnection(lineEditAdress->text(), lineEditPort->text().toInt());
}

void MainWindow::push2Log(QString entry, Qt::GlobalColor backgroundColor)
{
    QListWidgetItem* item = new QListWidgetItem(listWidgetLog);
    item->setBackgroundColor(QColor(backgroundColor));
    item->setText(entry);
    listWidgetLog->addItem(item);
}
