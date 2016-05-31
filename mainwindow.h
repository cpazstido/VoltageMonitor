#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTime>
#include "tcpserver.h"
#include "serialcommunication.h"
#include <QButtonGroup>
#include <QtSerialPort>
#include <qmath.h>
#include <QStandardItemModel>
typedef unsigned char BYTE;

namespace Ui {
class MainWindow;
}

#pragma pack(push, 1)
typedef struct {
    BYTE Sync[2];           //报文头5AA5
    BYTE Packet_Length[2];  //报文长度
    BYTE Device_Code[17];   //电压监测装置ID(17位编码)
    BYTE Frame_Type[1];     //帧类型
    BYTE Packet_Type[1];    //报文类型
    BYTE Frame_No[1];       //帧序列号
}ProtocolHeader;
#pragma pop

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void changeState();


public slots:
    void showTime();//显示时间

public slots:
    void on_pushButtonOpen_clicked();
    void onRadioClick();
    void slotReadBegin();
    void slotreadEnd();
    void slotWriteBegin();
    void slotWriteEnd();
    void slotRevData(BYTE *ba, int length);

private:
    QButtonGroup *btnGroupConn;
    Server *m_server;
    TCPServer *m_tcp;
    SerialCommunication *m_serial;
    bool m_openFlag;
    ProtocolHeader *stProtocolHeader;
    ProtocolHeader *stSndProtocolHeader;
    BYTE *bProtocolBody;
    int iProtocolLen;
    BYTE *deviceID;
    QStandardItemModel *voltageData_model;
    QStandardItemModel *event_model;
    QStandardItemModel *monthStatistical_model;
    QStandardItemModel *dailyStatistical_model;

public:
    Ui::MainWindow *ui;
    QString byteToHexStr(BYTE *b, int length);
    void analysisProtocol(BYTE *protocol, int length);
    QString getFrameType(ProtocolHeader *);
    QString getPacketType(BYTE *protocol);
    BYTE* doubleToVoltage(double dd);
    double VoltageToDouble(BYTE* dd);
    BYTE *int2ToBYTE(int ii);
    int byteToint(BYTE *byte);
    //将byte型ip装化为QString
    QString byte4IPToString(BYTE *dd);
    //将QString IP转化为BYTE格式
    BYTE * QStringIPToByte(QString ip);
    //将int转化为2位BYTE
    BYTE *intToByte2(int i);

    //获取当前时间
    BYTE *getCurrentTime();
    //获取事件类型
    QString getEventType(BYTE type);
    //获取版本号
    QString getStringFromProtocol(BYTE *protocol);
    //将datetime转化为Byte*
    BYTE *formateFromDateTimeToBYTE(QDateTime dt);


    //将4字节byte转化为int
    int byte4ToInt(BYTE *protocol);
    //将2字节byte转化为int
    int byte2ToInt(BYTE *byte);
    //从协议中获取时间
    qint64 getTimeFromByte(BYTE *protocol);
    //将double电压合格率转化为指定格式的Byte数组
    BYTE *doubleToVoltageEligibilityRate(double dd);

    //将byte电压合格率转化为double
    double VoltageEligibilityRateToDouble(BYTE* dd);

    //处理电压数据
    void handleVoltageData(BYTE *protocol);
    //处理日统计报
    void handleDailyStatistical(BYTE *protocol);
    //处理月统计报
    void handleMonthStatistical(BYTE *protocol);
    //处理时间报
    void handleTime(BYTE *protocol);
    //处理事件报
    void handleEvent(BYTE *protocol);
    //处理装置基本信息
    void handleDeviceInfo(BYTE *protocol);
    //处理工作状态
    void handleWorkState(BYTE *protocol);
    //处理流量数据报
    void handleFlowData(BYTE *protocol);
    //处理CAC信息报
    void handleCACInfo(BYTE *protocol);
    //处理设备ID
    void handleDeviceID(BYTE *protocol);
    //处理命令
    void handleCMD(BYTE *protocol);
    //处理监测点参数
    void handleMeasurePointParam(BYTE *protocol);
    //处理通信参数
    void handleConnParam(BYTE *protocol);
    //处理事件参数
    void handleEventParam(BYTE *protocol);
    //处理数据请求
    void handleQueryData(BYTE *protocol);

    //回复
    //回复心跳
    void responseHeartBeat(ProtocolHeader *header);
    //回复电压数据
    void responseVoltageData(ProtocolHeader *header);
    //回复日统计
    void responseDailyStatistical(ProtocolHeader *header);
    //回复月统计
    void responseMonthStatistical(ProtocolHeader *header);
    //回复事件
    void responseEvent(ProtocolHeader *header);
    //回复流量数据上报
    void responseFlowData(ProtocolHeader *header);

    void responseProtocol();

private slots:
    void on_pushButtonQueryTime_clicked();
    void on_pushButtonSetTime_clicked();
    void on_pushButtonClearRcv_clicked();
    void on_comboBoxConnMode_currentIndexChanged(const QString &arg1);
    void on_comboBoxConnMode_currentIndexChanged(int index);
    void on_pushButtonQueryConParam_clicked();
    void on_pushButtonSetConParam_clicked();
    void on_pushButtonQueryMeasureParam_clicked();
    void on_pushButtonSetMeasureParam_clicked();
    void on_pushButtonQueryCAC_clicked();
    void on_pushButtonSetCAC_clicked();
    void on_pushButtonQueryWorkState_clicked();
    void on_pushButtonQueryID_clicked();
    void on_pushButtonSetID_clicked();
    void on_pushButtonDeviceReset_clicked();
    void on_pushButtonSendCMD_clicked();
    void on_pushButtonQueryDeviceInfo_clicked();
    void on_pushButtonQueryEventParam_clicked();
    void on_pushButtonSetEventParam_clicked();
    void on_pushButtonQueryData_clicked();
};

#endif // MAINWINDOW_H
