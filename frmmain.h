#ifndef FRMMAIN_H
#define FRMMAIN_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QTcpSocket>
#include "api/myapp.h"
#include "api/mythread.h"
#include "api/message.h"
#include "frmmain.h"

//延时，TIME_OUT是串口读写的延时
#define TIME_OUT 100

namespace Ui {
class frmMain;
}

class frmMain : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit frmMain(QWidget *parent = 0);
    ~frmMain();
    
private:
    Ui::frmMain *ui;
    QLabel *labTime;    //当前时间标签
    QLabel *labWelcom;    //当前时间标签
    QLabel *labUser;    //当前时间标签
    QLabel *labIP;    //当前ip
    QTimer *timerDate;  //计算在线时长和显示当前时间定时器对象
    QTimer *timerSample; //定时读取WEB对采样器的指令
    QTimer *tcpSocketTimer1;
    QTimer *tcpSocketTimer2;
    QTimer *tcpSocketTimer3;
    QTimer *tcpSocketTimer4;
    QTimer *statusTimer;

    frmValve v;
    myAPI api;

    int  columnWidths[6];        //列宽数组

    void InitStyle();
    void InitForm();
    void InitTcpSocketClient();
    void InitCOM(int port);
    void InitRtdTable();

private slots:
    void ShowDateTime();
    void displayError(QAbstractSocket::SocketError);
    void clientReadMessage();
    void updateclient();
    void ConnectServer1();
    void ConnectServer2();
    void ConnectServer3();
    void ConnectServer4();
    bool CheckSampleCmd();

    void ShowForm();
    void Status();
    void on_btnClearData_clicked();
    void updateClientStatusDisconnect();
    void get_rain_signal();
    void OverFlag(int contyp,int drainsta,int catchmentsta,int issample,QString str_tmp);
    void Sample_Flag_Changed();
    void Cod_Flag_Changed();

    void on_btn_ManualControl_clicked();

signals: //信令操作
    void   ResultValveControl(int ValveNum,int Per,int isSucceed,int ConType);


};

#endif // FRMMAIN_H
