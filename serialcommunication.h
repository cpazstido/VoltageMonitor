#ifndef SERIALCOMMUNICATION_H
#define SERIALCOMMUNICATION_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include "utils.h"
#include "server.h"
typedef unsigned char BYTE;

class SerialCommunication: public QObject,public Server
{
    Q_OBJECT
public:
    SerialCommunication(QObject *parent = 0);
    int init(QSerialPort::BaudRate baud,QSerialPort::DataBits data,
             QSerialPort::Parity parity,QSerialPort::StopBits stopBits,
             QString name);
    void close();    
    void getCommand();
    int searchA55A(BYTE *bBuf,int startPos, int endPos);
    int getCmdLength(BYTE *bBuf,int cmdStartPos);
    int writeMessage(char *data,int length);


signals:
    void readBegin();
    void readEnd();
    void writeBegin();
    void writeEnd();
    void signalSendData(BYTE *ba, int length);

public slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);
    void readMessage();

private:
    QSerialPort *m_serialPort;
    QByteArray  m_readData;
    BYTE *bRcvBuf;
    int iSearchStart;
    int iRcvBufEnd;
};

#endif // SERIALCOMMUNICATION_H
