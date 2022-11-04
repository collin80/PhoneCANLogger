#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDateTime>
#include <QDebug>
#include <QNetworkDatagram>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    tcpClient = nullptr;
    rx_state = IDLE;
    loggingActive = false;
    connectedToDevice = false;
    capturedFrames = 0;
    writingFile = nullptr;

    rxBroadcastGVRET = new QUdpSocket(this);
    //Need to make sure it tries to share the address in case there are
    //multiple instances of SavvyCAN running.
    rxBroadcastGVRET->bind(QHostAddress::AnyIPv4, 17222, QAbstractSocket::ShareAddress);
    connect(rxBroadcastGVRET, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
    connect(ui->btnConnect, &QAbstractButton::clicked, this, &MainWindow::clickedConnect);
    connect(ui->btnStartLog, &QAbstractButton::clicked, this, &MainWindow::loggingButtonClicked);
    connect(&updateTimer, &QTimer::timeout, this, &MainWindow::tickedTimer);

    updateTimer.setInterval(1000);
    updateTimer.start();
}

MainWindow::~MainWindow()
{
    if (writingFile) writingFile->close();
    delete writingFile;
    delete ui;
}

void MainWindow::readPendingDatagrams()
{
    //qDebug() << "Got a UDP frame!";
    while (rxBroadcastGVRET->hasPendingDatagrams()) {
        QNetworkDatagram datagram = rxBroadcastGVRET->receiveDatagram();
        if (!remoteDeviceIPGVRET.contains(datagram.senderAddress().toString()))
        {
            remoteDeviceIPGVRET.append(datagram.senderAddress().toString());
            ui->listIPAddrs->addItem(datagram.senderAddress().toString());
            //qDebug() << "Add new remote IP " << datagram.senderAddress().toString();
        }
    }
}

void MainWindow::clickedConnect()
{
    if (!connectedToDevice)
    {
        QString IPAddr = ui->listIPAddrs->currentItem()->text();
        if(tcpClient)
            disconnectDevice();

        // /*
        //sendDebug("TCP Connection to a GVRET device");
        tcpClient = new QTcpSocket();
        tcpClient->connectToHost(IPAddr, 23);
        connect(tcpClient, SIGNAL(readyRead()), this, SLOT(readSerialData()));
        connect(tcpClient, SIGNAL(connected()), this, SLOT(deviceConnected()));
        //sendDebug("Created TCP Socket");
    }
    else
    {
        if (tcpClient) disconnectDevice();
        ui->btnConnect->setText("Connect");
    }
}

void MainWindow::readSerialData()
{
    QByteArray data;
    unsigned char c;
    QString debugBuild;

    if (tcpClient) data = tcpClient->readAll();

    //sendDebug("Got data from serial. Len = " % QString::number(data.length()));
    for (int i = 0; i < data.length(); i++)
    {
        c = data.at(i);
        //qDebug() << c << "    " << QString::number(c, 16) << "     " << QString(c);
        //debugBuild = debugBuild % QString::number(c, 16).rightJustified(2,'0') % " ";
        procRXChar(c);
    }
    //qDebug() << debugBuild;

}

void MainWindow::deviceConnected()
{
    //sendDebug("Connecting to GVRET Device!");
    QByteArray output;
    output.append((char)0xE7); //this puts the device into binary comm mode
    output.append((char)0xE7);

    output.append((char)0xF1); //yet another command
    output.append((char)0x09); //comm validation command

    if (tcpClient) tcpClient->write(output);

    connectedToDevice = true;
    ui->btnConnect->setText("Disconnect from Device");
}

void MainWindow::disconnectDevice() {
    if (tcpClient != nullptr)
    {
        if (tcpClient->isOpen())
        {
            tcpClient->close();
        }
        tcpClient->disconnect();
        delete tcpClient;
        tcpClient = nullptr;
    }
}

