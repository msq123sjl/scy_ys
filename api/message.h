#ifndef MESSAGE_H
#define MESSAGE_H

#include "qextserial/qextserialport.h"
#include <QDateTime>
#include <QtNetwork>
#include <QObject>
#define RESULT_SUCCESS 1
#define RESULT_FAILED  2
#define RESULT_NODATA  100
#define RESULT_DO        4

#define REQUEST_READY    1
#define REQUEST_REFUSED  2
#define REQUEST_CODE_ERR 3

#define CN_SendTime         1011
#define CN_SendAlarmTarget  1031
#define CN_SendReportTime   1041
#define CN_SendRtdInterval  1061
#define CN_SendStatus       2021
#define CN_SendDayData      2031
#define CN_SendHourData     2061
#define CN_SendMinsData     2051
#define CN_SendAlarmData    2071
#define CN_SendAlarmEvent   2072
#define CN_SendRunTimeData  2041

#define CN_Set_OverTime_ReCount 1000
#define CN_SetAlarmTime     1001
#define CN_GetTime          1011
#define CN_SetTime          9016
#define CN_GetAlarmValue    1021
#define CN_SetAlarmValue    1022
#define CN_GetAlarmTarget   1031
#define CN_SetAlarmTarget   1032
#define CN_GetReportTime    1041
#define CN_SetReportTime    1042
#define CN_GetRtdInterval   1061
#define CN_SetRtdInterval   1062
#define CN_SetPW            1072
#define CN_GetRtdData       2011
#define CN_StopRtdData      2012
#define CN_GetStatus        2021
#define CN_StopStatus       2022
#define CN_GetDayData       2031
#define CN_GetRunTimeData   2041
#define CN_GetMinsData      2051
#define CN_GetHourData      2061
#define CN_GetAlarmData
#define CN_DataRespond      9014
//平台下发命令
#define CN_Valve_control      3051     //阀门控制  完成
#define CN_Factor_control    3012     //反控采样或者其他在线设备
#define CN_Limit_set             3094    //设置阈值
#define CN_Rain_begin          6011      //开始降雨  完成
#define CN_Rain_stop            6012      //停止降雨    完成

#define CN_Card_add            5012     //开卡
#define CN_Card_delete        5013      //销卡
#define CN_Door_control      3097     //远程开门

//向平台上传的指令
#define CN_Valve_record       3052     //上传阀门控制记录
#define CN_SendRtdData      2011     //实时数据（降雨过程发送）
#define CN_Status                  3071     //上传状态信息（全过程-阀门、泵、液位、当前瓶号）
#define CN_ValveOpen          3053     //上传开阀请求
#define CN_DoorRecord        3097     //上传开门记录

const QString ST_to_Str[]={
    "20","31","32","33","34","35","36","37","38","21","22","23","41"
};

class Message: public QObject
{
    Q_OBJECT
public:
    void  SendCurrentTime(QextSerialPort *com,QTcpSocket *tcp);
    void  SendAlarmTarget(QextSerialPort *com,QTcpSocket *tcp);
    void  SendReportTime(QextSerialPort *com,QTcpSocket *tcp);
    void  SendRtdInterval(QextSerialPort *com,QTcpSocket *tcp);
    void  SendStatus(QextSerialPort *com,QTcpSocket *tcp);
    void  SendRtdData_Slave(QextSerialPort *com,QTcpSocket *tcp);
    void  SendCountData_Slave(int CN_Type,QextSerialPort *com,QTcpSocket *tcplink);
    void  SendRunTimeData(QextSerialPort *com,QTcpSocket *tcp);
    void  SendAlarmValue(QString s,QextSerialPort *com,QTcpSocket *tcp);
    void  SetAlarmValue(QString s);
    void  CacheCountDataForSend(QString datatime);
    int    messageProc(QString str,QextSerialPort *com,QTcpSocket *tcp);
    void  RequestRespond(int QnRtn,QextSerialPort *com,QTcpSocket *tcp);
    void  NoticeRespond(QextSerialPort *com,QTcpSocket *tcp);
    void  DataRespond(QextSerialPort *com,QTcpSocket *tcp);
    int    ScanString(char *object,char *scan);
    void  ExecuteRespond(int ExeRtn,QextSerialPort *com,QTcpSocket *tcp);
    bool  messageSetTime(int year,int month,int day,int hour,int min,int sec);
    int    messageThresholdValue(QString s);
signals:
    //void rain_start();
    void Cod_Run();
    void Sampling();
    void consignal(int contyp,int drainsta,int catchmentsta,int issample,QString str_tmp);


private:
    QDateTime BeginTime;
    QDateTime EndTime;
    QString QN;
    int CN;

};


#endif // MESSAGE_H
