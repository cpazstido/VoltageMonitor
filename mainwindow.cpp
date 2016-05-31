#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QNetworkInterface>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),m_openFlag(false),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->mainToolBar->hide();//影藏工具条不要

    //单选按钮
    btnGroupConn=new QButtonGroup(this);
    btnGroupConn->addButton(ui->radioButtonTCP,0);
    btnGroupConn->addButton(ui->radioButtonSerial,1);
    ui->radioButtonSerial->setChecked(true);
    ui->groupBoxSerialParam->setVisible(true);
    ui->groupBoxTCPParam->setVisible(false);
    connect(ui->radioButtonTCP, SIGNAL(clicked()), this, SLOT(onRadioClick()));
    connect(ui->radioButtonSerial, SIGNAL(clicked()), this, SLOT(onRadioClick()));

    //设置串口
    int i=0;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        qDebug() << "Name        : " << info.portName();
        qDebug() << "Description : " << info.description();
        qDebug() << "Manufacturer: " << info.manufacturer();

        ui->comboBoxSerial->addItem(info.portName(), i);
        i++;
    }

    //设置波特率
    ui->comboBoxBitrate->addItem("1200",1200);
    ui->comboBoxBitrate->addItem("2400",2400);
    ui->comboBoxBitrate->addItem("4800",4800);
    ui->comboBoxBitrate->addItem("9600",9600);

    //数据位
    ui->comboBoxDataBit->addItem("8",8);
    ui->comboBoxDataBit->addItem("7",7);
    ui->comboBoxDataBit->addItem("6",6);
    ui->comboBoxDataBit->addItem("5",5);

    //校验位
    ui->comboBoxVerify->addItem("无",0);
    ui->comboBoxVerify->addItem("奇",2);
    ui->comboBoxVerify->addItem("偶",3);

    //停止位
    ui->comboBoxStopBit->addItem("1",1);
    ui->comboBoxStopBit->addItem("2",2);

    //设置时间
    QTimer *timer=new QTimer(this);
    //在界面上显示时间
    connect(timer,SIGNAL(timeout()),this,SLOT(showTime()));
    timer->start(500);

    //参数设置 选择通信方式
    ui->comboBoxConnMode->addItem("无线专网",0);
    ui->comboBoxConnMode->addItem("短信通信",2);

    //装置复位
    ui->comboBoxReset->addItem("复位至正常模式",0);
    ui->comboBoxReset->addItem("复位至调试模式",1);

    //数据类型
    ui->comboBoxQueryDataType->addItem("电压数据报",0x04);
    ui->comboBoxQueryDataType->addItem("日统计数据报",0x05);
    ui->comboBoxQueryDataType->addItem("月统计数据报",0x06);
    //响应数据类型
    ui->comboBoxResponseDataType->addItem("电压数据报",0x04);
    ui->comboBoxResponseDataType->addItem("日统计数据报",0x05);
    ui->comboBoxResponseDataType->addItem("月统计数据报",0x06);

    m_server=NULL;
    m_tcp=new TCPServer(this);
    m_serial=new SerialCommunication(this);
    connect(m_tcp,SIGNAL(readBegin()),this,SLOT(slotReadBegin()));
    connect(m_tcp,SIGNAL(readEnd()),this,SLOT(slotreadEnd()));
    connect(m_tcp,SIGNAL(writeBegin()),this,SLOT(slotWriteBegin()));
    connect(m_tcp,SIGNAL(writeEnd()),this,SLOT(slotWriteEnd()));
    connect(m_tcp,SIGNAL(signalSendData(BYTE *, int)),this,SLOT(slotRevData(BYTE *, int)));


    connect(m_serial,SIGNAL(readBegin()),this,SLOT(slotReadBegin()));
    connect(m_serial,SIGNAL(readEnd()),this,SLOT(slotreadEnd()));
    connect(m_serial,SIGNAL(writeBegin()),this,SLOT(slotWriteBegin()));
    connect(m_serial,SIGNAL(writeEnd()),this,SLOT(slotWriteEnd()));
    connect(m_serial,SIGNAL(signalSendData(BYTE *, int)),this,SLOT(slotRevData(BYTE *, int)));

    stSndProtocolHeader=new ProtocolHeader;
    deviceID=new BYTE[17];
    memset(deviceID,'\0',17);

    //事件上报 表头
    event_model = new QStandardItemModel(this);
    event_model->setHorizontalHeaderItem(0, new QStandardItem(QObject::tr("监测点号")));
    event_model->setHorizontalHeaderItem(1, new QStandardItem(QObject::tr("采集时间")));
    event_model->setHorizontalHeaderItem(2, new QStandardItem(QObject::tr("类型")));
    event_model->setHorizontalHeaderItem(3, new QStandardItem(QObject::tr("描述")));
    this->ui->tableViewEvent->setModel(event_model);

    //电压数据表头
    voltageData_model = new QStandardItemModel();
    voltageData_model->setHorizontalHeaderItem(0, new QStandardItem(QObject::tr("监测点号")));
    voltageData_model->setHorizontalHeaderItem(1, new QStandardItem(QObject::tr("采集时间")));
    voltageData_model->setHorizontalHeaderItem(2, new QStandardItem(QObject::tr("电压值")));
    ui->tableViewVoltageData->setModel(voltageData_model);

    //月统计表头
    monthStatistical_model = new QStandardItemModel();
    monthStatistical_model->setHorizontalHeaderItem(0, new QStandardItem(QObject::tr("监测点号")));
    monthStatistical_model->setHorizontalHeaderItem(1, new QStandardItem(QObject::tr("采集时间")));
    monthStatistical_model->setHorizontalHeaderItem(2, new QStandardItem(QObject::tr("电压合格率(%)")));
    monthStatistical_model->setHorizontalHeaderItem(3, new QStandardItem(QObject::tr("电压越上限率(%)")));
    monthStatistical_model->setHorizontalHeaderItem(4, new QStandardItem(QObject::tr("电压越下限率(%)")));
    monthStatistical_model->setHorizontalHeaderItem(5, new QStandardItem(QObject::tr("电压统计时间(分钟)")));
    monthStatistical_model->setHorizontalHeaderItem(6, new QStandardItem(QObject::tr("电压合格时间(分钟)")));
    monthStatistical_model->setHorizontalHeaderItem(7, new QStandardItem(QObject::tr("电压越上限时间(分钟)")));
    monthStatistical_model->setHorizontalHeaderItem(8, new QStandardItem(QObject::tr("电压越下线时间(分钟)")));
    monthStatistical_model->setHorizontalHeaderItem(9, new QStandardItem(QObject::tr("电压平均值(V)")));
    monthStatistical_model->setHorizontalHeaderItem(10, new QStandardItem(QObject::tr("电压最小值(V)")));
    monthStatistical_model->setHorizontalHeaderItem(11, new QStandardItem(QObject::tr("电压最小值发生时间")));
    monthStatistical_model->setHorizontalHeaderItem(12, new QStandardItem(QObject::tr("电压最大值(V)")));
    monthStatistical_model->setHorizontalHeaderItem(13, new QStandardItem(QObject::tr("电压最大值发生时间")));
    ui->tableViewMonthStatistical->setModel(monthStatistical_model);

    //日统计表头
    dailyStatistical_model = new QStandardItemModel();
    dailyStatistical_model->setHorizontalHeaderItem(0, new QStandardItem(QObject::tr("监测点号")));
    dailyStatistical_model->setHorizontalHeaderItem(1, new QStandardItem(QObject::tr("采集时间")));
    dailyStatistical_model->setHorizontalHeaderItem(2, new QStandardItem(QObject::tr("电压合格率(%)")));
    dailyStatistical_model->setHorizontalHeaderItem(3, new QStandardItem(QObject::tr("电压越上限率(%)")));
    dailyStatistical_model->setHorizontalHeaderItem(4, new QStandardItem(QObject::tr("电压越下限率(%)")));
    dailyStatistical_model->setHorizontalHeaderItem(5, new QStandardItem(QObject::tr("电压统计时间(分钟)")));
    dailyStatistical_model->setHorizontalHeaderItem(6, new QStandardItem(QObject::tr("电压合格时间(分钟)")));
    dailyStatistical_model->setHorizontalHeaderItem(7, new QStandardItem(QObject::tr("电压越上限时间(分钟)")));
    dailyStatistical_model->setHorizontalHeaderItem(8, new QStandardItem(QObject::tr("电压越下线时间(分钟)")));
    dailyStatistical_model->setHorizontalHeaderItem(9, new QStandardItem(QObject::tr("电压平均值(V)")));
    dailyStatistical_model->setHorizontalHeaderItem(10, new QStandardItem(QObject::tr("电压最小值(V)")));
    dailyStatistical_model->setHorizontalHeaderItem(11, new QStandardItem(QObject::tr("电压最小值发生时间")));
    dailyStatistical_model->setHorizontalHeaderItem(12, new QStandardItem(QObject::tr("电压最大值(V)")));
    dailyStatistical_model->setHorizontalHeaderItem(13, new QStandardItem(QObject::tr("电压最大值发生时间")));
    ui->tableViewDailyStatistical->setModel(dailyStatistical_model);

//    double dd=196.789;
//    qDebug()<<byteToHexStr(doubleToVoltageEligibilityRate(dd),3);
//    BYTE *ss=new BYTE[3];
//    ss[0]=137;
//    ss[1]=103;
//    ss[2]=25;
//    qDebug()<<VoltageEligibilityRateToDouble(ss);
//    BYTE *ss=new BYTE[4];
//    ss[0]=192;
//    ss[1]=168;
//    ss[2]=0;
//    ss[3]=1;
//    qDebug()<<byte4IPToString(ss);
//    qDebug()<<byteToHexStr(QStringIPToByte(byte4IPToString(ss)),4);
}

//将QString IP转化为BYTE格式
BYTE * MainWindow::QStringIPToByte(QString ip)
{
    BYTE *bIP=new BYTE[4];
    QStringList ips=ip.split(".");
    bIP[0]=ips.at(0).toInt();
    bIP[1]=ips.at(1).toInt();
    bIP[2]=ips.at(2).toInt();
    bIP[3]=ips.at(3).toInt();
    return bIP;
}

//将Byte格式IP转化为QString
QString MainWindow::byte4IPToString(BYTE *dd)
{
    QString str;
    str=QString::number((int)dd[0]);
    str+=".";
    str+=QString::number((int)dd[1]);
    str+=".";
    str+=QString::number((int)dd[2]);
    str+=".";
    str+=QString::number((int)dd[3]);
    return str;
}

//将Byte电压合格率数据格式转化为double
double MainWindow::VoltageEligibilityRateToDouble(BYTE* dd)
{
    return ((float)(byteToint(dd)))/1000+((float)(((dd+1)[0])&0x0f))/10+(((dd+1)[0])>>4)+(byteToint(dd+2))*10;
}

