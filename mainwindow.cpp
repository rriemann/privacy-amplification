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
    connect(this, SIGNAL(initiateConnection(QString,int,bool)), &client, SLOT(initiateConnection(QString,int,bool)));
    connect(&client, SIGNAL(receivedRole(bool)), checkBoxMaster, SLOT(setChecked(bool)));

    connect(&client, SIGNAL(logMessage(QString,Qt::GlobalColor)), this, SLOT(push2Log(QString,Qt::GlobalColor)));

    connect(&client, SIGNAL(establishedConnection()), this, SLOT(establishedConnection()));
    connect(&client, SIGNAL(closedConnection()), this, SLOT(closedConnection()));
}

void MainWindow::connectClicked()
{
    lineEditAdress->setEnabled(false);
    lineEditPort->setEnabled(false);
    checkBoxMaster->setEnabled(false);
    emit initiateConnection(lineEditAdress->text(), lineEditPort->text().toInt(), checkBoxMaster->isChecked());
}

void MainWindow::push2Log(QString entry, Qt::GlobalColor backgroundColor)
{
    QListWidgetItem* item = new QListWidgetItem(listWidgetLog);
    item->setBackgroundColor(QColor(backgroundColor));
    item->setText(entry);
    listWidgetLog->addItem(item);
    listWidgetLog->scrollToBottom();
}

void MainWindow::establishedConnection()
{
    lineEditAdress->setEnabled(false);
    lineEditPort->setEnabled(false);
    checkBoxMaster->setEnabled(false);
}

void MainWindow::closedConnection()
{
    lineEditAdress->setEnabled(true);
    lineEditPort->setEnabled(true);
    checkBoxMaster->setEnabled(true);
}
