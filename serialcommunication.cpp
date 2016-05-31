#include "serialcommunication.h"
#include <QDebug>
#include "mainwindow.h"
#include "utils.h"
#include <QMessageBox>

SerialCommunication::SerialCommunication(QObject *parent):QObject(parent)
{
    m_serialPort=new QSerialPort(this);
    bRcvBuf=new BYTE[4096];
    memset(bRcvBuf,'\0',4096);
    iSearchStart=0;
    iRcvBufEnd=0;
}

int SerialCommunication::init(QSerialPort::BaudRate baud,QSerialPort::DataBits data,
                              QSerialPort::Parity parity,QSerialPort::StopBits stopBits,
                              QString name)
{
    m_serialPort->setBaudRate(baud);
    m_serialPort->setDataBits(data);
    m_serialPort->setParity(parity);
    m_serialPort->setStopBits(stopBits);
    m_serialPort->setPortName(name);
    m_serialPort->setReadBufferSize(9600);

    if (!m_serialPort->open(QIODevice::ReadWrite))
    {
        qDebug()<<"can't open serial port!";
        return -1;
    }

    connect(m_serialPort,SIGNAL(readyRead()),this,SLOT(handleReadyRead()));
    connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), SLOT(handleError(QSerialPort::SerialPortError)));
    return 1;
}

void SerialCommunication::close()
{
    if(m_serialPort->isOpen())
    {
        m_serialPort->close();
    }
}

void SerialCommunication::getCommand()
{
    int iRcvCmdStart = -1;
    int iCmdLength = 0;

    while(((iRcvCmdStart=searchA55A(bRcvBuf, iSearchStart, iRcvBufEnd))!=-1))
    {
        //如果这个命令没有接收完，就继续接收
        if(iRcvBufEnd < iRcvCmdStart+ getCmdLength(bRcvBuf, iRcvCmdStart)+27)
        {
            qDebug()<<"命令没接收完，继续接收！";
            break;
        }
        iCmdLength = getCmdLength(bRcvBuf, iRcvCmdStart);
        qDebug()<<"报文内容长度:"<<iCmdLength;

        qDebug()<<"命令接收完，进行CRC校验和命令处理！";
        BYTE *bCommand = new BYTE[iCmdLength + 27];
        memcpy(bCommand, bRcvBuf+iRcvCmdStart, iCmdLength + 27);
        iSearchStart = iRcvCmdStart + iCmdLength + 27;

        qDebug()<<"接收到的命令：=========="<<byteToHexStr(bCommand,iCmdLength + 27);
        // 首先进行CRC校验，校验不通过的命令丢弃
        int iCRCLength=0;
        iCRCLength = iCmdLength + 27 - 5;
        BYTE *bTmp = new BYTE[iCRCLength];
        memcpy(bTmp, bCommand+2, iCRCLength);

        qDebug()<<"crc长度："<<iCRCLength;
        BYTE crc[2];

        RTU_CRC(crc,bTmp,iCRCLength);
        if (bCommand[iCRCLength + 2] != crc[0] || bCommand[iCRCLength + 3] != crc[1])
        {
            qDebug()<<"该命令的Crc校验不通过，丢弃";
            continue;
        }
        //命令处理
        emit signalSendData(bRcvBuf+iRcvCmdStart, iCmdLength+27);//传数据给主界面

        iCmdLength =0;
        iRcvCmdStart = -1;

        qDebug()<<"命令处理完成";
    }

    if(iSearchStart != 0)
    {
        for(int i=0; i< iRcvBufEnd-iSearchStart;i++)
        {
            bRcvBuf[i] = bRcvBuf[iSearchStart+i];
        }
        for(int i = iRcvBufEnd-iSearchStart; i< iRcvBufEnd; i++)
        {
            bRcvBuf[i] = 0;
        }
        iRcvBufEnd = iRcvBufEnd - iSearchStart;
        iSearchStart = 0;
    }
}

int SerialCommunication::searchA55A(BYTE *bBuf, int startPos, int endPos)
{
    for(int i=startPos; i<endPos; i++)
    {
        if(((bBuf[i]&0xFF) == 0xA5)&& ((bBuf[i+1]&0xFF) == 0x5A))
        {
            return i;
        }
    }
    return -1;
}

int SerialCommunication::getCmdLength(BYTE *bBuf, int cmdStartPos)
{
    int iLen = (bBuf[cmdStartPos+2]&0xFF)+ (bBuf[cmdStartPos+3]&0xFF)*256;
    return iLen;
}

int SerialCommunication::writeMessage(char *data, int length)
{
    if(m_serialPort == NULL)
    {
        QMessageBox::warning(NULL,"提示","还没有设备上线");
        return -1;
    }
    //TX灯亮
    emit writeBegin();

    //TX灯灭
    emit writeEnd();
    return m_serialPort->write(data,length);
}

void SerialCommunication::handleReadyRead()
{
//    emit readBegin();
//    qDebug()<<m_serialPort->bytesAvailable();
//    m_readData=m_serialPort->readAll();

//    if (m_readData.isEmpty())
//    {
//        qDebug()<<"No data was currently available for reading from port";
//    }
//    else
//    {
//        qDebug()<<"Data successfully:";
//        qDebug()<<m_readData;
//    }
//    emit readEnd();

    while(m_serialPort->bytesAvailable()!=0)
    {
        //RX灯亮
        emit readBegin();
        //qDebug()<<"总大小:"<<tcpSocket->bytesAvailable();

        char *bReadNew=new char[1024];
        int iReadLen=m_serialPort->read(bReadNew,1024);
        memcpy(bRcvBuf+iRcvBufEnd,bReadNew,iReadLen);
        iRcvBufEnd = iRcvBufEnd + iReadLen;
        //qDebug()<<"接收了:"<<tcpSocket->bytesAvailable();
        getCommand();
        emit readEnd();//RX灯灭
    }
}

void SerialCommunication::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ReadError) {
        qDebug()<<"An I/O error occurred while reading the data from port";
    }
}

void SerialCommunication::readMessage()
{
    while(m_serialPort->bytesAvailable()!=0)
    {
        //RX灯亮
        emit readBegin();
        //qDebug()<<"总大小:"<<tcpSocket->bytesAvailable();

        char *bReadNew=new char[1024];
        int iReadLen=m_serialPort->read(bReadNew,1024);
        memcpy(bRcvBuf+iRcvBufEnd,bReadNew,iReadLen);
        iRcvBufEnd = iRcvBufEnd + iReadLen;
        //qDebug()<<"接收了:"<<tcpSocket->bytesAvailable();
        getCommand();
        emit readEnd();//RX灯灭
    }
}

//int byteToInt(char *str)
//{
//    int result = 0;
//    int i0 = str[0]&0xFF;
//    int i1 = (str[1]&0xFF)<<8;
//    result=i0+i1;
//    return result;
//}