void MainWindow::procRXChar(unsigned char c)
{
    QByteArray output;

    switch (rx_state)
    {
    case IDLE:
        if (c == 0xF1) rx_state = GET_COMMAND;
        break;
    case GET_COMMAND:
        switch (c)
        {
        case 0: //receiving a can frame
            rx_state = BUILD_CAN_FRAME;
            rx_step = 0;
            break;
        case 1: //time sync
            rx_state = IDLE;
            rx_step = 0;
            break;
        case 2: //process a return reply for digital input states.
            rx_state = IDLE;
            rx_step = 0;
            break;
        case 3: //process a return reply for analog inputs
            rx_state = IDLE;
            break;
        case 4: //we set digital outputs we don't accept replies so nothing here.
            rx_state = IDLE;
            break;
        case 5: //we set canbus specs we don't accept replies.
            rx_state = IDLE;
            break;
        case 6: //get canbus parameters from GVRET
            rx_state = IDLE;
            rx_step = 0;
            break;
        case 7: //get device info
            rx_state = IDLE;
            rx_step = 0;
            break;
        case 9:
            //validationCounter = 10;
            qDebug() << "Got validated";
            rx_state = IDLE;
            break;
        case 12:
            rx_state = IDLE;
            qDebug() << "Got num buses reply";
            rx_step = 0;
            break;
        case 13:
            rx_state = IDLE;
            qDebug() << "Got extended buses info reply";
            rx_step = 0;
            break;
        case 20:
            rx_state = BUILD_FD_FRAME;
            rx_step = 0;
            break;
        case 22:
            rx_state = IDLE;
            rx_step = 0;
            qDebug() << "Got FD settings reply";
            break;
        }
        break;
    case BUILD_CAN_FRAME:
        switch (rx_step)
        {
        case 0:
            buildTimestamp = c;
            break;
        case 1:
            buildTimestamp |= (uint)(c << 8);
            break;
        case 2:
            buildTimestamp |= (uint)c << 16;
            break;
        case 3:
            buildTimestamp |= (uint)c << 24;
            break;
        case 4:
            canID = c;
            break;
        case 5:
            canID |= c << 8;
            break;
        case 6:
            canID |= c << 16;
            break;
        case 7:
            canID |= c << 24;
            if ((canID & 1 << 31) == 1u << 31)
            {
                canID &= 0x7FFFFFFF;
                isExtended = true;
            }
            isExtended = false;
            break;
        case 8:
            canLen = (c & 0xF);
            canBus = (c & 0xF0) >> 4;
            break;
        default:
            if (rx_step < canLen + 9)
            {
                canData[rx_step - 9] = c;
                if (rx_step == canLen + 8) //it's the last data byte so immediately process the frame
                {
                    rx_state = IDLE;
                    rx_step = 0;
                    //if capturing is on then send this frame to the logging file
                    //qDebug() << "Saving a frame";
                    writeFrameToFile();
                }
            }
            else //should never get here! But, just in case, reset the comm
            {
                rx_state = IDLE;
                rx_step = 0;
            }
            break;
        }
        rx_step++;
        break;
    case BUILD_FD_FRAME:
        switch (rx_step)
        {
        case 0:
            buildTimestamp = c;
            break;
        case 1:
            buildTimestamp |= (uint)(c << 8);
            break;
        case 2:
            buildTimestamp |= (uint)c << 16;
            break;
        case 3:
            buildTimestamp |= (uint)c << 24;
            break;
        case 4:
            canID = c;
            break;
        case 5:
            canID |= c << 8;
            break;
        case 6:
            canID |= c << 16;
            break;
        case 7:
            canID |= c << 24;
            if ((canID & 1 << 31) == 1u << 31)
            {
                canID &= 0x7FFFFFFF;
                isExtended = true;
            }
            else isExtended = false;
            break;
        case 8:
            canLen = (c & 0x3F);
            break;
        case 9:
            canBus = c;
        default:
            if (rx_step < canLen + 10)
            {
                canData[rx_step - 9] = c;
            }
            else
            {
                rx_state = IDLE;
                rx_step = 0;
                writeFrameToFile();
            }
            break;
        }
        rx_step++;
        break;
    }
}

void MainWindow::writeFrameToFile()
{
    if (!writingFile) return;
    capturedFrames++;
    writingFile->write(QString::number(buildTimestamp).toUtf8());
    writingFile->putChar(44);

    writingFile->write(QString::number(canID, 16).toUpper().rightJustified(8, '0').toUtf8());
    writingFile->putChar(44);

    if (isExtended) writingFile->write("true,");
    else writingFile->write("false,");

    writingFile->write("Rx,");

    writingFile->write(QString::number(canBus).toUtf8());
    writingFile->putChar(44);

    writingFile->write(QString::number(canLen).toUtf8());
    writingFile->putChar(44);

    for (int temp = 0; temp < 8; temp++)
    {
        if (temp < canLen)
            writingFile->write(QString::number(canData[temp], 16).toUpper().rightJustified(2, '0').toUtf8());
        else
            writingFile->write("00");
        writingFile->putChar(44);
    }

    writingFile->write("\n");
}


void MainWindow::loggingButtonClicked()
{
    loggingActive = !loggingActive;
    ui->btnStartLog->setText(loggingActive?"Stop Logging":"Start Logging");
    if (loggingActive)
    {
        QDateTime dt = QDateTime::currentDateTime();
        QString name =  QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/" + dt.toString("dd-MM-yyyy_hh-mm-ss") + ".csv";
        qDebug() << name;
        if (writingFile)
        {
            writingFile->close();
            delete writingFile;
            writingFile = nullptr;
        }
        writingFile = new QFile(name);
        writingFile->open(QFile::WriteOnly);
        writingFile->write("Time Stamp,ID,Extended,Dir,Bus,LEN,D1,D2,D3,D4,D5,D6,D7,D8\n");
        capturedFrames = 0;
    }
    else
    {
        if (writingFile)
        {
            writingFile->close();
            delete writingFile;
            writingFile = nullptr;
        }
    }
}

//every timer tick we'll update the number of frames we've sent.
void MainWindow::tickedTimer()
{
    ui->txtStatus->clear();
    QString txt;
    if (connectedToDevice) txt = "Connected to a capture device.\n";
    if (writingFile)
    {
        txt += "Writing to file: " + writingFile->fileName() + "\n" + "Number of frames captured: " +
                QString::number(capturedFrames) + "\nFile Size: " + QString::number(writingFile->size()) + "\n";
    }
    ui->txtStatus->setPlainText(txt);
}
