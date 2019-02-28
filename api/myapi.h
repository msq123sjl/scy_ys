#ifndef MYAPI_H
#define MYAPI_H

#include <QObject>
#include "api/myapp.h"
#include "api/message.h"
#include <QList>

//#define _DEBUG
#define _TEST


class myAPI : public QObject
{
    Q_OBJECT
public:
    explicit myAPI(QObject *parent = 0);

    int   CountRecordIsExist(QString tableName,QString Time);
    int   CacheRecordIsExist(QString Code);
    bool TableIsExist(QString tableName);
    void RtdTableCreate(QString tableName);
    void CountDataTableCreate(QString tableName);
    double GetCountDataFromSql(QString tableName,QString StartTime,QString EndTime,QString field,QString func);
    double GetCountDataFromSql1(QString tableName,QString StartTime,QString EndTime,QString field,QString func);
    void CacheDataProc(double rtd,double total,QString flag,int dec,QString name,QString code,QString unit);
    void RtdProc();
    void CacheCard(QString cardtype,QString cardno);

     void AddEventInfo(QString TriggerType, QString TriggerContent);
     void AddEventInfoUser(QString TriggerContent); 
     void MessageFromCom(int port);
     void ShowRtd();
     double AnalogConvert(double adValue,double RangeUp,double RangeLow,QString Signal);


     void MinsDataProc_WaterFlow(QString startTime,QString endTime);
     void MinsDataProc_WaterPara(QString startTime,QString endTime);
     void HourDataProc_WaterFlow(QString startTime,QString endTime);
     void HourDataProc_WaterPara(QString startTime,QString endTime);
     void DayDataProc(QString startTime,QString endTime);

     void Insert_Message_Count(int CN,int flag,QString dt);
     void Insert_Message_Control(int CN,int flag,QString dt,int valvenum,int per,bool isok,int contyp,QString str_tmp);
     void Insert_Message_VSErr(int CN,int flag,QString dt,QString str_tmp);
     void Insert_Message_Exertn(int CN,int flag,bool isok,QString qn);
     void Insert_Message_Rtd(int flag,QString dt);
     void Update_Respond(QString QN,QString From);
     void Insert_Message_Received(QString QN,int CN,QString From,QString Content);
     void SendData_Master(int CN,int flag);
     void Protocol_1();
     bool Sample_execute();
     void Insert_Message_Card_Add(QString QN,QString CardNo,QString CardType);
     void Update_Message_Card_delete(QString CardNo,QString CardType);
     void SendData_Status(int flag);
     void InsertList(QString str);

private:  
    double HexToDouble(const unsigned char* bytes);
    void Protocol_2(int port,int Address,int Dec,QString Name,QString Code,QString Unit);
    void Protocol_3(int port,int Address,int Dec,QString Name,QString Code,QString Unit);
    int  Protocol_4_read(int port,int Address,double *rtd);
    void Protocol_4_control(int port,int Address);
    void Protocol_4(int port,int Address,int Dec,QString Name,QString Code,QString Unit,double alarm_max);
    void Protocol_5(int port);
    void Protocol_6(int port);
    void Protocol_7(int port,int Address,int Dec,QString Name,QString Code,QString Unit,double alarm_min,double alarm_max);
    void Protocol_8(int port,int Dec,QString Name,QString Code,QString Unit);
    void Protocol_9(int port,int Address,int Dec,QString Name,QString Code,QString Unit);
    void Protocol_10(int port,int Address,int Dec,QString Name,QString Code,QString Unit,int COD_or_NH3);
    void Protocol_11(int port,int Address,int Dec,QString Name,QString Code,QString Unit);
    void Protocol_12(int port,int Address,int Dec,QString Name,QString Code,QString Unit,int COD_or_NH3);
    void Protocol_13(int port,int Address,int Dec,QString Name,QString Code,QString Unit);
    void Protocol_14_Rtu(int port,int Address,int Dec,QString Name,QString Code,QString Unit,int path);
    void Protocol_21(int port,int Dec,QString Name,QString Code,QString Unit,int COD_or_NH3,int equipment);

    bool Get_Sampler_Status();
    bool Get_Bottle_Status();
    bool Sample_current_all();
    bool Sample_reset();
    bool sample_record();



signals:
    void ProtocolOver(int contyp,int drainsta,int catchmentsta,int issample,QString str_tmp);

};

#endif // MYAPI_H
