#include "frmmain.h"
#include "ui_frmmain.h"
#include <QDateTime>
#include <QTableView>
#include "api/myapi.h"
#include "api/myapp.h"
#include "api/myhelper.h"
#include "gpio.h"
#include "api/spi_drivers.h"
#include "frmconfig.h"
#include "frmdata.h"
#include "frmlogin.h"
#include "frmevent.h"
#include "frmsampler.h"
#include "frmvalve.h"
#include "frmdiagnose.h"
#include "frmcalibration.h"
#include "frmcod.h"
#include<QProcess>
#include <sys/socket.h>
#include <netinet/tcp.h>
#ifdef Q_OS_LINUX
#include <sys/reboot.h>
#endif
extern Com_para COM[6];
extern Tcp_para TCP[4];
extern QextSerialPort *myCom[6];
extern QStandardItemModel  *model_rtd;
Uart1_Execute u1;
Uart2_Execute u2;
Uart3_Execute u3;
Uart4_Execute u4;
Uart5_Execute u5;
Uart6_Execute u6;
Control_Execute rain;
RtdProc          rtdProc;
Count              count;
SendMessage Msend;
DB_Clear db_clear;
SPI_Read_ad  read_ad;
myAPI protocol;
Message message_api;
QTcpSocket *tcpSocket1=NULL;
QTcpSocket *tcpSocket2=NULL;
QTcpSocket *tcpSocket3=NULL;
QTcpSocket *tcpSocket4=NULL;
QTimer *tcpSocketTimer1=NULL;
QTimer *tcpSocketTimer2=NULL;
QTimer *tcpSocketTimer3=NULL;
QTimer *tcpSocketTimer4=NULL;
extern float ad_version;
#define  Valve_TimeOut   150
frmMain::frmMain(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::frmMain)
{
    ui->setupUi(this);

    this->InitStyle();                  //初始化窗口作用
    myHelper::FormInCenter(this,myApp::DeskWidth,myApp::DeskHeigth);//窗体居中显示
//初始化硬件接口以及界面
    InitGPIO();
    this->InitForm();
    this->InitTcpSocketClient();
    this->InitRtdTable();
    this->InitCOM(0);
    this->InitCOM(1);
    this->InitCOM(2);
    this->InitCOM(3);
    this->InitCOM(4);
    this->InitCOM(5);
    u1.start();
    u2.start();
    u3.start();
    u4.start();
    u5.start();
    u6.start();
    if(myApp::COM3ToServerOpen)
    {
        if(u2.isRunning()==false){
        this->InitCOM(1);
        myHelper::Sleep(1);
        u2.start();
        }
    }

    rtdProc.start();
    count.start();
    Msend.start();
    db_clear.start();
    read_ad.start();
    rain.start();
    qDebug()<<QString("com2 id = %1").arg(u1.isRunning());
    qDebug()<<QString("com3 id = %1").arg(u2.isRunning());
    qDebug()<<QString("com4 id = %1").arg(u3.isRunning());
    qDebug()<<QString("com5 id = %1").arg(u4.isRunning());
    qDebug()<<QString("com6 id = %1").arg(u5.isRunning());
    qDebug()<<QString("com7 id = %1").arg(u6.isRunning());


}

frmMain::~frmMain()
{
    rtdProc.quit();
    count.quit();
    Msend.quit();
    db_clear.quit();
    read_ad.quit();
    rain.quit();
    delete ui;
}

void frmMain::InitStyle()
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);   //没有窗体边框
}