//将double电压合格率转化为BYTE
BYTE *MainWindow::doubleToVoltageEligibilityRate(double dd)
{
    BYTE *bpVolEliPate=new BYTE[4];

    QString sdd=QString::number(dd,'g',8);
    int inte=0;
    //处理小数
    qDebug()<<sdd;
    int theUnit=0;//个位
    if(sdd.indexOf('.')==-1)
    {
        bpVolEliPate[0]=0x00;
        inte=dd;
        theUnit=0;
    }
    else
    {
        QStringList ss=sdd.split('.');
        int decimal=ss.at(1).toInt();
        inte=ss.at(0).toInt();

        if(decimal >= 100)//小数点后3位
        {
        memcpy(bpVolEliPate,int2ToBYTE(decimal%100),1);
        theUnit=decimal/100;
        }
        else if(decimal < 100)
        {
            if(decimal < 10)//小数点后1位
            {
                bpVolEliPate[0]=0x00;
                theUnit=decimal;
            }
            else//小数点后2位
            {
                memcpy(bpVolEliPate,int2ToBYTE(((decimal%10)*10)),1);
                theUnit=decimal/10;
            }
        }
    }
    memcpy(bpVolEliPate+1,int2ToBYTE(((inte%10)*10)+theUnit),1);
    inte=inte/10;
    memcpy(bpVolEliPate+2,int2ToBYTE(inte%100),1);

    return bpVolEliPate;
}

//将Byte电压值数据格式转化为double
double MainWindow::VoltageToDouble(BYTE* dd)
{
    return ((float)(byteToint(dd)))/100+byteToint(dd+1)+byteToint(dd+2)*100+byteToint(dd+3)*10000;
}

//将一个byte电压值数据格式转化为int
int MainWindow::byteToint(BYTE *byte)
{
    return (byte[0]>>4)*10+(int)(byte[0]&0x0f);
}

int MainWindow::byte2ToInt(BYTE *byte)
{
    return (byte[0])+(byte[1]&0xff)*256;
}

int MainWindow::byte4ToInt(BYTE *protocol)
{
    return (protocol[0]&0xff) + ((protocol[1])&0xff)*qPow(2,8) + ((protocol[2])&0xff)*qPow(2,16) + ((protocol[3])&0xff)*qPow(2,24);
}

//将double转化为电压值数据格式
BYTE* MainWindow::doubleToVoltage(double dd)
{
    BYTE *bpVol=new BYTE[4];

    QString sdd=QString::number(dd,'g',8);
    int inte=0;
    //处理小数
    qDebug()<<sdd;
    if(sdd.indexOf('.')==-1)
    {
        bpVol[0]=0x00;
        inte=dd;
    }
    else
    {
        QStringList ss=sdd.split('.');
        int decimal=ss.at(1).toInt();
        inte=ss.at(0).toInt();
        //bpVol[0]=decimal/10*qPow(2,4)+decimal%10;
        memcpy(bpVol,int2ToBYTE(decimal),1);
        //qDebug()<<byteToHexStr(bpVol,1);
    }
    memcpy(bpVol+1,int2ToBYTE(inte%100),1);
    inte=inte/100;
    memcpy(bpVol+2,int2ToBYTE(inte%100),1);
    inte=inte/100;
    memcpy(bpVol+3,int2ToBYTE(inte%100),1);
    inte=inte/100;
    return bpVol;
}

//将int转化为2位BYTE
BYTE *MainWindow::intToByte2(int i)
{
    BYTE *bRe=new BYTE[2];
    bRe[0]=(i%256)&0xff;
    bRe[1]=(i/256)&0xff;
    return bRe;
}

