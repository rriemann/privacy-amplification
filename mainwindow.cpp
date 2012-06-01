#include "mainwindow.h"

#include "file.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), file(0), connectionEstablished(false), clientReady(false), state(CSready)
{
    setupUi(this);
    labelPort = new QLabel(this);
    QMainWindow::statusBar()->addPermanentWidget(labelPort);

    labelPort->setText(QString("Listening on port %1").arg(client.getServerPort()));
    logMessage(QString("Server started at port %1").arg(client.getServerPort()), Qt::yellow);

    connect(this, SIGNAL(initiateConnection(QString,int,bool)), &client, SLOT(initiateConnection(QString,int,bool)));
    connect(&client, SIGNAL(receivedRole(bool)), checkBoxMaster, SLOT(setChecked(bool)));

    connect(&client, SIGNAL(logMessage(QString,Qt::GlobalColor)), this, SLOT(logMessage(QString,Qt::GlobalColor)));

    connect(&client, SIGNAL(establishedConnection(bool)), this, SLOT(establishedConnection(bool)));
    connect(&client, SIGNAL(closedConnection()), this, SLOT(closedConnection()));
    connect(&client, SIGNAL(receiveData(quint8,QVariant)), this, SLOT(incomingData(quint8,QVariant)));


    connect(actionConnect, SIGNAL(triggered()), this, SLOT(connectClicked()));
    connect(actionDisconnect, SIGNAL(triggered()), &client, SLOT(removeConnection()));
    connect(actionClose, SIGNAL(triggered()), this, SLOT(fileClose()));
    connect(actionOpen, SIGNAL(triggered()), this, SLOT(fileOpen()));
    connect(actionStart, SIGNAL(triggered()), this, SLOT(processStart()));
    connect(actionReset, SIGNAL(triggered()), this, SLOT(processStop()));

    connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(sendTextMessage()));
    connect(lineEditAdress, SIGNAL(returnPressed()), this, SLOT(connectClicked()));
    connect(lineEditPort, SIGNAL(returnPressed()), this, SLOT(connectClicked()));
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

    actionClose->setEnabled(false);
    actionConnect->setEnabled(false);


    emit initiateConnection(lineEditAdress->text(), lineEditPort->text().toInt(), checkBoxMaster->isChecked());
}

void MainWindow::logMessage(QString entry, Qt::GlobalColor backgroundColor)
{
    QListWidgetItem* item = new QListWidgetItem(listWidgetLog);
    item->setBackgroundColor(QColor(backgroundColor));
    item->setText(QTime::currentTime().toString("hh:mm:ss> ") + entry);
    listWidgetLog->addItem(item);
    listWidgetLog->scrollToBottom();
}

void MainWindow::establishedConnection(bool isMaster)
{
    connectionEstablished = true;

    actionDisconnect->setEnabled(true);
    actionConnect->setEnabled(false);

    lineEditAdress->setEnabled(false);
    lineEditPort->setEnabled(false);
    checkBoxMaster->setEnabled(false);

    this->isMaster = isMaster;
    checkBoxMaster->setChecked(isMaster);

    if(file) {
        actionStart->setEnabled(isMaster);
        actionReset->setEnabled(true);

        actionClose->setEnabled(false);
    }
}

void MainWindow::closedConnection()
{
    connectionEstablished = false;
    clientReady = false;
    state = CSready;
    actionDisconnect->setEnabled(false);
    actionConnect->setEnabled(true);

    if(file)
        actionClose->setEnabled(true);

    processStop();
    actionStart->setEnabled(false);
    actionReset->setEnabled(false);

    lineEditAdress->setEnabled(true);
    lineEditPort->setEnabled(true);
    checkBoxMaster->setEnabled(true);
}

void MainWindow::incomingData(quint8 type, QVariant data)
{
    switch((PackageType)type) {
    case PTtextMessage: {
        logMessage(data.toString(), Qt::magenta);
    }
        break;
    case PThaveFile: {
        clientReady = data.toBool();
        logMessage(QString("client has %1file.").arg(clientReady ? QString() : QString("not ")), Qt::yellow);
        if(state == CScontinueProcess) {
            processStart();
        }

    }
        break;
    case PTask4File: {
        sendHaveFile();
    }
        break;
    }
}

void MainWindow::sendTextMessage()
{
    client.sendData(PTtextMessage, lineEdit->text());
    lineEdit->setText(QString());
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
        if(fileName.isNull())
            return;
    }

    file = new File(fileName, this);
    if(!file->open(QIODevice::ReadOnly)) {
        logMessage(QString("Unable to open File \"%1\" in readonly-mode.").arg(fileName), Qt::lightGray);
        delete file;
        file = 0;
        return;
    }

    logMessage(QString("Open file \"%1\" successfully in read-only mode.").arg(fileName), Qt::lightGray);

    actionOpen->setEnabled(false);
    actionClose->setEnabled(true);

    actionConnect->setEnabled(true);

    if(connectionEstablished) {
        sendHaveFile();

        actionStart->setEnabled(isMaster);
    }
}

void MainWindow::fileClose()
{
    processStop();

    if(file) {
        file->close();
        delete file;
        file = 0;

        logMessage(QString("File has been closed."), Qt::lightGray);
    }

    actionOpen->setEnabled(true);
    actionClose->setEnabled(false);

    actionConnect->setEnabled(false);

    if(connectionEstablished)
        sendHaveFile();
}

void MainWindow::processStart()
{
    actionStart->setEnabled(false);
    if(!clientReady) {
        client.sendData(PTask4File);
        state = CScontinueProcess;
        return;
    }
    logMessage("here we go");
    state = CSprocessing;
}

void MainWindow::processStop()
{
    actionStart->setEnabled(true);
    state = CSready;
}