void frmMain::InitForm()
{
    labWelcom=new QLabel(QString("%1【%2】 ").arg(myApp::SoftTitle).arg(myApp::SoftVersion));//显示版本信息
    labWelcom->setAttribute(Qt::WA_DeleteOnClose);
    ui->statusbar->addWidget(labWelcom);
    if(myApp::manual_control==false){
        ui->btn_ManualControl->SetCheck(false);
    }
    else{
        ui->btn_ManualControl->SetCheck(true);
    }

    myApp::LocalIP=QNetworkInterface().allAddresses().at(1).toString();
    labIP=new QLabel(QString("本机IP:%1 ").arg(myApp::LocalIP));//显示本机IP信息
    labIP->setAttribute(Qt::WA_DeleteOnClose);
    ui->statusbar->addWidget(labIP);


    labUser=new QLabel(QString("未登录"));
    labUser->setAttribute(Qt::WA_DeleteOnClose);
    ui->statusbar->addWidget(labUser);

    labTime=new QLabel(QDateTime::currentDateTime().toString("yyyy年MM月dd日 HH:mm:ss"));
    labUser->setAttribute(Qt::WA_DeleteOnClose);
    ui->statusbar->addWidget(labTime);
    ui->statusbar->setStyleSheet(QString("QStatusBar::item{border:0px}"));//去掉label的边框.
    connect(&rain,SIGNAL(conrainsignal(int,int,int,int,QString)),this,SLOT(OverFlag(int,int,int,int,QString)));
    connect(&message_api,SIGNAL(consignal(int,int,int,int,QString)),this,SLOT(OverFlag(int,int,int,int,QString)));
    //connect(&message_api,SIGNAL(rain_start()),this,SLOT(get_rain_signal()));
    connect(&message_api,SIGNAL(Sampling()),this,SLOT(Sample_Flag_Changed()));
    connect(&message_api,SIGNAL(Cod_Run()),this,SLOT(Cod_Flag_Changed()));
    timerDate=new QTimer(this);
    timerDate->setInterval(1000);
    connect(timerDate,SIGNAL(timeout()),this,SLOT(ShowDateTime()));
    timerDate->start();

    timerSample=new QTimer(this);
    timerSample->setInterval(5000);
    connect(timerSample,SIGNAL(timeout()),this,SLOT(CheckSampleCmd()));
    timerSample->start();

    /*timerSampleMake=new QTimer(this);
    timerSampleMake->setInterval(28800000);
    myApp::TOC_Flag=1;
    myApp::NH3_Flag=1;
    connect(timerSampleMake,SIGNAL(timeout()),this,SLOT(Cod_Flag_Changed()));
    timerSampleMake->start();*/

    statusTimer=new QTimer(this);
    statusTimer->setInterval(2000);
    connect(statusTimer,SIGNAL(timeout()),this,SLOT(Status()));
    statusTimer->start();

    connect(ui->action_Setting,SIGNAL(triggered()),this,SLOT(ShowForm()));
    connect(ui->action_HistoryData,SIGNAL(triggered()),this,SLOT(ShowForm()));
    connect(ui->action_Event,SIGNAL(triggered()),this,SLOT(ShowForm()));
    connect(ui->action_Sampler,SIGNAL(triggered()),this,SLOT(ShowForm()));
    connect(ui->action_Valve,SIGNAL(triggered()),this,SLOT(ShowForm()));
    connect(ui->action_COD,SIGNAL(triggered()),this,SLOT(ShowForm()));
    connect(ui->action_User,SIGNAL(triggered()),this,SLOT(ShowForm()));
    connect(ui->action_Reset,SIGNAL(triggered()),this,SLOT(ShowForm()));
    connect(ui->action_Import,SIGNAL(triggered()),this,SLOT(ShowForm()));
    connect(ui->action_Export,SIGNAL(triggered()),this,SLOT(ShowForm()));
    connect(ui->action_Diagnose,SIGNAL(triggered()),this,SLOT(ShowForm()));
    connect(ui->action_Calibration,SIGNAL(triggered()),this,SLOT(ShowForm()));
}

/*void frmMain::get_rain_signal()
{
    qDebug()<<"get rain signal";
    if(rain.isRunning())
    {
        rain.terminate();   //关闭
        rain.wait(2000); //2000ms
        while(!rain.isFinished());
        rain.start();   //开启
    }
    else{
        rain.start();
    }
    connect(&protocol,SIGNAL(ProtocolOver(int,int,int,int,QString)),this,SLOT(OverFlag(int,int,int,int,QString)));
    connect(&rain,SIGNAL(conrainsignal(int,int,int,int,QString)),this,SLOT(OverFlag(int,int,int,int,QString)));
}*/

void frmMain::Sample_Flag_Changed()
{
    myApp::Sample_Flag=1;
    qDebug("START SAMPLE");
}
void frmMain::Cod_Flag_Changed()
{
    myApp::COD_Flag=1;
    myApp::TOC_Flag=1;
    myApp::NH3_Flag=1;
    qDebug()<<"START Sample making";
}

void frmMain::OverFlag(int contyp,int drainsta,int catchmentsta,int issample,QString str_tmp)//超标次数上限
{
    frmValve *valvecon = new frmValve;
    myAPI *api=new myAPI;
    bool result;
    switch (contyp) {
    case 1:   //本地控制
        switch (drainsta) {
        case 1://本地开排水阀门
        result=valvecon->Valve_Open_Set();   //排水阀打开
        api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),2,100,result,0,";");
        break;
        case 2://本地关排水阀门
        result=valvecon->Valve_Close_Set();
        api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),2,0,result,0,";");
            break;
        default:
            break;
        }
        switch (catchmentsta) {
        case 1://本地开集水阀门
            result=valvecon->Catchment_Valve_Open_Set();
            api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),1,100,result,0,";");
            break;
        case 2://本地关集水阀门
            result=valvecon->Catchment_Valve_Close_Set();
            api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),1,0,result,0,";");
            break;
        default:
            break;
        }
        if(issample)//本地留样
        {
            myApp::COD_Flag=1;
        }
        break;

    case 2: //自动控制
        if(issample)//自动留样
        {
            myApp::COD_Flag=1;
        }
        switch (drainsta) {
        case 1://自动开排水阀门
            result=valvecon->Valve_Open_Set();   //排水阀打开
            api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),2,100,result,1,";");

            result=valvecon->Catchment_Valve_Close_Set();
            api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),1,0,result,1,";");
            break;
        case 2://自动关排水阀门（超标控制）
            myApp::Sample_Flag=1;
            result=valvecon->Catchment_Valve_Open_Set();

            api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),1,100,result,1,";");

            result=valvecon->Valve_Close_Set();   //排水阀打关
            api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),2,0,result,1,str_tmp);
            break;
        default:
            break;
        }
        break;
    case 3://远程控制
        switch (drainsta) {
        case 1://远程开排水阀门
            result=valvecon->Valve_Open_Set();   //排水阀打开
            api->Insert_Message_Exertn(9012,4,result,str_tmp);
            break;
        case 2://远程关排水阀门
            result=valvecon->Valve_Close_Set();
            api->Insert_Message_Exertn(9012,4,result,str_tmp);
            break;
        default:
            break;
        }
        switch (catchmentsta) {
        case 1://远程开集水阀门
            result=valvecon->Catchment_Valve_Open_Set();
            api->Insert_Message_Exertn(9012,4,result,str_tmp);
            break;
        case 2://远程关集水阀门
            result=valvecon->Catchment_Valve_Close_Set();
            api->Insert_Message_Exertn(9012,4,result,str_tmp);
            break;
        default:
            break;
        }
        if(issample)//远程留样
        {
            myApp::COD_Flag=1;
        }
        break;

    default:
        break;
    }
    delete valvecon;
    delete api;

}