//将int转化为电压值数据格式
BYTE *MainWindow::int2ToBYTE(int ii)
{
    BYTE *bp=new BYTE;
    *bp=ii/10*qPow(2,4)+ii%10;
    return bp;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showTime()
{
    QDateTime time=QDateTime::currentDateTime();
    QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
    ui->lcdNumber->display(txtTime);
}

void MainWindow::on_pushButtonOpen_clicked()
{
    if(btnGroupConn->checkedId()==0)//tcp
    {
        if(m_openFlag)
        {            
            m_tcp->close();
            m_openFlag=false;
            m_server=NULL;
        }
        else
        {
            if(m_tcp->init(ui->spinBoxPort->text().toInt())==-1)
            {
                qDebug()<<"start server fail!";
                m_openFlag=false;
            }
            else
            {
                m_server=m_tcp;
                m_openFlag=true;
            }
        }

    }
    else if(btnGroupConn->checkedId()==1)//serial
    {
        if(m_openFlag)
        {
            m_serial->close();
            m_openFlag=false;
            m_server=NULL;
        }
        else
        {
            if(m_serial->init((QSerialPort::BaudRate)ui->comboBoxBitrate->currentData().toInt(),
                              (QSerialPort::DataBits)ui->comboBoxDataBit->currentData().toInt(),
                              (QSerialPort::Parity)ui->comboBoxVerify->currentData().toInt(),
                              (QSerialPort::StopBits)ui->comboBoxStopBit->currentData().toInt(),
                              ui->comboBoxSerial->currentText()
                              )==-1)
            {
                qDebug()<<"start serial fail!";

                m_openFlag=false;
            }
            else
            {
                m_server=m_serial;
                m_openFlag=true;
            }
        }
    }
    changeState();//改变图标、文字状态

}

void MainWindow::onRadioClick()
{
    switch(btnGroupConn->checkedId())
    {
    case 0:
        ui->groupBoxSerialParam->setVisible(false);
        ui->groupBoxTCPParam->setVisible(true);
        m_serial->close();
        break;
    case 1:
        ui->groupBoxSerialParam->setVisible(true);
        ui->groupBoxTCPParam->setVisible(false);
        m_tcp->close();
        break;
    }
    m_openFlag=false;
    changeState();
}

void MainWindow::slotReadBegin()
{    
    QPixmap pixmap(":/images/redlight.png");
    ui->labelReadLight->setPixmap(pixmap);
    QTime t;
    t.start();
    while(t.elapsed()<100)
        QCoreApplication::processEvents();
}

void MainWindow::slotreadEnd()
{    
    QPixmap pixmap(":/images/graylight.png");
    ui->labelReadLight->setPixmap(pixmap);
}

void MainWindow::slotWriteBegin()
{
    QPixmap pixmap(":/images/redlight.png");
    ui->labelWriteLight->setPixmap(pixmap);
    QTime t;
    t.start();
    while(t.elapsed()<100)
        QCoreApplication::processEvents();
}

void MainWindow::slotWriteEnd()
{
    QPixmap pixmap(":/images/graylight.png");
    ui->labelWriteLight->setPixmap(pixmap);
}

QString MainWindow::byteToHexStr(BYTE *b, int length)
{
    QString stmp="";

    for (int n=0;n<length;n++)
    {
        QString tmp = QString::number(b[n]&0xff,16);
        stmp.append((tmp.length()==1)? "0"+tmp : tmp);
    }
    return stmp.toUpper().trimmed();
}

//处理命令
void MainWindow::handleCMD(BYTE *protocol)
{
    int cmdLen=byte2ToInt(protocol+2);
    char *cDeviceID=new char[cmdLen+1];
    memset(cDeviceID,'\0',cmdLen+1);
    memcpy(cDeviceID,protocol+24,cmdLen);
    this->ui->lineEditCMD->setText(cDeviceID);
    delete cDeviceID;
    cDeviceID=NULL;
}

//处理设备ID
void MainWindow::handleDeviceID(BYTE *protocol)
{
    char *cDeviceID=new char[18];
    memset(cDeviceID,'\0',18);
    memcpy(cDeviceID,protocol+4,17);
    this->ui->lineEditNewDeviceCode->setText(cDeviceID);
    memcpy(cDeviceID,protocol+25,17);
    this->ui->labelOriginalID->setText(cDeviceID);
}

//处理工作状态
void MainWindow::handleWorkState(BYTE *protocol)
{
    qint64 time=getTimeFromByte(protocol+24);
    this->ui->labelStartTime->setText(QDateTime::fromMSecsSinceEpoch(time*1000).toString("yyyy-MM-dd hh:mm:ss"));
    this->ui->labelTotalWorkingTime->setText(QString::number(byte4ToInt(protocol+28)));
    this->ui->labelWorkingTime->setText(QString::number(byte4ToInt(protocol+32)));
}

//处理通信参数
void MainWindow::handleConnParam(BYTE *protocol)
{
    int i=6;
    char * buff=new char[22];
    memset(buff,'\0',22);

    while(i>0)
    {
        if((protocol[25] & (((int)qPow(2,i-1)) & 0xff)) > 0)
        {
            switch(i)
            {
            case 6:
                this->ui->checkBoxPhoneNum->setChecked(true);
                memcpy(buff,protocol+86,20);
                this->ui->lineEditPhoneNum_2->setText(buff);
                memset(buff,'\0',22);
                break;
            case 5:
                this->ui->checkBoxHeartBeat->setChecked(true);
                this->ui->lineEditHeartBeat->setText(QString::number((int)protocol[85]));
                break;
            case 4:
                this->ui->checkBoxAPNPassword->setChecked(true);
                memcpy(buff,protocol+64,21);
                this->ui->lineEditAPNPassword->setText(buff);
                memset(buff,'\0',22);
                break;
            case 3:
                this->ui->checkBoxAPNName->setChecked(true);
                memcpy(buff,protocol+43,21);
                this->ui->lineEditAPNName->setText(buff);
                memset(buff,'\0',22);
                break;
            case 2:
                this->ui->checkBoxAPN->setChecked(true);
                memcpy(buff,protocol+27,16);
                this->ui->lineEditAPN->setText(buff);
                memset(buff,'\0',22);
                break;
            case 1:
                this->ui->groupBoxConMode->setChecked(true);
                if(protocol[26]==0)
                {
                    this->ui->comboBoxConnMode->setCurrentIndex(0);
                }
                else
                {
                    this->ui->comboBoxConnMode->setCurrentIndex(1);
                }
                break;
            }
        }
        i--;
    }
    delete buff;
    buff=NULL;
}

//处理事件参数
void MainWindow::handleEventParam(BYTE *protocol)
{
    int i=3;
    while(i>0)
    {
        if((protocol[25] & (((int)qPow(2,i-1)) & 0xff)) > 0)
        {
            switch(i)
            {
            case 3:
                this->ui->checkBoxCommunicationFlow->setChecked(true);
                this->ui->spinBoxCommunicationFlow->setValue((int)protocol[28]);
                break;
            case 2:
                this->ui->checkBoxResetAutoSend->setChecked(true);
                this->ui->spinBoxResetAutoSend->setValue((int)protocol[27]);
                break;
            case 1:
                this->ui->checkBoxPowerAlarm->setChecked(true);
                this->ui->spinBoxPowerAlarm->setValue((int)protocol[26]);
                break;
            }
        }
        i--;
    }
}

//处理监测点参数
void MainWindow::handleMeasurePointParam(BYTE *protocol)
{
    int i=8;
    while(i>0)
    {
        if((protocol[26] & (((int)qPow(2,i-1)) & 0xff)) > 0)
        {
            switch(i)
            {
            case 8:
                this->ui->checkBoxAverageDay->setChecked(true);
                this->ui->spinBoxAverageDay->setValue((int)protocol[44]);
                break;
            case 7:
                this->ui->checkBoxAverageMins->setChecked(true);
                this->ui->spinBoxAverageMins->setValue((int)protocol[43]);
                break;
            case 6:
                this->ui->checkBoxVoltageGrade->setChecked(true);
                this->ui->lineEditVoltageGrade->setText(QString::number(VoltageToDouble(protocol+39),'g',8));
                break;
            case 5:
                this->ui->checkBoxMonthSettle->setChecked(true);
                this->ui->spinBoxMonthSettle->setValue((int)protocol[38]);
                break;
            case 4:
                this->ui->checkBoxVoltageLLimit->setChecked(true);
                this->ui->lineEditVoltageLLimit->setText(QString::number(VoltageToDouble(protocol+34),'g',8));
                break;
            case 3:
                this->ui->checkBoxVoltageULimit->setChecked(true);
                this->ui->lineEditVoltageULimit->setText(QString::number(VoltageToDouble(protocol+30),'g',8));
                break;
            case 2:
                this->ui->checkBoxVoltageAverageDataInterval->setChecked(true);
                this->ui->lineEditVoltageAverageDataInterval->setText(QString::number((int)protocol[29]));
                break;
            case 1:
                this->ui->checkBoxVoltageAverageCycle->setChecked(true);
                this->ui->lineEditVoltageAverageCycle->setText(QString::number((int)protocol[28]));
                break;
            }
        }
        i--;
    }

    i=2;
    while(i>0)
    {
        if((protocol[27] & (((int)qPow(2,i-1)) & 0xff)) > 0)
        {
            switch(i)
            {
            case 2:
                this->ui->checkBoxOffLimitAlarm->setChecked(true);
                this->ui->spinBoxOffLimitAlarm->setValue(protocol[46]);
                break;
            case 1:
                this->ui->checkBoxAverageMons->setChecked(true);
                this->ui->spinBoxAverageMons->setValue(protocol[45]);
                break;
            }
        }
        i--;
    }
}

//处理CAC信息报
void MainWindow::handleCACInfo(BYTE *protocol)
{
    int i=3;
    while(i>0)
    {
        if((protocol[25] & (((int)qPow(2,i-1)) & 0xff)) > 0)
        {
            switch(i)
            {
            case 3:
                this->ui->checkBoxIP->setChecked(true);
                this->ui->lineEditIP->setText(byte4IPToString(protocol+26));
                break;
            case 2:
                this->ui->checkBoxPort->setChecked(true);
                this->ui->lineEditPort->setText(QString::number(byte2ToInt(protocol+30)));
                break;
            case 1:
                this->ui->checkBoxPhoneNum_2->setChecked(true);
                this->ui->lineEditPhoneNum->setText((char*)(protocol+32));
                break;
            }
        }
        i--;
    }
}

//处理装置基本信息
void MainWindow::handleDeviceInfo(BYTE *protocol)
{
    char *buff=new char[21];
    memset(buff,'\0',21);
    memcpy(buff,protocol+24,10);

    this->ui->labelVMDModel->setText(QString(buff));
    memset(buff,'\0',21);

    this->ui->labelHardwareVersion->setText(getStringFromProtocol(protocol+34));
    this->ui->labelSoftwareVersion->setText(getStringFromProtocol(protocol+38));
    this->ui->labelProtocolVersion->setText(getStringFromProtocol(protocol+42));

    memcpy(buff,protocol+46,20);

    QTextCodec *codec = QTextCodec::codecForName("GBK");//指定QString的编码方式
    QString str=codec->toUnicode(buff);//con可以是char*，可以是QByteArray。
    this->ui->labelManufacturer->setText(str);
    memset(buff,'\0',21);

    qint64 time=getTimeFromByte(protocol+66);
    this->ui->labelProductionDate->setText(QDateTime::fromMSecsSinceEpoch(time*1000).toString("yyyy-MM-dd hh:mm:ss"));

    qDebug()<<QDateTime::fromMSecsSinceEpoch(time*1000).toString("yyyy-MM-dd hh:mm:ss");
    memcpy(buff,protocol+70,20);
    QTextCodec *codec1 = QTextCodec::codecForName("GBK");//指定QString的编码方式
    QString str1=codec1->toUnicode(buff);//con可以是char*，可以是QByteArray。
    this->ui->labelIdentifier->setText(str1);
    memset(buff,'\0',21);

    memcpy(buff,protocol+90,15);
    this->ui->labelIMSINum->setText(QString(buff));
    memset(buff,'\0',21);

    memcpy(buff,protocol+105,11);
    this->ui->labelPhoneNum->setText(QString(buff));
    memset(buff,'\0',21);
}

QString MainWindow::getStringFromProtocol(BYTE *protocol)
{
    QString str;
    str=QString::number((protocol[0]&0xff));
    str+='.';
    str+=QString::number((protocol[1]&0xff));
    str+='.';
    str+=QString::number((protocol[2]&0xff));
    str+='.';
    str+=QString::number((protocol[3]&0xff));
    return str;
}

//处理时间
void MainWindow::handleTime(BYTE *protocol)
{
    qint64 iTime=getTimeFromByte(protocol+25);
    QString txtTime=QDateTime::fromMSecsSinceEpoch(iTime*1000).toString("yyyy-MM-dd hh:mm:ss");
    ui->lcdNumberDeviceTime->display(txtTime);
}

//处理流量数据报
void MainWindow::handleFlowData(BYTE *protocol)
{
    qint64 time=getTimeFromByte(protocol+24);
    this->ui->labelFlowStartTime->setText(QDateTime::fromMSecsSinceEpoch(time*1000).toString("yyyy-MM-dd hh:mm:ss"));
    this->ui->labelSendFlow->setText(QString::number(byte4ToInt(protocol+28)));
    this->ui->labelReceiveFlow->setText(QString::number(byte4ToInt(protocol+32)));
}

//处理事件
void MainWindow::handleEvent(BYTE *protocol)
{
    int iMeasurePointId=(int)(protocol[24]&0xff);
    qint64 iTime1 = getTimeFromByte(protocol+25);
    int oldRowCount=event_model->rowCount();

    event_model->setItem(oldRowCount, 0, new QStandardItem(QString::number(iMeasurePointId)));
    event_model->setItem(oldRowCount, 1, new QStandardItem(QDateTime::fromMSecsSinceEpoch(iTime1*1000).toString("yyyy-MM-dd hh:mm:ss")));
    event_model->setItem(oldRowCount, 2, new QStandardItem(getEventType((protocol+29)[0])));
    event_model->setItem(oldRowCount, 3, new QStandardItem(QString((char*)(protocol+30))));
}

QString MainWindow::getEventType(BYTE type)
{
    QString eventType;
    switch(type&0xff)
    {
    case 0x01:
        eventType="停电";
        break;
    case 0x02:
        eventType="上电";
        break;
    case 0x03:
        eventType="越上限";
        break;
    case 0x04:
        eventType="越上限回复";
        break;
    case 0x05:
        eventType="越下限";
        break;
    case 0x06:
        eventType="越下限回复";
        break;
    case 0x07:
        eventType="复位";
        break;
    case 0x08:
        eventType="其他";
        break;
    }
    return eventType;
}

//处理数据请求
void MainWindow::handleQueryData(BYTE *protocol)
{
    switch (protocol[25]&0xff)
    {
    case 0x04:
        this->ui->comboBoxResponseDataType->setCurrentIndex(0);
        break;
    case 0x05:
        this->ui->comboBoxResponseDataType->setCurrentIndex(1);
        break;
    case 0x06:
        this->ui->comboBoxResponseDataType->setCurrentIndex(2);
        break;
    default:
        break;
    }
    this->ui->spinBoxResponseMeasurePoint->setValue((int)protocol[24]);
    this->ui->dateTimeEditResponseStartTime->setDateTime(QDateTime::fromMSecsSinceEpoch(getTimeFromByte(protocol+26)*1000));
    this->ui->dateTimeEditResponseEndTime->setDateTime(QDateTime::fromMSecsSinceEpoch(getTimeFromByte(protocol+30)*1000));

}

//处理月统计报
void MainWindow::handleMonthStatistical(BYTE *protocol)
{
    int iMeasureDataNum=(int)(protocol[25]&0xff);
    int iMeasurePointId=(int)(protocol[24]&0xff);

    for(int i=0;i<iMeasureDataNum;i++)
    {
        qint64 iTime1 = getTimeFromByte(protocol+26+i*41);
        monthStatistical_model->setItem(i, 0, new QStandardItem(QString::number(iMeasurePointId)));

        monthStatistical_model->setItem(i, 1, new QStandardItem(QDateTime::fromMSecsSinceEpoch(iTime1*1000).toString("yyyy-MM-dd hh:mm:ss")));
        monthStatistical_model->setItem(i, 2, new QStandardItem(QString::number(VoltageEligibilityRateToDouble(protocol+30+i*41),'g',8)));
        monthStatistical_model->setItem(i, 3, new QStandardItem(QString::number(VoltageEligibilityRateToDouble(protocol+33+i*41),'g',8)));
        monthStatistical_model->setItem(i, 4, new QStandardItem(QString::number(VoltageEligibilityRateToDouble(protocol+36+i*41),'g',8)));

        monthStatistical_model->setItem(i, 5, new QStandardItem(QString::number(((protocol+39+i*41)[0]+(protocol+39+i*41)[1]*qPow(16,2)))));
        monthStatistical_model->setItem(i, 6, new QStandardItem(QString::number(((protocol+41+i*41)[0]+(protocol+41+i*41)[1]*qPow(16,2)))));
        monthStatistical_model->setItem(i, 7, new QStandardItem(QString::number(((protocol+43+i*41)[0]+(protocol+43+i*41)[1]*qPow(16,2)))));
        monthStatistical_model->setItem(i, 8, new QStandardItem(QString::number(((protocol+45+i*41)[0]+(protocol+45+i*41)[1]*qPow(16,2)))));

        monthStatistical_model->setItem(i, 9, new QStandardItem(QString::number(VoltageToDouble(protocol+47+i*41),'g',8)));
        monthStatistical_model->setItem(i, 10, new QStandardItem(QString::number(VoltageToDouble(protocol+51+i*41),'g',8)));
        qint64 iTime2 = getTimeFromByte(protocol+55+i*41);
        monthStatistical_model->setItem(i, 11, new QStandardItem(QDateTime::fromMSecsSinceEpoch(iTime2*1000).toString("yyyy-MM-dd hh:mm:ss")));
        monthStatistical_model->setItem(i, 12, new QStandardItem(QString::number(VoltageToDouble(protocol+59+i*41),'g',8)));
        qint64 iTime3 = getTimeFromByte(protocol+63+i*41);
        monthStatistical_model->setItem(i, 13, new QStandardItem(QDateTime::fromMSecsSinceEpoch(iTime3*1000).toString("yyyy-MM-dd hh:mm:ss")));
    }
}

//处理日统计报
void MainWindow::handleDailyStatistical(BYTE *protocol)
{
    int iMeasureDataNum=(int)(protocol[25]&0xff);
    int iMeasurePointId=(int)(protocol[24]&0xff);

    //dailyStatistical_model->clear();
    int oldRowCount=dailyStatistical_model->rowCount();
    int newRowCount=oldRowCount+iMeasureDataNum;
    for(int i=oldRowCount;i<newRowCount;i++)
    {
        qint64 iTime1 = getTimeFromByte(protocol+26+(i-oldRowCount)*41);
        dailyStatistical_model->setItem(i, 0, new QStandardItem(QString::number(iMeasurePointId)));

        dailyStatistical_model->setItem(i, 1, new QStandardItem(QDateTime::fromMSecsSinceEpoch(iTime1*1000).toString("yyyy-MM-dd hh:mm:ss")));
        dailyStatistical_model->setItem(i, 2, new QStandardItem(QString::number(VoltageEligibilityRateToDouble(protocol+30+(i-oldRowCount)*41),'g',8)));
        dailyStatistical_model->setItem(i, 3, new QStandardItem(QString::number(VoltageEligibilityRateToDouble(protocol+33+(i-oldRowCount)*41),'g',8)));
        dailyStatistical_model->setItem(i, 4, new QStandardItem(QString::number(VoltageEligibilityRateToDouble(protocol+36+(i-oldRowCount)*41),'g',8)));

        dailyStatistical_model->setItem(i, 5, new QStandardItem(QString::number(((protocol+39+(i-oldRowCount)*41)[0]+(protocol+39+(i-oldRowCount)*41)[1]*qPow(16,2)))));
        dailyStatistical_model->setItem(i, 6, new QStandardItem(QString::number(((protocol+41+(i-oldRowCount)*41)[0]+(protocol+41+(i-oldRowCount)*41)[1]*qPow(16,2)))));
        dailyStatistical_model->setItem(i, 7, new QStandardItem(QString::number(((protocol+43+(i-oldRowCount)*41)[0]+(protocol+43+(i-oldRowCount)*41)[1]*qPow(16,2)))));
        dailyStatistical_model->setItem(i, 8, new QStandardItem(QString::number(((protocol+45+(i-oldRowCount)*41)[0]+(protocol+45+(i-oldRowCount)*41)[1]*qPow(16,2)))));

        dailyStatistical_model->setItem(i, 9, new QStandardItem(QString::number(VoltageToDouble(protocol+47+(i-oldRowCount)*41),'g',8)));
        dailyStatistical_model->setItem(i, 10, new QStandardItem(QString::number(VoltageToDouble(protocol+51+(i-oldRowCount)*41),'g',8)));
        qint64 iTime2 = getTimeFromByte(protocol+55+(i-oldRowCount)*41);
        dailyStatistical_model->setItem(i, 11, new QStandardItem(QDateTime::fromMSecsSinceEpoch(iTime2*1000).toString("yyyy-MM-dd hh:mm:ss")));
        dailyStatistical_model->setItem(i, 12, new QStandardItem(QString::number(VoltageToDouble(protocol+59+(i-oldRowCount)*41),'g',8)));
        qint64 iTime3 = getTimeFromByte(protocol+63+(i-oldRowCount)*41);
        dailyStatistical_model->setItem(i, 13, new QStandardItem(QDateTime::fromMSecsSinceEpoch(iTime3*1000).toString("yyyy-MM-dd hh:mm:ss")));
    }

}

//从协议中获取时间
qint64 MainWindow::getTimeFromByte(BYTE *protocol)
{
    qint64 iTime1 = (protocol[0]&0xff) + ((protocol[1])&0xff)*qPow(2,8) + ((protocol[2])&0xff)*qPow(2,16) + ((protocol[3])&0xff)*qPow(2,24);
    return iTime1;
}

//处理电压数据报
void MainWindow::handleVoltageData(BYTE *protocol)
{
    int iMeasureDataNum=(int)(protocol[25]&0xff);
    int iMeasurePointId=(int)(protocol[24]&0xff);
    qDebug()<<"Num:"<<iMeasureDataNum<<" id:"<<iMeasurePointId;

    for(int i=0;i<iMeasureDataNum;i++)
    {        
        qint64 iTime1 = getTimeFromByte(protocol+26+i*8);
        qDebug()<<QDateTime::fromMSecsSinceEpoch(iTime1*1000).toString("yyyy-MM-dd hh:mm:ss");
        qDebug()<<QString::number(VoltageToDouble(&protocol[30+i*8]),'g',8);
        voltageData_model->setItem(i, 0, new QStandardItem(QString::number(iMeasurePointId)));
        voltageData_model->setItem(i, 1, new QStandardItem(QDateTime::fromMSecsSinceEpoch(iTime1*1000).toString("yyyy-MM-dd hh:mm:ss")));
        voltageData_model->setItem(i, 2, new QStandardItem(QString::number(VoltageToDouble(&protocol[30+i*8]),'g',8)));

        voltageData_model->item(i, 0)->setTextAlignment(Qt::AlignCenter);
    }


}

void MainWindow::analysisProtocol(BYTE *protocol, int length)
{
    stProtocolHeader = (ProtocolHeader *)protocol;

    switch(stProtocolHeader->Packet_Type[0]&0xff)
    {
    case 0x01:
        //type="心跳数据报";
        memcpy(deviceID,stProtocolHeader->Device_Code,17);
        break;
    case 0x04:
        //type="电压数据报";
        handleVoltageData(protocol);
        break;
    case 0x05:
        //type="日统计数据报";
        handleDailyStatistical(protocol);
        break;
    case 0x06:
        //type="月统计数据报";
        handleMonthStatistical(protocol);
        break;
    case 0xA1:
       // type="数据请求报";
        handleQueryData(protocol);
        break;
    case 0xA4:
        //type="装置时间查询/设置";
        handleTime(protocol);
        break;
    case 0xA6:
        //type="装置通信参数查询/设置";
        handleConnParam(protocol);
        break;
    case 0xA7:
        //type="监测点参数查询/设置";
        handleMeasurePointParam(protocol);
        break;
    case 0xA8:
        //type="装置事件参数查询/设置";
        handleEventParam(protocol);
        break;
    case 0xA9:
        //type="装置所属CAC的信息查询/设置";
        handleCACInfo(protocol);
        break;
    case 0xAA:
        //type="装置基本信息查询";
        handleDeviceInfo(protocol);
        break;
    case 0xAB:
        //type="装置工作状态信息查询";
        handleWorkState(protocol);
        break;
    case 0xAC:
        //type="装置通信流量信息查询";
        break;
    case 0xAD:
        //type="装置ID查询/设置";
        handleDeviceID(protocol);
        break;
    case 0xAE:
        //type="装置复位";
        break;
    case 0xAF:
        //type="装置调试命令交互";
        handleCMD(protocol);
        break;
    case 0xC1:
        //type="流量数据报";
        handleFlowData(protocol);
        break;
    case 0xC4:
        //type="事件信息报";
        handleEvent(protocol);
        break;
    case 0xC7:
        //type="升级启动数据报";
        break;
    case 0xC8:
        //type="升级过程数据报";
        break;
    case 0xC9:
        //type="升级结束数据报";
        break;
    default:
        ;
    }
}

void MainWindow::slotRevData(BYTE *ba, int length)
{
    analysisProtocol(ba,length);
    QDateTime time=QDateTime::currentDateTime();
    QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
    char deviceID[18]={'\0'};
    memcpy(deviceID,stProtocolHeader->Device_Code,17);

    this->ui->textEditData->insertPlainText(txtTime + " 收到设备" + deviceID + "帧类型：" + getFrameType(stProtocolHeader) + " 报文类型：" +getPacketType(ba)+ "\n" + byteToHexStr(ba,length)+"\n\r");
    responseProtocol();
}

void MainWindow::responseProtocol()
{
    switch(stProtocolHeader->Packet_Type[0]&0xff)
    {
    case 0x01:
        //type="心跳数据报";
        responseHeartBeat(stProtocolHeader);
        break;
    case 0x04:
        //type="电压数据报";
        responseVoltageData(stProtocolHeader);
        break;
    case 0x05:
        //type="日统计数据报";
        responseDailyStatistical(stProtocolHeader);
        break;
    case 0x06:
        //type="月统计数据报";
        responseMonthStatistical(stProtocolHeader);
        break;
    case 0xA1:
        //type="数据请求报";
        break;
    case 0xA4:
        //type="装置时间查询/设置";
        break;
    case 0xA6:
        //type="装置通信参数查询/设置";
        break;
    case 0xA7:
        //type="监测点参数查询/设置";
        break;
    case 0xA8:
        //type="装置时间参数查询/设置";
        break;
    case 0xA9:
        //type="装置所属CAC的信息查询/设置";
        break;
    case 0xAA:
        //type="装置基本信息查询";
        break;
    case 0xAB:
        //type="装置工作状态信息查询";
        break;
    case 0xAC:
        //type="装置通信流量信息查询";
        break;
    case 0xAD:
        //type="装置ID查询/设置";
        break;
    case 0xAE:
        //type="装置复位";
        break;
    case 0xAF:
        //type="装置调试命令交互";
        break;
    case 0xC1:
        //type="流量数据报";
        responseFlowData(stProtocolHeader);
        break;
    case 0xC4:
        //type="事件信息报";
        responseEvent(stProtocolHeader);
        break;
    case 0xC7:
        //type="升级启动数据报";
        break;
    case 0xC8:
        //type="升级过程数据报";
        break;
    case 0xC9:
        //type="升级结束数据报";
        break;
    default:
        ;
    }
}

//回复事件
void MainWindow::responseEvent(ProtocolHeader *header)
{
    BYTE res[29]={'\0'};
    memcpy(res,header,25);

    res[0]=0xA5&0xff;//报文头低位
    res[1]=0x5A&0xff;//报文头高位
    res[3]=0x02&0xff;//报文长度低位
    res[4]=0x00&0xff;//报文长度高位
    res[21]=0x0c&0xff;//帧类型

    res[25]=255&0xff;//发送状态
    BYTE crc[2];
    RTU_CRC(crc,res+2,24);
    memcpy(res+26,crc,2);
    res[28]=(BYTE)(150&0xff);//报文尾
    if(m_server->writeMessage((char*)res,29)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,header->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)res) + " 报文类型："+getPacketType(res)+"\n" + byteToHexStr(res,29)+"\n\r");
    }
}

