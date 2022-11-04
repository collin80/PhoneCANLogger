#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkDatagram>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QFile>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


namespace SERIALSTATE {

enum STATE
{
    IDLE,
    GET_COMMAND,
    BUILD_CAN_FRAME,
    TIME_SYNC,
    GET_DIG_INPUTS,
    GET_ANALOG_INPUTS,
    SET_DIG_OUTPUTS,
    SETUP_CANBUS,
    GET_CANBUS_PARAMS,
    GET_DEVICE_INFO,
    SET_SINGLEWIRE_MODE,
    GET_NUM_BUSES,
    GET_EXT_BUSES,
    BUILD_FD_FRAME,
    GET_FD_SETTINGS
};

}

using namespace SERIALSTATE;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void readPendingDatagrams();
    void clickedConnect();
    void readSerialData();
    void deviceConnected();
    void loggingButtonClicked();
    void tickedTimer();

private:
    Ui::MainWindow *ui;
    QUdpSocket *rxBroadcastGVRET;
    QTcpSocket *tcpClient;
    QVector<QString> remoteDeviceIPGVRET;
    QFile *writingFile;
    QTimer updateTimer;
    bool loggingActive;
    bool connectedToDevice;
    STATE rx_state;
    int rx_step;
    qint64 buildTimestamp;
    uint32_t canID;
    uint8_t canLen;
    bool isExtended;
    uint8_t canBus;
    uint8_t canData[8];
    int capturedFrames;

    void disconnectDevice();
    void procRXChar(unsigned char c);
    void writeFrameToFile();
};
#endif // MAINWINDOW_H