//显示界面
void frmMain::ShowForm()
{
    QAction *action=(QAction *)sender();
    QString triggerName=action->objectName();

    if (triggerName=="action_Setting"){            //系统设置
        if(myApp::Login){
            frmconfig *d=new frmconfig();
            d->showFullScreen();
        }else {
            myHelper::ShowMessageBoxInfo("请登录!");
        }
    }

    if (triggerName=="action_HistoryData"){     //历史数据
            frmdata *d=new frmdata();
            d->showFullScreen();
    }

    if (triggerName=="action_Event"){              //系统日志
            frmevent *d=new frmevent();
            d->showFullScreen();
    }

    if (triggerName=="action_Valve"){              //阀门控制
        if(myApp::Login){
            frmValve *d=new frmValve();
            d->showFullScreen();
        }else {
            myHelper::ShowMessageBoxInfo("请登录!");
        }
    }



    if (triggerName=="action_Sampler"){         //采样器控制
        if(myApp::Login){
            frmSampler *d=new frmSampler();
            d->showFullScreen();
        }else{
            myHelper::ShowMessageBoxInfo("请登录!");
        }
    }

    if (triggerName=="action_COD"){         //COD控制
        if(myApp::Login){
            frmcod *d=new frmcod();
            d->showFullScreen();
        }else{
            myHelper::ShowMessageBoxInfo("请登录!");
        }
    }


    if (triggerName=="action_User"){              //用户登录
        if(!myApp::Login){//未登录
            frmlogin *d=new frmlogin();
            d->exec();
            if(myApp::Login){
                ui->action_User->setText(tr("用户退出"));
                labUser->setText(QString("当前用户:%1【%2】").arg(myApp::CurrentUserName).arg(myApp::CurrentUserType));
                api.AddEventInfoUser("用户登录");
            }
            else{
                labUser->setText(QString("未登录"));
            }
        }
        else {
            myApp::Login=false;
            ui->action_User->setText(tr("用户登录"));
            labUser->setText(QString("未登录"));
            api.AddEventInfoUser("用户退出");
        }

    }

    if (triggerName=="action_Reset"){              //系统重启
        if(myHelper::ShowMessageBoxQuesion("确认系统重启？")==false){
        #ifdef Q_OS_LINUX
        system("reboot -n");
        #elif defined (Q_OS_WIN)
            qApp->quit();
            QProcess::startDetached(qApp->applicationFilePath(), QStringList());
        #endif
        }
    }

    if (triggerName=="action_Export"){              //数据导出
        if(myHelper::ShowMessageBoxQuesion("确认导出吗？")==false){

        }
    }

    if (triggerName=="action_Import"){              //数据导入
        if(myHelper::ShowMessageBoxQuesion("确认导入吗？")==false){

        }
    }

    if (triggerName=="action_Diagnose"){              //在线诊断
        frmdiagnose *d=new frmdiagnose();
        d->showFullScreen();

    }
    
    if (triggerName=="action_Calibration"){           //模拟量校准
        if(myApp::Login){
            if(ad_version > 0){
                frmcalibration *d=new frmcalibration();
                d->showFullScreen();
            }else{
                myHelper::ShowMessageBoxInfo(QObject::trUtf8("请升级AD板程序"));
            }
         }else{
            //frmcalibration *d=new frmcalibration();
            //d->showFullScreen();
            myHelper::ShowMessageBoxInfo("请登录!");
         }
    
    }

}