//回复月统计
void MainWindow::responseMonthStatistical(ProtocolHeader *header)
{
    BYTE res[29]={'\0'};
    memcpy(res,header,25);

    res[0]=0xA5&0xff;//报文头低位
    res[1]=0x5A&0xff;//报文头高位
    res[3]=0x02&0xff;//报文长度低位
    res[4]=0x00&0xff;//报文长度高位
    res[21]=0x04&0xff;//帧类型

    res[25]=255&0xff;//发送状态
    BYTE crc[2];
    RTU_CRC(crc,res+2,24);
    memcpy(res+26,crc,2);
    res[28]=(BYTE)(150&0xff);
    if(m_server->writeMessage((char*)res,29)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,header->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)res) + " 报文类型："+getPacketType(res)+"\n" + byteToHexStr(res,29)+"\n\r");
    }
}

//回复日统计
void MainWindow::responseDailyStatistical(ProtocolHeader *header)
{
    BYTE res[29]={'\0'};
    memcpy(res,header,25);

    res[0]=0xA5&0xff;//报文头低位
    res[1]=0x5A&0xff;//报文头高位
    res[3]=0x02&0xff;//报文长度低位
    res[4]=0x00&0xff;//报文长度高位
    res[21]=0x04&0xff;//帧类型

    res[25]=255&0xff;//发送状态
    //BYTE crc[2];
    BYTE crc[2];
    RTU_CRC(crc,res+2,24);
    memcpy(res+26,crc,2);
    res[28]=(BYTE)(150&0xff);
    if(m_server->writeMessage((char*)res,29)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,header->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)res) + " 报文类型："+getPacketType(res)+"\n" + byteToHexStr(res,29)+"\n\r");
    }
}

//回复流量数据包
void MainWindow::responseFlowData(ProtocolHeader *header)
{
    BYTE res[28]={'\0'};
    memcpy(res,header,25);

    res[0]=0xA5&0xff;//报文头低位
    res[1]=0x5A&0xff;//报文头高位
    res[3]=0x01&0xff;//报文长度低位
    res[4]=0x00&0xff;//报文长度高位
    res[21]=0x10&0xff;//帧类型

    res[24]=255&0xff;//发送状态
    //BYTE *crc=RTU_CRC(res+2,23);
    BYTE crc[2];
    RTU_CRC(crc,res+2,24);
    memcpy(res+25,crc,2);
    res[27]=(BYTE)(150&0xff);//报文尾
    if(m_server->writeMessage((char*)res,28)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,header->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)res) + " 报文类型："+getPacketType(res)+"\n" + byteToHexStr(res,29)+"\n\r");
    }
}

