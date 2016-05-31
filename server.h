#ifndef SERVER_H
#define SERVER_H


class Server
{
public:
    Server();
    virtual int writeMessage(char *data,int length)=0;
    virtual void close()=0;
};

#endif // SERVER_H