//设置实时数据显示表格模型
void frmMain::InitRtdTable()
{
    model_rtd = new QStandardItemModel();
    model_rtd->setColumnCount(4);
    model_rtd->setHeaderData(0,Qt::Horizontal,QString::fromLocal8Bit("监测因子"));
    model_rtd->setHeaderData(1,Qt::Horizontal,QString::fromLocal8Bit("实时值"));
    model_rtd->setHeaderData(2,Qt::Horizontal,QString::fromLocal8Bit("累计值"));
    model_rtd->setHeaderData(3,Qt::Horizontal,QString::fromLocal8Bit("实时标记"));

    columnWidths[0]=200;
    columnWidths[1]=200;
    columnWidths[2]=200;
    columnWidths[3]=180;
    ui->tableView->setModel(model_rtd);
    //表头信息显示居中
    ui->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignHCenter);

    //设置列宽根据内容变化
    for(int i=0;i<model_rtd->columnCount();i++){
        ui->tableView->horizontalHeader()->setResizeMode(i,QHeaderView::Interactive);
        ui->tableView->setColumnWidth(i,columnWidths[i]);
    }
    //点击表头时不对表头光亮
    ui->tableView->horizontalHeader()->setHighlightSections(false);
    //选中模式为单行选中
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    //选中整行
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setStyleSheet( "QTableView::item:hover{background-color:rgb(0,200,255,255)}"
                                                             "QTableView::item:selected{background-color:#0000AA}");
    //设置非编辑状态
    ui->tableView->setEditTriggers(QTableView::NoEditTriggers);
    //设置表头背景色
    ui->tableView->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue}");
    ui->tableView->verticalHeader()->setStyleSheet("QHeaderView::section{background:skyblue}");

}

//显示当前运行时间
void frmMain::ShowDateTime()
{
    labTime->setText(QDateTime::currentDateTime().toString("yyyy年MM月dd日 HH:mm:ss"));
}

bool frmMain::CheckSampleCmd()
{
///////////定时检测物理网络是否断开///////////
    QFile etho_status("/sys/class/net/eth0/carrier");
    if(!etho_status.open(QIODevice::ReadOnly)){
        return false;
    }
    QString strList;
    QTextStream stream(&etho_status);
    strList=stream.readAll();
    //qDebug()<<strList;
    etho_status.close();
    if(strList.startsWith("0"))
    {
        if(TCP[0].isConnected==true)
        {
            TCP[0].isConnected=false;
            qDebug()<<QString("%1:%2 disconnected.").arg(TCP[0].ServerIP).arg(TCP[0].ServerPort);
        }

        if(TCP[1].isConnected==true)
        {
            TCP[1].isConnected=false;
            qDebug()<<QString("%1:%2 disconnected.").arg(TCP[1].ServerIP).arg(TCP[1].ServerPort);
        }
        if(TCP[2].isConnected==true)
        {
            TCP[2].isConnected=false;
            qDebug()<<QString("%1:%2 disconnected.").arg(TCP[2].ServerIP).arg(TCP[2].ServerPort);
        }
        if(TCP[3].isConnected==true)
        {
            TCP[3].isConnected=false;
            qDebug()<<QString("%1:%2 disconnected.").arg(TCP[3].ServerIP).arg(TCP[3].ServerPort);
        }

    }
    return true;
}