//回复电压数据报
void MainWindow::responseVoltageData(ProtocolHeader *header)
{
    BYTE res[29]={'\0'};
    memcpy(res,header,25);

    res[0]=0xA5&0xff;//报文头低位
    res[1]=0x5A&0xff;//报文头高位
    res[3]=0x02&0xff;//报文长度低位
    res[4]=0x00&0xff;//报文长度高位
    res[21]=0x04&0xff;//帧类型

    res[25]=255&0xff;//发送状态

    BYTE crc[2];
    RTU_CRC(crc,res+2,24);
    memcpy(res+26,crc,2);
    res[28]=(BYTE)(150&0xff);
    if(m_server->writeMessage((char*)res,29)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,header->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)res) + " 报文类型："+getPacketType(res)+"\n" + byteToHexStr(res,29)+"\n\r");
    }
}

BYTE *MainWindow::getCurrentTime()
{
    BYTE *time=new BYTE[4];
    memset(time,'\0',4);
    qint64 iTimeDiff=(QDateTime(QDate(1970,1,1),QTime(0,0),Qt::OffsetFromUTC)).secsTo(QDateTime::currentDateTime());
    qDebug()<<"second:"<<iTimeDiff;
    qDebug()<<QString::number(iTimeDiff,16);
    time[0] = (BYTE)(iTimeDiff&0xFF); //设置为当前时间
    time[1] = (BYTE)((iTimeDiff >>8)&0xFF);
    time[2] = (BYTE)((iTimeDiff >>16)&0xFF);
    time[3] = (BYTE)((iTimeDiff >>24)&0xFF);
    return time;
}

//将datetime转化为Byte*
BYTE *MainWindow::formateFromDateTimeToBYTE(QDateTime dt)
{
    BYTE *time=new BYTE[4];
    memset(time,'\0',4);
    qint64 iTimeDiff=(QDateTime(QDate(1970,1,1),QTime(0,0),Qt::OffsetFromUTC)).secsTo(dt);
    time[0] = (BYTE)(iTimeDiff&0xFF);
    time[1] = (BYTE)((iTimeDiff >>8)&0xFF);
    time[2] = (BYTE)((iTimeDiff >>16)&0xFF);
    time[3] = (BYTE)((iTimeDiff >>24)&0xFF);
    return time;
}

void MainWindow::responseHeartBeat(ProtocolHeader *header)
{
    BYTE res[32]={'\0'};
    memcpy(res,header,sizeof(ProtocolHeader));

    res[0]=0xA5&0xff;//报文头低位
    res[1]=0x5A&0xff;//报文头高位
    res[3]=0x05&0xff;//报文长度低位
    res[4]=0x00&0xff;//报文长度高位
    res[21]=0x02&0xff;//帧类型
    res[24]=255&0xff;//发送状态

    qint64 iTimeDiff=(QDateTime(QDate(1970,1,1),QTime(0,0),Qt::OffsetFromUTC)).secsTo(QDateTime::currentDateTime());
    qDebug()<<"second:"<<iTimeDiff;
    qDebug()<<QString::number(iTimeDiff,16);
    res[25] = (BYTE)(iTimeDiff&0xFF); //设置为当前时间
    res[26] = (BYTE)((iTimeDiff >>8)&0xFF);
    res[27] = (BYTE)((iTimeDiff >>16)&0xFF);
    res[28] = (BYTE)((iTimeDiff >>24)&0xFF);
    BYTE crc[2];
    RTU_CRC(crc,res+2,27);
    memcpy(res+29,crc,2);
    res[31]=(BYTE)(150&0xff);
    if(m_server->writeMessage((char*)res,32)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,header->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)res) + " 报文类型："+getPacketType(res)+"\n" + byteToHexStr(res,32)+"\n\r");
    }
}

QString MainWindow::getPacketType(BYTE *protocol)
{
    QString type;
    switch(((ProtocolHeader *)protocol)->Packet_Type[0]&0xff)
    {
    case 0x01:
        type = "心跳数据报";
        break;
    case 0x04:
        type = "电压数据报";
        break;
    case 0x05:
        type = "日统计数据报";
        break;
    case 0x06:
        type = "月统计数据报";
        break;
    case 0xA1:
        type = "数据请求报";
        break;
    case 0xA4:
        type = "装置时间查询/设置";
        if((int)(protocol[24]&0xff) == 0x00)
        {
            type = "装置时间查询";
        }
        else
        {
            type = "装置时间设置";
            if((int)(protocol[21]&0xff)%2 == 0)//响应数据报  装置==>CAC
            {
                type="装置时间";
            }
//            else
//            {
//                type="装置时间设置";
//            }
        }
        break;
    case 0xA6:
        type = "装置通信参数查询/设置";
        if((int)(protocol[24]&0xff) == 0x00)
        {
            type = "装置通信参数查询";
        }
        else
        {
            type = "装置通信参数设置";
            if((int)(protocol[21]&0xff)%2 == 0)//响应数据报  装置==>CAC
            {
                type="装置通信参数";
            }
        }
        break;
    case 0xA7:
        type = "监测点参数查询/设置";
        if((int)(protocol[25]&0xff) == 0x00)
        {
            type = "监测点参数查询";
        }
        else
        {
            type = "监测点参数设置";
            if((int)(protocol[21]&0xff)%2 == 0)//响应数据报  装置==>CAC
            {
                type="监测点参数";
            }
        }
        break;
    case 0xA8:
        type = "装置事件参数查询/设置";
        if((int)(protocol[24]&0xff) == 0x00)
        {
            type = "装置事件参数查询";
        }
        else
        {
            type = "装置事件参数设置";
            if((int)(protocol[21]&0xff)%2 == 0)//响应数据报  装置==>CAC
            {
                type="装置事件参数";
            }
        }
        break;
    case 0xA9:
        type = "装置所属CAC的信息查询/设置";
        if((int)(protocol[24]&0xff) == 0x00)
        {
            type = "装置所属CAC的信息查询";
        }
        else
        {
            type = "装置所属CAC的信息设置";
            if((int)(protocol[21]&0xff)%2 == 0)//响应数据报  装置==>CAC
            {
                type="装置所属CAC的信息";
            }
        }
        break;
    case 0xAA:
        type = "装置基本信息查询";
        break;
    case 0xAB:
        type = "装置工作状态信息查询";
        break;
    case 0xAC:
        type = "装置通信流量信息查询";
        break;
    case 0xAD:
        type = "装置ID查询/设置";
        if((int)(protocol[24]&0xff) == 0x00)
        {
            type = "装置ID查询";
        }
        else
        {
            type = "装置ID设置";
            if((int)(protocol[21]&0xff)%2 == 0)//响应数据报  装置==>CAC
            {
                type="装置ID";
            }
        }
        break;
    case 0xAE:
        type = "装置复位";
        break;
    case 0xAF:
        type = "装置调试命令交互";
        break;
    case 0xC1:
        type = "流量数据报";
        break;
    case 0xC4:
        type = "事件信息报";
        break;
    case 0xC7:
        type = "升级启动数据报";
        break;
    case 0xC8:
        type = "升级过程数据报";
        break;
    case 0xC9:
        type = "升级结束数据报";
        break;
    default:
        ;
    }
    return type;

}

QString MainWindow::getFrameType(ProtocolHeader *header)
{
    QString type;
    //将接收到的header复制到确认header中以便发送
    memcpy(stSndProtocolHeader,stProtocolHeader,sizeof(ProtocolHeader));
    switch(header->Frame_Type[0]&0xff)
    {
    case 0x01:
        type="心跳数据报文";
        break;
    case 0x02:
        type="心跳数据确认报文";
        break;
    case 0x03:
        type="监测数据报文";
        break;
    case 0x04:
        type="监测数据确认报文";
        break;
    case 0x05:
        type="数据请求报文";
        break;
    case 0x06:
        type="数据请求确认报文";
        break;
    case 0x07:
        type="配置/状态交互数据报文";
        break;
    case 0x08:
        type="配置/状态交互响应报文";
        break;
    case 0x09:
        type="流量数据报文";
        break;
    case 0x10:
        type="流量数据确认报文";
        break;
    case 0x11:
        type="事件信息报文";
        break;
    case 0x12:
        type="事件信息确认报文";
        break;
    case 0x13:
        type="远程升级数据报文";
        break;
    case 0x14:
        type="远程升级数据确认报文";
        break;
    default:
        ;
    }
    return type;
}

void MainWindow::changeState()
{
    if(m_openFlag)//设置显示在按钮上的字 ‘打开‘  ’关闭‘
    {
        ui->pushButtonOpen->setText("关闭");
        QPixmap pixmap(":/images/greenlight.png");
        ui->labelOpenClose->setPixmap(pixmap);
    }
    else
    {
        ui->pushButtonOpen->setText("打开");
        QPixmap pixmap(":/images/redlight.png");
        ui->labelOpenClose->setPixmap(pixmap);
    }
}

void MainWindow::on_pushButtonQueryTime_clicked()
{
    BYTE *queryTime=new BYTE[32];
    queryTime[0]=0xA5&0xff;//报文头低位
    queryTime[1]=0x5A&0xff;//报文头高位
    queryTime[2]=0x05&0xff;//报文长度低字节
    queryTime[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(queryTime+4,deviceID,17);//设备ID
    queryTime[21]=0x07&0xff;//帧类型
    queryTime[22]=0xa4&0xff;//报文类型
    queryTime[23]=0x01&0xff;//帧序列号
    queryTime[24]=0x00&0xff;//参数配置类型标志
    BYTE crc[2];
    RTU_CRC(crc,queryTime+2,27);
    memcpy(queryTime+25,getCurrentTime(),4);//设置时间
    memcpy(queryTime+29,crc,2);
    queryTime[31]=0x96&0xff;

    this->ui->tabWidget->setCurrentIndex(3);//切换tab

    if(m_server == NULL)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    if(m_server->writeMessage((char*)queryTime,32)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)queryTime)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)queryTime) + " 报文类型："+getPacketType(queryTime)+"\n" + byteToHexStr(queryTime,29)+"\n\r");
    }
}

void MainWindow::on_pushButtonSetTime_clicked()
{
    BYTE *queryTime=new BYTE[32];
    queryTime[0]=0xA5&0xff;//报文头低位
    queryTime[1]=0x5A&0xff;//报文头高位
    queryTime[2]=0x05&0xff;//报文长度低字节
    queryTime[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(queryTime+4,deviceID,17);//设备ID
    queryTime[21]=0x07&0xff;//帧类型
    queryTime[22]=0xa4&0xff;//报文类型
    queryTime[23]=0x01&0xff;//帧序列号
    queryTime[24]=0x01&0xff;//参数配置类型标志
    BYTE crc[2];
    RTU_CRC(crc,queryTime+2,27);
    memcpy(queryTime+25,getCurrentTime(),4);//设置时间
    memcpy(queryTime+29,crc,2);
    queryTime[31]=0x96&0xff;

    this->ui->tabWidget->setCurrentIndex(3);//切换tab

    if(m_server == NULL)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    if(m_server->writeMessage((char*)queryTime,32)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)queryTime)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)queryTime) + " 报文类型："+getPacketType(queryTime)+"\n" + byteToHexStr(queryTime,29)+"\n\r");
    }

}

