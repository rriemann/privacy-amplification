#include "mainwindow.h"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), file(0)
{
    setupUi(this);
    labelPort = new QLabel(this);
    QMainWindow::statusBar()->addPermanentWidget(labelPort);

    labelPort->setText(QString("Listening on port %1").arg(client.getServerPort()));
    push2Log(QString("Server started at port %1").arg(client.getServerPort()), Qt::yellow);

    connect(actionConnect, SIGNAL(triggered()), this, SLOT(connectClicked()));
    connect(this, SIGNAL(initiateConnection(QString,int,bool)), &client, SLOT(initiateConnection(QString,int,bool)));
    connect(&client, SIGNAL(receivedRole(bool)), checkBoxMaster, SLOT(setChecked(bool)));

    connect(&client, SIGNAL(logMessage(QString,Qt::GlobalColor)), this, SLOT(push2Log(QString,Qt::GlobalColor)));

    connect(&client, SIGNAL(establishedConnection()), this, SLOT(establishedConnection()));
    connect(&client, SIGNAL(closedConnection()), this, SLOT(closedConnection()));


    connect(actionClose, SIGNAL(triggered()), this, SLOT(fileClose()));
    connect(actionOpen, SIGNAL(triggered()), this, SLOT(fileOpen()));
}

MainWindow::~MainWindow()
{
    fileClose();
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

void MainWindow::fileOpen(QString fileName)
{
    if(file) {
        if(QMessageBox::Ok == QMessageBox::warning(this, "File Already Open", "There is already a file opened. Do you want to close this now?", QMessageBox::Ok, QMessageBox::Abort)) {
            fileClose();
        } else {
            return;
        }
    }

    if(fileName.isNull()) {
        fileName = QFileDialog::getOpenFileName(this, tr("Open Measurement File"), fileName, tr("Measurement Files (*.bin)"));
    }

    push2Log(QString("Try to open file \"%1\".").arg(fileName), Qt::lightGray);

    file = new QFile(fileName, this);
    if(!file->open(QIODevice::ReadOnly)) {
        push2Log(QString("Unable to open File \"%1\" in readonly-mode.").arg(fileName), Qt::lightGray);
    }
}

void MainWindow::fileClose()
{
    if(file) {
        file->close();
        delete file;
        file = 0;
    }
}