void frmMain::InitTcpSocketClient()
{


    if(TCP[0].ServerOpen){

        tcpSocket1 = new QTcpSocket(this);
        connect(tcpSocket1, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
        connect(tcpSocket1, SIGNAL(connected()), this, SLOT(updateclient()));
        connect(tcpSocket1, SIGNAL(readyRead()), this, SLOT(clientReadMessage()));

        tcpSocketTimer1=new QTimer(this);
        tcpSocketTimer1->setInterval(10000);
        connect(tcpSocketTimer1,SIGNAL(timeout()),this,SLOT(ConnectServer1()));
        tcpSocketTimer1->start();

    }

    if(TCP[1].ServerOpen){

        tcpSocket2 = new QTcpSocket(this);
        connect(tcpSocket2, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
        connect(tcpSocket2, SIGNAL(connected()), this, SLOT(updateclient()));
        connect(tcpSocket2, SIGNAL(readyRead()), this, SLOT(clientReadMessage()));

        tcpSocketTimer2=new QTimer(this);
        tcpSocketTimer2->setInterval(10000);
        connect(tcpSocketTimer2,SIGNAL(timeout()),this,SLOT(ConnectServer2()));
        tcpSocketTimer2->start();
    }

    if(TCP[2].ServerOpen){

        tcpSocket3 = new QTcpSocket(this);
        connect(tcpSocket3, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
        connect(tcpSocket3, SIGNAL(connected()), this, SLOT(updateclient()));
        connect(tcpSocket3, SIGNAL(readyRead()), this, SLOT(clientReadMessage()));
        tcpSocketTimer3=new QTimer(this);
        tcpSocketTimer3->setInterval(10000);
        connect(tcpSocketTimer3,SIGNAL(timeout()),this,SLOT(ConnectServer3()));
        tcpSocketTimer3->start();
    }

    if(TCP[3].ServerOpen){

        tcpSocket4 = new QTcpSocket(this);
        connect(tcpSocket4, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
        connect(tcpSocket4, SIGNAL(connected()), this, SLOT(updateclient()));
        connect(tcpSocket4, SIGNAL(readyRead()), this, SLOT(clientReadMessage()));
        tcpSocketTimer4=new QTimer(this);
        tcpSocketTimer4->setInterval(10000);
        connect(tcpSocketTimer4,SIGNAL(timeout()),this,SLOT(ConnectServer4()));
        tcpSocketTimer4->start();

    }
}

//客户端无连接
void frmMain::displayError(QAbstractSocket::SocketError)
{
    if(tcpSocket1!=NULL){
        if(tcpSocket1==dynamic_cast<QTcpSocket *>(sender()))
        {
            TCP[0].isConnected=false;
            qDebug()<<QString("%1:%2 disconnected.").arg(TCP[0].ServerIP).arg(TCP[0].ServerPort);
        }
    }
    if(tcpSocket2!=NULL){
        if(tcpSocket2==dynamic_cast<QTcpSocket *>(sender()))
        {
            TCP[1].isConnected=false;
            qDebug()<<QString("%1:%2 disconnected.").arg(TCP[1].ServerIP).arg(TCP[1].ServerPort);

        }
    }
    if(tcpSocket3!=NULL){
        if(tcpSocket3==dynamic_cast<QTcpSocket *>(sender()))
        {
            TCP[2].isConnected=false;
            qDebug()<<QString("%1:%2 disconnected.").arg(TCP[2].ServerIP).arg(TCP[2].ServerPort);

        }
    }
    if(tcpSocket4!=NULL){
        if(tcpSocket4==dynamic_cast<QTcpSocket *>(sender()))
        {
            TCP[3].isConnected=false;
            qDebug()<<QString("%1:%2 disconnected.").arg(TCP[3].ServerIP).arg(TCP[3].ServerPort);
        }
    }

}

void frmMain::clientReadMessage()
{
    QByteArray str;
    if(tcpSocket1!=NULL){
    if(tcpSocket1==dynamic_cast<QTcpSocket *>(sender()))
    {
        str=tcpSocket1->readAll();
        message_api.messageProc(str,NULL,tcpSocket1);
        qDebug()<<QString("received from socket1:%1").arg(str.data());
    }
    }
    if(tcpSocket2!=NULL){
    if(tcpSocket2==dynamic_cast<QTcpSocket *>(sender()))
    {
        str=tcpSocket2->readAll();
        message_api.messageProc(str,NULL,tcpSocket2);
        qDebug()<<QString("received from socket2:%1").arg(str.data());
    }
    }
    if(tcpSocket3!=NULL){
    if(tcpSocket3==dynamic_cast<QTcpSocket *>(sender()))
    {
        str=tcpSocket3->readAll();
        message_api.messageProc(str,NULL,tcpSocket3);
        qDebug()<<QString("received from socket3:%1").arg(str.data());
    }
    }
    if(tcpSocket4!=NULL){
    if(tcpSocket4==dynamic_cast<QTcpSocket *>(sender()))
    {
        str=tcpSocket4->readAll();
        message_api.messageProc(str,NULL,tcpSocket4);
        qDebug()<<QString("received from socket4:%1").arg(str.data());
    }
    }

}

//已连接上服务器
void frmMain::updateclient()
{
    int keepAlive = 1; // 开启keepalive属性
    int keepIdle = 30; // 如该连接在30秒内没有任何数据往来,则进行探测
    int keepInterval = 10; // 探测时发包的时间间隔为5秒
    int keepCount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
    if(tcpSocket1!=NULL){
    if(tcpSocket1==dynamic_cast<QTcpSocket *>(sender()))
    {
        TCP[0].isConnected=true;
        qDebug()<<QString("%1:%2 connected.").arg(TCP[0].ServerIP).arg(TCP[0].ServerPort);
        int fd_1;
        fd_1=tcpSocket1->socketDescriptor();
        if(setsockopt(fd_1, SOL_SOCKET,SO_KEEPALIVE,&keepAlive,sizeof(keepAlive))!=0)
        {
            qDebug()<<QString("keepalive error");
        }

        if(setsockopt(fd_1, SOL_TCP , TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle))!=0)
        {
            qDebug()<<QString("TCP_KEEPIDLE error");
        }

        if(setsockopt(fd_1, SOL_TCP , TCP_KEEPINTVL, &keepInterval, sizeof(keepInterval))!=0)
        {
            qDebug()<<QString("TCP_KEEPINTVL error");
        }

        if(setsockopt(fd_1, SOL_TCP , TCP_KEEPCNT, &keepCount, sizeof(keepCount))!=0)
        {
            qDebug()<<QString("TCP_KEEPCNT error");
        }

    }
    }
    if(tcpSocket2!=NULL){
    if(tcpSocket2==dynamic_cast<QTcpSocket *>(sender()))
    {
        TCP[1].isConnected=true;
        qDebug()<<QString("%1:%2 connected.").arg(TCP[1].ServerIP).arg(TCP[1].ServerPort);
        int fd_2;
        fd_2=tcpSocket2->socketDescriptor();
        if(setsockopt(fd_2, SOL_SOCKET,SO_KEEPALIVE,&keepAlive,sizeof(keepAlive))!=0)
        {
            qDebug()<<QString("keepalive error");

        }

        if(setsockopt(fd_2, SOL_TCP , TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle))!=0)
        {
            qDebug()<<QString("TCP_KEEPIDLE error");
        }

        if(setsockopt(fd_2, SOL_TCP , TCP_KEEPINTVL, &keepInterval, sizeof(keepInterval))!=0)
        {
            qDebug()<<QString("TCP_KEEPINTVL error");
        }

        if(setsockopt(fd_2, SOL_TCP , TCP_KEEPCNT, &keepCount, sizeof(keepCount))!=0)
        {
            qDebug()<<QString("TCP_KEEPCNT error");
        }

    }
    }
    if(tcpSocket3!=NULL){
    if(tcpSocket3==dynamic_cast<QTcpSocket *>(sender()))
    {
        TCP[2].isConnected=true;
        qDebug()<<QString("%1:%2 connected.").arg(TCP[2].ServerIP).arg(TCP[2].ServerPort);
        int fd_3;
        fd_3=tcpSocket3->socketDescriptor();
        if(setsockopt(fd_3, SOL_SOCKET,SO_KEEPALIVE,&keepAlive,sizeof(keepAlive))!=0)
        {
            qDebug()<<QString("keepalive error");

        }

        if(setsockopt(fd_3, SOL_TCP , TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle))!=0)
        {
            qDebug()<<QString("TCP_KEEPIDLE error");
        }

        if(setsockopt(fd_3, SOL_TCP , TCP_KEEPINTVL, &keepInterval, sizeof(keepInterval))!=0)
        {
            qDebug()<<QString("TCP_KEEPINTVL error");
        }

        if(setsockopt(fd_3, SOL_TCP , TCP_KEEPCNT, &keepCount, sizeof(keepCount))!=0)
        {
            qDebug()<<QString("TCP_KEEPCNT error");
        }

    }
    }
    if(tcpSocket4!=NULL){
    if(tcpSocket4==dynamic_cast<QTcpSocket *>(sender()))
    {
        TCP[3].isConnected=true;
        qDebug()<<QString("%1:%2 connected.").arg(TCP[3].ServerIP).arg(TCP[3].ServerPort);
        int fd_4;
        fd_4=tcpSocket4->socketDescriptor();
        if(setsockopt(fd_4, SOL_SOCKET,SO_KEEPALIVE,&keepAlive,sizeof(keepAlive))!=0)
        {
            qDebug()<<QString("keepalive error");

        }

        if(setsockopt(fd_4, SOL_TCP , TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle))!=0)
        {
            qDebug()<<QString("TCP_KEEPIDLE error");
        }

        if(setsockopt(fd_4, SOL_TCP , TCP_KEEPINTVL, &keepInterval, sizeof(keepInterval))!=0)
        {
            qDebug()<<QString("TCP_KEEPINTVL error");
        }

        if(setsockopt(fd_4, SOL_TCP , TCP_KEEPCNT, &keepCount, sizeof(keepCount))!=0)
        {
            qDebug()<<QString("TCP_KEEPCNT error");
        }
    }
    }
}