void MainWindow::on_pushButtonClearRcv_clicked()
{
    this->ui->textEditData->clear();
}

void MainWindow::on_comboBoxConnMode_currentIndexChanged(const QString &arg1)
{
}

void MainWindow::on_comboBoxConnMode_currentIndexChanged(int index)
{
    if(this->ui->comboBoxConnMode->currentData().toInt() == 0)//无线专网
    {
        this->ui->groupBoxWirelessConn->setVisible(true);
        this->ui->groupBoxMessageConn->setVisible(false);
    }
    else//短信
    {
        this->ui->groupBoxWirelessConn->setVisible(false);
        this->ui->groupBoxMessageConn->setVisible(true);
    }
}

void MainWindow::on_pushButtonQueryConParam_clicked()
{
    BYTE *measurePintParam=new BYTE[109];
    memset(measurePintParam,'\0',109);
    measurePintParam[0]=0xA5&0xff;//报文头低位
    measurePintParam[1]=0x5A&0xff;//报文头高位
    measurePintParam[2]=0x52&0xff;//报文长度低字节
    measurePintParam[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(measurePintParam+4,deviceID,17);//设备ID
    measurePintParam[21]=0x07&0xff;//帧类型
    measurePintParam[22]=0xa6&0xff;//报文类型
    measurePintParam[23]=0x06&0xff;//帧序列号

    measurePintParam[24]=0x00&0xff;//查询
    measurePintParam[25]=0x00&0xff;//标识

    if(this->ui->groupBoxConMode->isChecked())
    {
        measurePintParam[25]=(measurePintParam[25] | 0x01);
        if(this->ui->comboBoxConnMode->currentData().toInt()==0)
        {//通信方式,无线
            measurePintParam[26] = 0x00&0xff;
            if(this->ui->checkBoxAPN->isChecked())
            {//无线通信APN
                measurePintParam[25]=(measurePintParam[25] | 0x02);
                memcpy(measurePintParam+27,this->ui->lineEditAPN->text().toLatin1().data(),this->ui->lineEditAPN->text().size());
            }
            if(this->ui->checkBoxAPNName->isChecked())
            {//APN用户名
                measurePintParam[25]=(measurePintParam[25] | 0x04);
                memcpy(measurePintParam+43,this->ui->lineEditAPNName->text().toLatin1().data(),this->ui->lineEditAPNName->text().size());
            }
            if(this->ui->checkBoxAPNPassword->isChecked())
            {//APN密码
                measurePintParam[25]=(measurePintParam[25] | 0x08);
                memcpy(measurePintParam+64,this->ui->lineEditAPNPassword->text().toLatin1().data(),this->ui->lineEditAPNPassword->text().size());
            }
            if(this->ui->checkBoxHeartBeat->isChecked())
            {//心跳间隔
                measurePintParam[25]=(measurePintParam[25] | 0x10);
                measurePintParam[85]=(this->ui->lineEditHeartBeat->text().toInt())&0xff;
            }
        }
        else
        {//短信
            measurePintParam[26] = 0x02&0xff;
            if(this->ui->checkBoxPhoneNum->isChecked())
            {
                measurePintParam[25]=(measurePintParam[25] | 0x20);
                memcpy(measurePintParam+86,this->ui->lineEditPhoneNum_2->text().toLatin1().data(),this->ui->lineEditPhoneNum_2->text().size());
            }
        }
    }

    BYTE crc[2];
    RTU_CRC(crc,measurePintParam+2,82);
    memcpy(measurePintParam+106,crc,2);
    measurePintParam[108]=0x96&0xff;

    if(m_server->writeMessage((char*)measurePintParam,109)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)measurePintParam)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)measurePintParam) + " 报文类型："+getPacketType(measurePintParam)+"\n" + byteToHexStr(measurePintParam,109)+"\n\r");
    }
    this->ui->tabWidget->setCurrentIndex(4);
}

void MainWindow::on_pushButtonSetConParam_clicked()
{
    BYTE *measurePintParam=new BYTE[109];
    memset(measurePintParam,'\0',109);
    measurePintParam[0]=0xA5&0xff;//报文头低位
    measurePintParam[1]=0x5A&0xff;//报文头高位
    measurePintParam[2]=0x52&0xff;//报文长度低字节
    measurePintParam[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(measurePintParam+4,deviceID,17);//设备ID
    measurePintParam[21]=0x07&0xff;//帧类型
    measurePintParam[22]=0xa6&0xff;//报文类型
    measurePintParam[23]=0x06&0xff;//帧序列号

    measurePintParam[24]=0x00&0xff;//查询
    measurePintParam[25]=0x00&0xff;//标识

    if(this->ui->groupBoxConMode->isChecked())
    {
        measurePintParam[25]=(measurePintParam[25] | 0x01);
        if(this->ui->comboBoxConnMode->currentData().toInt()==0)
        {//通信方式,无线
            measurePintParam[26] = 0x00&0xff;
            if(this->ui->checkBoxAPN->isChecked())
            {//无线通信APN
                measurePintParam[25]=(measurePintParam[25] | 0x02);
                memcpy(measurePintParam+27,this->ui->lineEditAPN->text().toLatin1().data(),this->ui->lineEditAPN->text().size());
            }
            if(this->ui->checkBoxAPNName->isChecked())
            {//APN用户名
                measurePintParam[25]=(measurePintParam[25] | 0x04);
                memcpy(measurePintParam+43,this->ui->lineEditAPNName->text().toLatin1().data(),this->ui->lineEditAPNName->text().size());
            }
            if(this->ui->checkBoxAPNPassword->isChecked())
            {//APN密码
                measurePintParam[25]=(measurePintParam[25] | 0x08);
                memcpy(measurePintParam+64,this->ui->lineEditAPNPassword->text().toLatin1().data(),this->ui->lineEditAPNPassword->text().size());
            }
            if(this->ui->checkBoxHeartBeat->isChecked())
            {//心跳间隔
                measurePintParam[25]=(measurePintParam[25] | 0x10);
                measurePintParam[85]=(this->ui->lineEditHeartBeat->text().toInt())&0xff;
            }
        }
        else
        {//短信
            measurePintParam[26] = 0x02&0xff;
            if(this->ui->checkBoxPhoneNum->isChecked())
            {
                measurePintParam[25]=(measurePintParam[25] | 0x20);
                memcpy(measurePintParam+86,this->ui->lineEditPhoneNum_2->text().toLatin1().data(),this->ui->lineEditPhoneNum_2->text().size());
            }
        }
    }

    BYTE crc[2];
    RTU_CRC(crc,measurePintParam+2,82);
    memcpy(measurePintParam+106,crc,2);
    measurePintParam[108]=0x96&0xff;

    if(m_server == NULL)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    if(m_server->writeMessage((char*)measurePintParam,109)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)measurePintParam)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)measurePintParam) + " 报文类型："+getPacketType(measurePintParam)+"\n" + byteToHexStr(measurePintParam,109)+"\n\r");
    }
    this->ui->tabWidget->setCurrentIndex(4);
}

void MainWindow::on_pushButtonQueryMeasureParam_clicked()
{
    BYTE *measurePintParam=new BYTE[50];
    memset(measurePintParam,'\0',50);
    measurePintParam[0]=0xA5&0xff;//报文头低位
    measurePintParam[1]=0x5A&0xff;//报文头高位
    measurePintParam[2]=0x17&0xff;//报文长度低字节
    measurePintParam[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(measurePintParam+4,deviceID,17);//设备ID
    measurePintParam[21]=0x07&0xff;//帧类型
    measurePintParam[22]=0xa7&0xff;//报文类型
    measurePintParam[23]=0x06&0xff;//帧序列号
    measurePintParam[24]=0x00&0xff;//监测点号
    measurePintParam[25]=0x00&0xff;//查询
    measurePintParam[26]=0x00&0xff;//标识
    measurePintParam[27]=0x00&0xff;//标识
    if(this->ui->checkBoxVoltageAverageCycle->isChecked())
    {//电压平均值上送周期
        measurePintParam[26]=(measurePintParam[26] | 0x01);
        measurePintParam[28]=this->ui->lineEditVoltageAverageCycle->text().toInt();
    }
    if(this->ui->checkBoxVoltageAverageDataInterval->isChecked())
    {//电压平均值的数据间隔
        measurePintParam[26]=(measurePintParam[26] | 0x02);
        measurePintParam[29]=this->ui->lineEditVoltageAverageDataInterval->text().toInt();
    }
    if(this->ui->checkBoxVoltageULimit->isChecked())
    {//电压上限值
        measurePintParam[26]=(measurePintParam[26] | 0x04);
        memcpy(measurePintParam+30,doubleToVoltage(this->ui->lineEditVoltageULimit->text().toDouble()),4);
    }
    if(this->ui->checkBoxVoltageLLimit->isChecked())
    {//电压下限值
        measurePintParam[26]=(measurePintParam[26] | 0x08);
        memcpy(measurePintParam+34,doubleToVoltage(this->ui->lineEditVoltageLLimit->text().toDouble()),4);
    }

    if(this->ui->checkBoxMonthSettle->isChecked())
    {//月结算日
        measurePintParam[26]=(measurePintParam[26] | 0x10);
        measurePintParam[38]=this->ui->spinBoxMonthSettle->text().toInt();
    }
    if(this->ui->checkBoxVoltageGrade->isChecked())
    {//电压等级
        measurePintParam[26]=(measurePintParam[26] | 0x20);
        memcpy(measurePintParam+39,doubleToVoltage(this->ui->lineEditVoltageGrade->text().toDouble()),4);
    }
    if(this->ui->checkBoxAverageMins->isChecked())
    {//分钟平均值主动上报标志
        measurePintParam[26]=(measurePintParam[26] | 0x40);
        measurePintParam[43]=this->ui->spinBoxAverageMins->text().toInt();
    }
    if(this->ui->checkBoxAverageDay->isChecked())
    {//日数据主动上报标志
        measurePintParam[26]=(measurePintParam[26] | 0x80);
        measurePintParam[44]=this->ui->spinBoxAverageDay->text().toInt();
    }
    if(this->ui->checkBoxAverageMons->isChecked())
    {//月数据主动上报标志
        measurePintParam[27]=(measurePintParam[27] | 0x01);
        measurePintParam[45]=this->ui->spinBoxAverageMons->text().toInt();
    }
    if(this->ui->checkBoxOffLimitAlarm->isChecked())
    {//越限告警主动上报标志
        measurePintParam[27]=(measurePintParam[27] | 0x02);
        measurePintParam[46]=this->ui->spinBoxOffLimitAlarm->text().toInt();
    }

    qDebug()<<(int)measurePintParam[25];
    BYTE crc[2];
    RTU_CRC(crc,measurePintParam+2,45);
    memcpy(measurePintParam+47,crc,2);
    measurePintParam[49]=0x96&0xff;

    this->ui->tabWidget->setCurrentIndex(3);//切换tab

    if(m_server->writeMessage((char*)measurePintParam,50)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)measurePintParam)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)measurePintParam) + " 报文类型："+getPacketType(measurePintParam)+"\n" + byteToHexStr(measurePintParam,50)+"\n\r");
    }
    this->ui->tabWidget->setCurrentIndex(4);
}

