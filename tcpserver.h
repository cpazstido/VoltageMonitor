#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include "utils.h"
#include "server.h"
typedef unsigned char BYTE;

class TCPServer : public QObject,public Server
{
    Q_OBJECT
public:
    explicit TCPServer(QObject *parent = 0);
    int init(int port);
    void close();
    void getCommand();
    int searchA55A(BYTE *bBuf,int startPos, int endPos);
    int getCmdLength(BYTE *bBuf,int cmdStartPos);
    int writeMessage(char *data,int length);

private:
    QTcpSocket *tcpSocket;
    QTcpServer *tcpServer;
    BYTE *bRcvBuf;
    int iSearchStart;
    int iRcvBufEnd;

signals:
    void readBegin();
    void readEnd();
    void writeBegin();
    void writeEnd();
    void signalSendData(BYTE *ba, int length);

public slots:
    void newListen();
    void acceptConnection();
    void displayError(QAbstractSocket::SocketError);
    void onSendClicked();
    void sendData();
    void readMessage();    
};

#endif // TCPSERVER_H