void frmMain::ConnectServer1()
{
    if(tcpSocketTimer1!=NULL){
        if(tcpSocketTimer1==dynamic_cast<QTimer *>(sender()))
        {

            if(TCP[0].isConnected==false&&(tcpSocket1->state()!=tcpSocket1->ConnectingState))
            {
                tcpSocket1->close();
                qDebug()<<QString("start connect to %1:%2").arg(TCP[0].ServerIP).arg(TCP[0].ServerPort);
                tcpSocket1->connectToHost(TCP[0].ServerIP,TCP[0].ServerPort.toInt());
            }
        }
    }
}

void frmMain::ConnectServer2()
{
    if(tcpSocketTimer2!=NULL){
    if(tcpSocketTimer2==dynamic_cast<QTimer *>(sender()))
    {
        if(TCP[1].isConnected==false&&(tcpSocket2->state()!=tcpSocket2->ConnectingState))
        {
            tcpSocket2->close();
            qDebug()<<QString("start connect to %1:%2").arg(TCP[1].ServerIP).arg(TCP[1].ServerPort);
            tcpSocket2->connectToHost(TCP[1].ServerIP,TCP[1].ServerPort.toInt());
        }
    }
    }

}

void frmMain::ConnectServer3()
{
    if(tcpSocketTimer3!=NULL){
    if(tcpSocketTimer3==dynamic_cast<QTimer *>(sender()))
    {
        if(TCP[2].isConnected==false&&(tcpSocket3->state()!=tcpSocket3->ConnectingState))
        {
            tcpSocket3->close();
            qDebug()<<QString("start connect to %1:%2").arg(TCP[2].ServerIP).arg(TCP[2].ServerPort);
            tcpSocket3->connectToHost(TCP[2].ServerIP,TCP[2].ServerPort.toInt());
        }
    }
    }
}
void frmMain::ConnectServer4()
{
    if(tcpSocketTimer4!=NULL){
    if(tcpSocketTimer4==dynamic_cast<QTimer *>(sender()))
    {
        if(TCP[3].isConnected==false&&(tcpSocket4->state()!=tcpSocket4->ConnectingState))
        {
            tcpSocket4->close();
            qDebug()<<QString("start connect to %1:%2").arg(TCP[3].ServerIP).arg(TCP[3].ServerPort);
            tcpSocket4->connectToHost(TCP[3].ServerIP,TCP[3].ServerPort.toInt());
        }
    }
    }
}