void MainWindow::on_pushButtonSetMeasureParam_clicked()
{
    BYTE *measurePintParam=new BYTE[50];
    memset(measurePintParam,'\0',50);
    measurePintParam[0]=0xA5&0xff;//报文头低位
    measurePintParam[1]=0x5A&0xff;//报文头高位
    measurePintParam[2]=0x17&0xff;//报文长度低字节
    measurePintParam[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(measurePintParam+4,deviceID,17);//设备ID
    measurePintParam[21]=0x07&0xff;//帧类型
    measurePintParam[22]=0xa7&0xff;//报文类型
    measurePintParam[23]=0x06&0xff;//帧序列号
    measurePintParam[24]=0x00&0xff;//监测点号
    measurePintParam[25]=0x01&0xff;//设置
    measurePintParam[26]=0x00&0xff;//标识
    measurePintParam[27]=0x00&0xff;//标识
    if(this->ui->checkBoxVoltageAverageCycle->isChecked())
    {//电压平均值上送周期
        measurePintParam[26]=(measurePintParam[26] | 0x01);
        measurePintParam[28]=this->ui->lineEditVoltageAverageCycle->text().toInt();
    }
    if(this->ui->checkBoxVoltageAverageDataInterval->isChecked())
    {//电压平均值的数据间隔
        measurePintParam[26]=(measurePintParam[26] | 0x02);
        measurePintParam[29]=this->ui->lineEditVoltageAverageDataInterval->text().toInt();
    }
    if(this->ui->checkBoxVoltageULimit->isChecked())
    {//电压上限值
        measurePintParam[26]=(measurePintParam[26] | 0x04);
        memcpy(measurePintParam+30,doubleToVoltage(this->ui->lineEditVoltageULimit->text().toDouble()),4);
    }
    if(this->ui->checkBoxVoltageLLimit->isChecked())
    {//电压下限值
        measurePintParam[26]=(measurePintParam[26] | 0x08);
        memcpy(measurePintParam+34,doubleToVoltage(this->ui->lineEditVoltageLLimit->text().toDouble()),4);
    }

    if(this->ui->checkBoxMonthSettle->isChecked())
    {//月结算日
        measurePintParam[26]=(measurePintParam[26] | 0x10);
        measurePintParam[38]=this->ui->spinBoxMonthSettle->text().toInt();
    }
    if(this->ui->checkBoxVoltageGrade->isChecked())
    {//电压等级
        measurePintParam[26]=(measurePintParam[26] | 0x20);
        memcpy(measurePintParam+39,doubleToVoltage(this->ui->lineEditVoltageGrade->text().toDouble()),4);
    }
    if(this->ui->checkBoxAverageMins->isChecked())
    {//分钟平均值主动上报标志
        measurePintParam[26]=(measurePintParam[26] | 0x40);
        measurePintParam[43]=this->ui->spinBoxAverageMins->text().toInt();
    }
    if(this->ui->checkBoxAverageDay->isChecked())
    {//日数据主动上报标志
        measurePintParam[26]=(measurePintParam[26] | 0x80);
        measurePintParam[44]=this->ui->spinBoxAverageDay->text().toInt();
    }
    if(this->ui->checkBoxAverageMons->isChecked())
    {//月数据主动上报标志
        measurePintParam[27]=(measurePintParam[27] | 0x01);
        measurePintParam[45]=this->ui->spinBoxAverageMons->text().toInt();
    }
    if(this->ui->checkBoxOffLimitAlarm->isChecked())
    {//越限告警主动上报标志
        measurePintParam[27]=(measurePintParam[27] | 0x02);
        measurePintParam[46]=this->ui->spinBoxOffLimitAlarm->text().toInt();
    }

    qDebug()<<(int)measurePintParam[25];
    BYTE crc[2];
    RTU_CRC(crc,measurePintParam+2,45);
    memcpy(measurePintParam+47,crc,2);
    measurePintParam[49]=0x96&0xff;

    this->ui->tabWidget->setCurrentIndex(3);//切换tab

    if(m_server == NULL)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    if(m_server->writeMessage((char*)measurePintParam,50)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)measurePintParam)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)measurePintParam) + " 报文类型："+getPacketType(measurePintParam)+"\n" + byteToHexStr(measurePintParam,50)+"\n\r");
    }
    this->ui->tabWidget->setCurrentIndex(4);
}

void MainWindow::on_pushButtonQueryCAC_clicked()
{
    BYTE *CACInfo=new BYTE[55];
    memset(CACInfo,'\0',55);
    CACInfo[0]=0xA5&0xff;//报文头低位
    CACInfo[1]=0x5A&0xff;//报文头高位
    CACInfo[2]=0x1c&0xff;//报文长度低字节
    CACInfo[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(CACInfo+4,deviceID,17);//设备ID
    CACInfo[21]=0x07&0xff;//帧类型
    CACInfo[22]=0xa9&0xff;//报文类型
    CACInfo[23]=0x06&0xff;//帧序列号
    CACInfo[24]=0x00&0xff;//查询
    CACInfo[25]=0x00&0xff;//标识位
    if(this->ui->checkBoxIP->isChecked())
    {
        CACInfo[25]=(CACInfo[25] | 0x01);
    }
    if(this->ui->checkBoxPort->isChecked())
    {
        CACInfo[25]=(CACInfo[25] | 0x02);
    }

    if(this->ui->checkBoxPhoneNum_2->isChecked())
    {
        CACInfo[25]=(CACInfo[25] | 0x04);
    }

    qDebug()<<(int)CACInfo[25];
    BYTE crc[2];
    RTU_CRC(crc,CACInfo+2,50);
    memcpy(CACInfo+52,crc,2);
    CACInfo[54]=0x96&0xff;

    this->ui->tabWidget->setCurrentIndex(3);//切换tab

    if(m_server->writeMessage((char*)CACInfo,27)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)CACInfo)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)CACInfo) + " 报文类型："+getPacketType(CACInfo)+"\n" + byteToHexStr(CACInfo,29)+"\n\r");
    }
    this->ui->tabWidget->setCurrentIndex(3);
}

void MainWindow::on_pushButtonSetCAC_clicked()
{
    BYTE *CACInfo=new BYTE[55];
    memset(CACInfo,'\0',55);
    CACInfo[0]=0xA5&0xff;//报文头低位
    CACInfo[1]=0x5A&0xff;//报文头高位
    CACInfo[2]=0x1c&0xff;//报文长度低字节
    CACInfo[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(CACInfo+4,deviceID,17);//设备ID
    CACInfo[21]=0x07&0xff;//帧类型
    CACInfo[22]=0xa9&0xff;//报文类型
    CACInfo[23]=0x06&0xff;//帧序列号
    CACInfo[24]=0x01&0xff;//设置
    CACInfo[25]=0x00&0xff;//标识位
    if(this->ui->checkBoxIP->isChecked())
    {
        CACInfo[25]=(CACInfo[25] | 0x01);
        memcpy(CACInfo+26,QStringIPToByte(this->ui->lineEditIP->text()),4);
    }
    if(this->ui->checkBoxPort->isChecked())
    {
        CACInfo[25]=(CACInfo[25] | 0x02);
        memcpy(CACInfo+30,intToByte2(this->ui->lineEditPort->text().toInt()),2);
    }

    if(this->ui->checkBoxPhoneNum_2->isChecked())
    {
        CACInfo[25]=(CACInfo[25] | 0x04);
        char * str=new char[20];
        memset(str,'\0',20);
        qDebug()<<this->ui->lineEditPhoneNum->text().data()<<" "<<this->ui->lineEditPhoneNum->text().size();
        qDebug()<<this->ui->lineEditPhoneNum->text().toLatin1().data();
        memcpy(str,this->ui->lineEditPhoneNum->text().toLatin1().data(),this->ui->lineEditPhoneNum->text().size());
        memcpy(CACInfo+32,str,20);
        delete str;
        str=NULL;
    }

    BYTE crc[2];
    RTU_CRC(crc,CACInfo+2,50);
    memcpy(CACInfo+52,crc,2);
    CACInfo[54]=0x96&0xff;

    this->ui->tabWidget->setCurrentIndex(3);//切换tab

    if(m_server == NULL)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    if(m_server->writeMessage((char*)CACInfo,55)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)CACInfo)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)CACInfo) + " 报文类型："+getPacketType(CACInfo)+"\n" + byteToHexStr(CACInfo,55)+"\n\r");
    }
    this->ui->tabWidget->setCurrentIndex(3);
}

void MainWindow::on_pushButtonQueryWorkState_clicked()
{

    BYTE *deviceInfo=new BYTE[27];
    deviceInfo[0]=0xA5&0xff;//报文头低位
    deviceInfo[1]=0x5A&0xff;//报文头高位
    deviceInfo[2]=0x00&0xff;//报文长度低字节
    deviceInfo[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(deviceInfo+4,deviceID,17);//设备ID
    deviceInfo[21]=0x07&0xff;//帧类型
    deviceInfo[22]=0xab&0xff;//报文类型
    deviceInfo[23]=0x06&0xff;//帧序列号

    BYTE crc[2];
    RTU_CRC(crc,deviceInfo+2,22);
    memcpy(deviceInfo+24,crc,2);
    deviceInfo[26]=0x96&0xff;

    this->ui->tabWidget->setCurrentIndex(3);//切换tab

    if(m_server->writeMessage((char*)deviceInfo,27)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)deviceInfo)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)deviceInfo) + " 报文类型："+getPacketType(deviceInfo)+"\n" + byteToHexStr(deviceInfo,29)+"\n\r");
    }
}

void MainWindow::on_pushButtonQueryID_clicked()
{
    BYTE *deviceInfo=new BYTE[62];
    memset(deviceInfo,'\0',62);
    deviceInfo[0]=0xA5&0xff;//报文头低位
    deviceInfo[1]=0x5A&0xff;//报文头高位
    deviceInfo[2]=0x23&0xff;//报文长度低字节
    deviceInfo[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(deviceInfo+4,deviceID,17);//设备ID
    deviceInfo[21]=0x07&0xff;//帧类型
    deviceInfo[22]=0xad&0xff;//报文类型
    deviceInfo[23]=0x06&0xff;//帧序列号
    deviceInfo[24]=0x00&0xff;//查询

    memcpy(deviceInfo+25,this->ui->labelOriginalID->text().toLatin1().data(),17);
    memcpy(deviceInfo+42,this->ui->lineEditNewDeviceCode->text().toLatin1().data(),17);

    BYTE crc[2];
    RTU_CRC(crc,deviceInfo+2,22);
    memcpy(deviceInfo+59,crc,2);//校验位
    deviceInfo[61]=0x96&0xff;//结束位

    this->ui->tabWidget->setCurrentIndex(5);//切换tab

    if(m_server == NULL)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    if(m_server->writeMessage((char*)deviceInfo,62)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)deviceInfo)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)deviceInfo) + " 报文类型："+getPacketType(deviceInfo)+"\n" + byteToHexStr(deviceInfo,62)+"\n\r");
    }
}

