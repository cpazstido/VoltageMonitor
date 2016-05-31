#include "tcpserver.h"
#include "utils.h"
#include <QMessageBox>

int byteToInt(char *str);
TCPServer::TCPServer(QObject *parent) : QObject(parent),tcpSocket(NULL)
{
    this->tcpServer = NULL;
    bRcvBuf=new BYTE[4096];
    memset(bRcvBuf,'\0',4096);
    iSearchStart=0;
    iRcvBufEnd=0;
    int i=0;

}

int TCPServer::init(int port)
{
    if(tcpServer == NULL)
    {
        this->tcpServer = new QTcpServer(this);
    }

    if(!tcpServer->isListening()&&!tcpServer->listen(QHostAddress::Any,port))
    {
        qDebug()<<tcpServer->errorString();
        return -1;
    }
    connect(tcpServer,SIGNAL(newConnection()),this,SLOT(acceptConnection()));
    return 1;
}

void TCPServer::close()
{
    if(tcpServer != NULL && tcpServer->isListening())
    {
        tcpServer->close();
        tcpServer = NULL;
    }
    if(tcpSocket!=NULL && tcpSocket->isOpen())
    {
        tcpSocket->close();        
    }
    tcpSocket = NULL;
}

void TCPServer::newListen()
{

}

void TCPServer::acceptConnection()
{
    if(tcpSocket == NULL)
    {
        qDebug()<<"new connectting!";
        //QMessageBox::warning(NULL,"提示","设备没上线！");
        tcpSocket = tcpServer->nextPendingConnection();
        connect(tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayError(QAbstractSocket::SocketError)));
        connect(tcpSocket,SIGNAL(readyRead()),this,SLOT(readMessage()));        
    }
    else
    {
        qDebug()<<"exist!";
    }
}

void TCPServer::displayError(QAbstractSocket::SocketError)
{
    tcpSocket = NULL;
}

void TCPServer::onSendClicked()
{

}

void TCPServer::sendData()
{

}

int TCPServer::searchA55A(BYTE *bBuf,int startPos, int endPos)
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

int TCPServer::getCmdLength(BYTE *bBuf,int cmdStartPos)
{
    int iLen = (bBuf[cmdStartPos+2]&0xFF)+ (bBuf[cmdStartPos+3]&0xFF)*256;
    return iLen;
}

int TCPServer::writeMessage(char *data,int length)
{
    if(tcpSocket == NULL)
    {
        QMessageBox::warning(NULL,"提示","还没有设备上线1");
        return -1;
    }
    //TX灯亮
    emit writeBegin();

    //TX灯灭
    emit writeEnd();
    return tcpSocket->write(data,length);
}

void TCPServer::readMessage()
{
    while(tcpSocket->bytesAvailable()!=0)
    {
        //RX灯亮
        emit readBegin();
        qDebug()<<"总大小:"<<tcpSocket->bytesAvailable();

        char *bReadNew=new char[1024];
        int iReadLen=tcpSocket->read(bReadNew,1024);
        memcpy(bRcvBuf+iRcvBufEnd,bReadNew,iReadLen);
        iRcvBufEnd = iRcvBufEnd + iReadLen;
        //qDebug()<<"接收了:"<<tcpSocket->bytesAvailable();
        getCommand();
        emit readEnd();//RX灯灭
    }
}

void TCPServer::getCommand()
{
    qDebug()<<"---------------------";
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
        qDebug()<<"校验位(原)："<<QString::number(bCommand[iCRCLength + 2]&0xff,16)<<" "<<QString::number(bCommand[iCRCLength + 3]&0xff,16);
        qDebug()<<"校验位(现)："<<QString::number(crc[0]&0xff,16)<<" "<<QString::number(crc[1]&0xff,16);
        if (bCommand[iCRCLength + 2] != crc[0] || bCommand[iCRCLength + 3] != crc[1])
        {
            qDebug()<<"该命令的Crc校验不通过，丢弃";
            //QMessageBox::warning(NULL,"提示","校验不通过！");
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


int byteToInt(char *str)
{
    int result = 0;
    int i0 = str[0]&0xFF;
    int i1 = (str[1]&0xFF)<<8;
    result=i0+i1;
    return result;
}