//串口初始化
void frmMain::InitCOM(int port)
{
    #ifdef Q_OS_LINUX
        myCom[port] = new QextSerialPort("/dev/" + COM[port].Name);
    #elif defined (Q_OS_WIN)
        myCom[port] = new QextSerialPort(portName);
    #endif
    //设置波特率
    myCom[port]->setBaudRate((BaudRateType)COM[port].Baudrate);

    //设置数据位
    myCom[port]->setDataBits((DataBitsType)COM[port].Databits);

    //设置校验
    switch(COM[port].Parity){
    case 0:
        myCom[port]->setParity(PAR_NONE);
        break;
    case 1:
        myCom[port]->setParity(PAR_ODD);
        break;
    case 2:
        myCom[port]->setParity(PAR_EVEN);
        break;
    default:
        myCom[port]->setParity(PAR_NONE);
        break;
    }

    //设置停止位
    switch(COM[port].Stopbits){
    case 0:
        myCom[port]->setStopBits(STOP_1);
        break;
    case 1:
     #ifdef Q_OS_WIN
        myCom[port]->setStopBits(STOP_1_5);
     #endif
        break;
    case 2:
        myCom[port]->setStopBits(STOP_2);
        break;
    default:
        myCom[port]->setStopBits(STOP_1);
        break;
    }

    //设置数据流控制
    myCom[port]->setFlowControl(FLOW_OFF);

    //设置延时
    myCom[port]->setTimeout(TIME_OUT);
    if(!myCom[port]->isOpen())//关闭状态下打开
    {
        if(myCom[port]->open(QIODevice::ReadWrite)){
            qDebug()<<QString("%1 open success").arg(COM[port].Name);

        }else{
            qDebug()<<QString("%1 open failed").arg(COM[port].Name);
        }

    }

}

enum PowerStatus{IsOff,IsOn}Power_CurrentStatus;