void MainWindow::on_pushButtonSetID_clicked()
{
    BYTE *deviceInfo=new BYTE[62];
    memset(deviceInfo,'\0',62);
    deviceInfo[0]=0xA5&0xff;//报文头低位
    deviceInfo[1]=0x5A&0xff;//报文头高位
    deviceInfo[2]=0x00&0xff;//报文长度低字节
    deviceInfo[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(deviceInfo+4,deviceID,17);//设备ID
    deviceInfo[21]=0x07&0xff;//帧类型
    deviceInfo[22]=0xad&0xff;//报文类型
    deviceInfo[23]=0x06&0xff;//帧序列号
    deviceInfo[24]=0x01&0xff;//查询

    memcpy(deviceInfo+25,this->ui->labelOriginalID->text().toLatin1().data(),17);
    memcpy(deviceInfo+42,this->ui->lineEditNewDeviceCode->text().toLatin1().data(),17);

    BYTE crc[2];
    RTU_CRC(crc,deviceInfo+2,22);
    memcpy(deviceInfo+59,crc,2);//校验位
    deviceInfo[61]=0x96&0xff;//结束位

    this->ui->tabWidget->setCurrentIndex(5);//切换tab

    if(m_server == NULL)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    if(m_server->writeMessage((char*)deviceInfo,62)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)deviceInfo)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)deviceInfo) + " 报文类型："+getPacketType(deviceInfo)+"\n" + byteToHexStr(deviceInfo,62)+"\n\r");
    }
    this->ui->tabWidget->setCurrentIndex(3);
}

void MainWindow::on_pushButtonDeviceReset_clicked()
{
    BYTE *deviceInfo=new BYTE[28];
    memset(deviceInfo,'\0',28);
    deviceInfo[0]=0xA5&0xff;//报文头低位
    deviceInfo[1]=0x5A&0xff;//报文头高位
    deviceInfo[2]=0x01&0xff;//报文长度低字节
    deviceInfo[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(deviceInfo+4,deviceID,17);//设备ID
    deviceInfo[21]=0x07&0xff;//帧类型
    deviceInfo[22]=0xae&0xff;//报文类型
    deviceInfo[23]=0x06&0xff;//帧序列号

    if(this->ui->comboBoxReset->currentData().toInt() == 0)
    {
        deviceInfo[24]=0x00&0xff;//复位至正常模式
    }
    else
    {
        deviceInfo[24]=0x01&0xff;//复位至调试模式
    }


    BYTE crc[2];
    RTU_CRC(crc,deviceInfo+2,23);
    memcpy(deviceInfo+25,crc,2);//校验位
    deviceInfo[27]=0x96&0xff;//结束位

    this->ui->tabWidget->setCurrentIndex(5);//切换tab

    if(m_server == NULL)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    if(m_server->writeMessage((char*)deviceInfo,28)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)deviceInfo)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)deviceInfo) + " 报文类型："+getPacketType(deviceInfo)+"\n" + byteToHexStr(deviceInfo,28)+"\n\r");
    }
    this->ui->tabWidget->setCurrentIndex(3);
}

void MainWindow::on_pushButtonSendCMD_clicked()
{

    int cmdLen=this->ui->lineEditCMD->text().size();
    BYTE *cmd=new BYTE[27+cmdLen];
    cmd[0]=0xA5&0xff;//报文头低位
    cmd[1]=0x5A&0xff;//报文头高位
    memcpy(cmd+2,intToByte2(cmdLen),2);//报文长度
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(cmd+4,deviceID,17);//设备ID
    cmd[21]=0x07&0xff;//帧类型
    cmd[22]=0xaf&0xff;//报文类型
    cmd[23]=0x06&0xff;//帧序列号
    memcpy(cmd+24,this->ui->lineEditCMD->text().toLatin1().data(),cmdLen);

    BYTE crc[2];
    RTU_CRC(crc,cmd+2,cmdLen);
    memcpy(cmd+27+cmdLen-3,crc,2);
    cmd[27+cmdLen-1]=0x96&0xff;

    this->ui->tabWidget->setCurrentIndex(5);//切换tab

    if(m_server == NULL)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    if(m_server->writeMessage((char*)cmd,27+cmdLen)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)cmd)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)cmd) + " 报文类型："+getPacketType(cmd)+"\n" + byteToHexStr(cmd,27+cmdLen)+"\n\r");
    }
    this->ui->tabWidget->setCurrentIndex(3);
}

void MainWindow::on_pushButtonQueryDeviceInfo_clicked()
{    

    BYTE *deviceInfo=new BYTE[27];
    deviceInfo[0]=0xA5&0xff;//报文头低位
    deviceInfo[1]=0x5A&0xff;//报文头高位
    deviceInfo[2]=0x00&0xff;//报文长度低字节
    deviceInfo[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(deviceInfo+4,deviceID,17);//设备ID
    deviceInfo[21]=0x07&0xff;//帧类型
    deviceInfo[22]=0xaa&0xff;//报文类型
    deviceInfo[23]=0x06&0xff;//帧序列号

    BYTE crc[2];
    RTU_CRC(crc,deviceInfo+2,22);
    memcpy(deviceInfo+24,crc,2);
    deviceInfo[26]=0x96&0xff;

    this->ui->tabWidget->setCurrentIndex(6);//切换tab

    if(m_server == NULL)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }

    if(m_server->writeMessage((char*)deviceInfo,27)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)deviceInfo)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)deviceInfo) + " 报文类型："+getPacketType(deviceInfo)+"\n" + byteToHexStr(deviceInfo,29)+"\n\r");
    }
}

void MainWindow::on_pushButtonQueryEventParam_clicked()
{
    BYTE *CACInfo=new BYTE[32];
    memset(CACInfo,'\0',32);
    CACInfo[0]=0xA5&0xff;//报文头低位
    CACInfo[1]=0x5A&0xff;//报文头高位
    CACInfo[2]=0x05&0xff;//报文长度低字节
    CACInfo[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(CACInfo+4,deviceID,17);//设备ID
    CACInfo[21]=0x07&0xff;//帧类型
    CACInfo[22]=0xa8&0xff;//报文类型
    CACInfo[23]=0x06&0xff;//帧序列号
    CACInfo[24]=0x00&0xff;//设置
    CACInfo[25]=0x00&0xff;//标识位
    if(this->ui->checkBoxPowerAlarm->isChecked())
    {
        CACInfo[25]=(CACInfo[25] | 0x01);
        CACInfo[26]=this->ui->spinBoxPowerAlarm->text().toInt()&0xff;
    }
    if(this->ui->checkBoxResetAutoSend->isChecked())
    {
        CACInfo[25]=(CACInfo[25] | 0x02);
        CACInfo[27]=this->ui->spinBoxResetAutoSend->text().toInt()&0xff;
    }

    if(this->ui->checkBoxCommunicationFlow->isChecked())
    {
        CACInfo[25]=(CACInfo[25] | 0x04);
        CACInfo[28]=this->ui->spinBoxCommunicationFlow->text().toInt()&0xff;
    }

    BYTE crc[2];
    RTU_CRC(crc,CACInfo+2,27);
    memcpy(CACInfo+29,crc,2);
    CACInfo[31]=0x96&0xff;

    this->ui->tabWidget->setCurrentIndex(5);//切换tab

    if(m_server->writeMessage((char*)CACInfo,32)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)CACInfo)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)CACInfo) + " 报文类型："+getPacketType(CACInfo)+"\n" + byteToHexStr(CACInfo,32)+"\n\r");
    }
    this->ui->tabWidget->setCurrentIndex(5);
}

void MainWindow::on_pushButtonSetEventParam_clicked()
{
    BYTE *CACInfo=new BYTE[32];
    memset(CACInfo,'\0',32);
    CACInfo[0]=0xA5&0xff;//报文头低位
    CACInfo[1]=0x5A&0xff;//报文头高位
    CACInfo[2]=0x05&0xff;//报文长度低字节
    CACInfo[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(CACInfo+4,deviceID,17);//设备ID
    CACInfo[21]=0x07&0xff;//帧类型
    CACInfo[22]=0xa8&0xff;//报文类型
    CACInfo[23]=0x06&0xff;//帧序列号
    CACInfo[24]=0x00&0xff;//设置
    CACInfo[25]=0x00&0xff;//标识位
    if(this->ui->checkBoxPowerAlarm->isChecked())
    {
        CACInfo[25]=(CACInfo[25] | 0x01);
        CACInfo[26]=this->ui->spinBoxPowerAlarm->text().toInt()&0xff;
    }
    if(this->ui->checkBoxResetAutoSend->isChecked())
    {
        CACInfo[25]=(CACInfo[25] | 0x02);
        CACInfo[27]=this->ui->spinBoxResetAutoSend->text().toInt()&0xff;
    }

    if(this->ui->checkBoxCommunicationFlow->isChecked())
    {
        CACInfo[25]=(CACInfo[25] | 0x04);
        CACInfo[28]=this->ui->spinBoxCommunicationFlow->text().toInt()&0xff;
    }

    BYTE crc[2];
    RTU_CRC(crc,CACInfo+2,27);
    memcpy(CACInfo+29,crc,2);
    CACInfo[31]=0x96&0xff;

    this->ui->tabWidget->setCurrentIndex(5);//切换tab

    if(m_server == NULL)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    if(m_server->writeMessage((char*)CACInfo,32)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)CACInfo)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)CACInfo) + " 报文类型："+getPacketType(CACInfo)+"\n" + byteToHexStr(CACInfo,32)+"\n\r");
    }
    this->ui->tabWidget->setCurrentIndex(5);
}

void MainWindow::on_pushButtonQueryData_clicked()
{
    this->ui->tabWidget->setCurrentIndex(6);//切换tab
    BYTE *CACInfo=new BYTE[37];
    memset(CACInfo,'\0',37);
    CACInfo[0]=0xA5&0xff;//报文头低位
    CACInfo[1]=0x5A&0xff;//报文头高位
    CACInfo[2]=0x0a&0xff;//报文长度低字节
    CACInfo[3]=0x00&0xff;//报文长度高字节
    BYTE *device=new BYTE[17];
    memset(device,'\0',17);
    if(strcmp((char*)deviceID,(char*)device)==0)
    {
        QMessageBox::warning(NULL,"提示","设备没上线！");
        return ;
    }
    memcpy(CACInfo+4,deviceID,17);//设备ID
    CACInfo[21]=0x05&0xff;//帧类型
    CACInfo[22]=0xa1&0xff;//报文类型
    CACInfo[23]=0x06&0xff;//帧序列号
    CACInfo[24]=this->ui->spinBoxMeasurePoint->text().toInt()&0xff;//监测点号
    CACInfo[25]=this->ui->comboBoxQueryDataType->currentData().toInt()&0xff;//类型
    QDateTime t1=this->ui->dateTimeEditStartTime->dateTime();
    memcpy(CACInfo+26,formateFromDateTimeToBYTE(t1),4);//开始时间

    QDateTime t2=this->ui->dateTimeEditEndTime->dateTime();
    memcpy(CACInfo+30,formateFromDateTimeToBYTE(t2),4);//结束时间


    BYTE crc[2];
    RTU_CRC(crc,CACInfo+2,32);
    memcpy(CACInfo+34,crc,2);
    CACInfo[36]=0x96&0xff;

    if(m_server->writeMessage((char*)CACInfo,37)!=-1)
    {
        QDateTime time=QDateTime::currentDateTime();
        QString txtTime=time.toString("yyyy-MM-dd hh:mm:ss");
        char deviceID[18]={'\0'};
        memcpy(deviceID,((ProtocolHeader*)CACInfo)->Device_Code,17);
        this->ui->textEditData->insertPlainText(txtTime + " 向设备  " + deviceID + "发送帧类型：" + getFrameType((ProtocolHeader*)CACInfo) + " 报文类型："+getPacketType(CACInfo)+"\n" + byteToHexStr(CACInfo,37)+"\n\r");
    }
    this->ui->tabWidget->setCurrentIndex(6);
}