extern enum Status{IsOpen,IsClose,IsError}Valve_CurrentStatus,Valve_RightStatus,Catchment_CurrentStatus,Catchment_RightStatus,reflux_CurrentStatus;
extern bool Set_Flag;
extern bool Catchment_Set_Flag;
//状态
void frmMain::Status()
{
    //网络状态
    if(TCP[0].ServerOpen){
        if(TCP[0].isConnected){
            ui->netStatus1->setStyleSheet("image: url(:/image/led_blue.png);");
        }
        else{
            ui->netStatus1->setStyleSheet("image: url(:/image/led_red.png);");
        }
    }else{
            ui->netStatus1->setStyleSheet("image: url(:/image/led_gray.png);");
    }

    if(TCP[1].ServerOpen){
        if(TCP[1].isConnected){
            ui->netStatus2->setStyleSheet("image: url(:/image/led_blue.png);");
        }
        else{
            ui->netStatus2->setStyleSheet("image: url(:/image/led_red.png);");
        }
    }else{
            ui->netStatus2->setStyleSheet("image: url(:/image/led_gray.png);");
    }

    if(TCP[2].ServerOpen){
        if(TCP[2].isConnected){
            ui->netStatus3->setStyleSheet("image: url(:/image/led_blue.png);");
        }
        else{
            ui->netStatus3->setStyleSheet("image: url(:/image/led_red.png);");
        }
    }else{
            ui->netStatus3->setStyleSheet("image: url(:/image/led_gray.png);");
    }

    if(TCP[3].ServerOpen){
        if(TCP[3].isConnected){
            ui->netStatus4->setStyleSheet("image: url(:/image/led_blue.png);");
        }
        else{
            ui->netStatus4->setStyleSheet("image: url(:/image/led_red.png);");
        }
    }else{
            ui->netStatus4->setStyleSheet("image: url(:/image/led_gray.png);");
    }
    //市电状态
    if(GetSwitchStatus(myApp::In_power)==true){
        Power_CurrentStatus=IsOn;//开
        ui->powerStatus->setStyleSheet("image: url(:/image/led_blue.png);");
        Power_Old=Power_New;
        Power_New=1;
    }
    else {
        Power_CurrentStatus=IsOff;//关       
        ui->powerStatus->setStyleSheet("image: url(:/image/led_red.png);");
        Power_Old=Power_New;
        Power_New=0;

    }
    if(Power_New!=Power_Old)
    {
        Power_Change=1;

    }
    qDebug()<<QString("rain=%1").arg(myApp::Pro_Rain);
    //排水阀门状态
    if(GetSwitchStatus(myApp::In_drain_open)==true&&GetSwitchStatus(myApp::In_drain_close)==false){
        Valve_CurrentStatus=IsOpen;//开
        ui->valveStatus->setStyleSheet("image: url(:/image/led_blue.png);");
    }
    else if(GetSwitchStatus(myApp::In_drain_open)==false&&GetSwitchStatus(myApp::In_drain_close)==true){
        Valve_CurrentStatus=IsClose;//关
        ui->valveStatus->setStyleSheet("image: url(:/image/led_red.png);");
    }
    else{
        Valve_CurrentStatus=IsError;//状态错误
         ui->valveStatus->setStyleSheet("image: url(:/image/led_gray.png);");
    }

    //集水阀门状态
    if(GetSwitchStatus(myApp::In_catchment_open)==true&&GetSwitchStatus(myApp::In_catchment_close)==false){
        Catchment_CurrentStatus=IsOpen;//开
        ui->CatchmentStatus->setStyleSheet("image: url(:/image/led_blue.png);");
    }
    else if(GetSwitchStatus(myApp::In_catchment_open)==false&&GetSwitchStatus(myApp::In_catchment_close)==true){
        Catchment_CurrentStatus=IsClose;//关
        ui->CatchmentStatus->setStyleSheet("image: url(:/image/led_red.png);");
    }
    else{
        Catchment_CurrentStatus=IsError;//状态错误
         ui->CatchmentStatus->setStyleSheet("image: url(:/image/led_gray.png);");
    }

    if(GetSwitchStatus(myApp::In_reflux_open)==true&&GetSwitchStatus(myApp::In_reflux_close)==false){
        reflux_CurrentStatus=IsOpen;//开
    }
    else if(GetSwitchStatus(myApp::In_reflux_open)==false&&GetSwitchStatus(myApp::In_reflux_close)==true){
        reflux_CurrentStatus=IsClose;//关
    }
    else{
        reflux_CurrentStatus=IsError;//状态错误
    }

    QString str_tmp;
    if(Set_Flag==false){
        if(Valve_CurrentStatus!=Valve_RightStatus){
            //排水阀门异常
            if(Valve_RightStatus==IsOpen){
                str_tmp="OriginalPer2=100;";
            }else if(Valve_RightStatus==IsClose){
                str_tmp="OriginalPer2=0;";
            }else{
                str_tmp="OriginalPer2=--;";
            }
            myAPI *api=new myAPI;
            api->Insert_Message_VSErr(3053,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),str_tmp);
            delete api;
            Set_Flag=true;
        }
    }
    if(Catchment_Set_Flag==false){
        if(Catchment_CurrentStatus!=Catchment_RightStatus){
            //集水阀门异常
            if(Catchment_RightStatus==IsOpen){
                str_tmp="OriginalPer1=100;";
            }else if(Catchment_RightStatus==IsClose){
                str_tmp="OriginalPer2=0;";
            }else{
                str_tmp="OriginalPer2=--;";
            }
            myAPI *api=new myAPI;
            api->Insert_Message_VSErr(3053,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),str_tmp);
            delete api;
            Catchment_Set_Flag=true;
        }
    }


}

void frmMain::on_btnClearData_clicked()
{
    QSqlQuery query(QSqlDatabase::database("memory",true));
    query.exec("delete from [CacheRtd]");
    model_rtd->removeRows(0,model_rtd->rowCount());
}


void frmMain::updateClientStatusDisconnect()
{
    if(tcpSocket1==dynamic_cast<QTcpSocket *>(sender()))
    {
        TCP[0].isConnected=false;
    }
    if(tcpSocket2==dynamic_cast<QTcpSocket *>(sender()))
    {
        TCP[1].isConnected=false;
    }
    if(tcpSocket3==dynamic_cast<QTcpSocket *>(sender()))
    {
        TCP[2].isConnected=false;
    }
    if(tcpSocket4==dynamic_cast<QTcpSocket *>(sender()))
    {
        TCP[3].isConnected=false;
    }

}


void frmMain::on_btn_ManualControl_clicked()
{
    QString str_tmp;
    if(ui->btn_ManualControl->GetCheck()==true)
    {
        myApp::manual_control=true;
        myApp *rain_pro=new myApp;
        rain_pro->PronumberChange(7);   //等待降雨
        delete rain_pro;


    }
    else {
        myApp::manual_control=false;
        myApp *rain_pro=new myApp;
        rain_pro->PronumberChange(0);   //等待降雨
        delete rain_pro;

    }
    if(myApp::manual_control){
        str_tmp="MaintainType=1";
    }
    else{
        str_tmp="MaintainType=0";
    }
    myAPI *api=new myAPI;
    api->Insert_Message_VSErr(4011,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),str_tmp);
    delete api;

}

