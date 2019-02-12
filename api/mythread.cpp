#include "api/mythread.h"
#include "api/spi_drivers.h"
#include <QSqlQuery>
#include "api/gpio.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include "gpio.h"
#include "api/myapp.h"
#include <QList>
#include <QSqlError>
#include "frmcalibration.h"
#include <algorithm>

extern myAPI protocol;
extern enum Status{IsOpen,IsClose,IsError}Valve_CurrentStatus,Valve_RightStatus,Catchment_CurrentStatus,Catchment_RightStatus,reflux_CurrentStatus;
void Uart1_Execute::run()
{   
    while(true)
    {
        protocol.MessageFromCom(0);
        sleep(1);
    }
}

void Uart2_Execute::run()
{
    myAPI api;
    while(true)
    {  
        if(myApp::COM3ToServerOpen){
            api.Protocol_1();
        }else{
            protocol.MessageFromCom(1);
        }
        sleep(1);


    }
}

void Uart3_Execute::run()
{
    while(true)
    {

        protocol.MessageFromCom(2);
        sleep(1);

    }
}

void Uart4_Execute::run()
{
    while(true)
    {
        protocol.MessageFromCom(3);
       sleep(1);

    }
}

void Uart5_Execute::run()
{
    while(true)
    {
        protocol.MessageFromCom(4);
        sleep(1);

    }
}

void Uart6_Execute::run()
{
    while(true)
    {
        protocol.MessageFromCom(5);
        sleep(1);

    }
}
void Control_Execute::rain_control()
{
    stopped=false;
}

void Control_Execute::run() //处理控制线程
{
    bool result;
    stopped=false;
    myAPI *api=new myAPI;
    //超标次数清零
    while(!stopped)
    {
        if(myApp::Pro_Rain==1)        //初次降雨
        {
            myApp::cod_overproof=0;
            myApp *rain_pro=new myApp;
            frmValve *catchment=new frmValve;
            result=catchment->Catchment_Valve_Open_Set();
            api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),1,100,result,1,";");
            sleep(myApp::catchmenttime*60);                       //时间界面上设定
            result=catchment->Catchment_Valve_Close_Set();
            api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),1,0,result,1,";");
            delete catchment;
            rain_pro->PronumberChange(2);   //发生做样标识
            delete rain_pro;
            sleep(330); //等待做样完成
        }


        if(myApp::Pro_Rain==7){  //运维状态或者远程控制时候终止降雨
            myApp *rain_pro=new myApp;
            rain_pro->PronumberChange(0);   //等待降雨
            delete rain_pro;
            stopped=true;

        }

        if(myApp::Pro_Rain==3)    //各项指标合格
        {
            if(Valve_CurrentStatus!=IsOpen)
            {
                emit conrainsignal(2,1,2,2,"null");
                myApp *rain_pro=new myApp;
                rain_pro->PronumberChange(2);
                delete rain_pro;
            }
        }

        if(myApp::Pro_Rain==6)  //收到停雨指令
        {
            frmValve *valve =new frmValve;
            myAPI *api =new myAPI;
            result=valve->Valve_Close_Set();
            api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),2,0,result,1,";");
            valve->Catchment_Valve_Close_Set();
            api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),1,0,result,1,";");
            delete valve;
            //添加阀门记录
            myApp *rain_pro=new myApp;
            rain_pro->PronumberChange(0);   //等待降雨
            delete rain_pro;
            stopped=true;
        }
        sleep(5);
    }
    delete api;
}
void Control_Execute::stop()
{
    stopped=true;

}

void RtdProc::run()
{
    myAPI api;
    QString startTime,endTime;
    QDateTime Before,Before1,Before2,Now;

    Now=QDateTime::currentDateTime();
    Before=Now;
    Before1=Now;
    Before2=Now;
    while(true)
    {
        Now=QDateTime::currentDateTime();
        if(Before.secsTo(Now)>=60){//处理实时数据
            Before=Now;
            api.RtdProc();
        }
        if(Before1.secsTo(Now)>=5){
            Before1=Now;
            api.ShowRtd();
        }
        Now=QDateTime::currentDateTime();
        if(Before2.secsTo(Now)>=myApp::RtdInterval){//处理实时数据
            Before2=Now;
            endTime=Now.toString("yyyy-MM-dd hh:mm:ss");
            startTime=Now.addSecs(-myApp::RtdInterval).toString("yyyy-MM-dd hh:mm:ss");
            api.Insert_Message_Rtd(4,startTime);
        }

        msleep(500);

        WDT_Feed();

    }
}

void Count::run()
{
    myAPI api;
    QString startTime,endTime;
    QDateTime Before_m,Before_h,Before_d,Now;

    do{
        Now=QDateTime::currentDateTime();
        Before_m=Now;
        Before_h=Now;
        Before_d=Now;
    }while(Now.time().second()!=0);//整分钟开始计时

    while(true)
    {
        Now=QDateTime::currentDateTime();
        if(Before_m.secsTo(Now)>=myApp::MinInterval*60){//处理分钟数据
            Before_m=Now;
            endTime=Now.toString("yyyy-MM-dd hh:mm:00");
            startTime=Now.addSecs(-myApp::MinInterval*60).toString("yyyy-MM-dd hh:mm:00");
            if(myApp::StType==2){
                api.MinsDataProc_WaterFlow(startTime,endTime);
                api.MinsDataProc_WaterPara(startTime,endTime);
            }
            api.Insert_Message_Count(2051,4,startTime);
        }

        if(Now.time().hour()!=Before_h.time().hour())//整点处理小时数据
        {
            Before_h=Now;
            startTime=Now.addSecs(-3600).toString("yyyy-MM-dd hh:00:00");
            endTime=Now.addSecs(-3600).toString("yyyy-MM-dd hh:59:59");
            if(myApp::StType==2){
                api.HourDataProc_WaterFlow(startTime,endTime);
                api.HourDataProc_WaterPara(startTime,endTime);
            }
            api.Insert_Message_Count(2061,myApp::RespondOpen+4,startTime);
        }

        if(Now.date().day()!=Before_d.date().day())//整点处理日数据
        {
            Before_d=Now;
            startTime=Now.addDays(-1).toString("yyyy-MM-dd 00:00:00");
            endTime=Now.addDays(-1).toString("yyyy-MM-dd 23:59:59");
            api.DayDataProc(startTime,endTime);
            api.Insert_Message_Count(2031,myApp::RespondOpen+4,startTime);
        }

        if(Power_Change)
        {
            Power_Change=0;
            startTime=Now.toString("yyyy-MM-dd hh:mm:ss");

            if(0==Power_New){
                api.Insert_Message_Count(3081,5,startTime);//无市电
            }
            else{
                api.Insert_Message_Count(3082,5,startTime);//有市电
            }

        }
    msleep(500);
    }
}

#include "frmdiagnose.h"
void SendMessage::run()
{
    while(true)
    {
        sleep(10);
        {
        myAPI api;
        api.SendData_Status(4);             //3071状态
        sleep(1);
        api.SendData_Master(2011,0);
        sleep(1);
        api.SendData_Master(2051,0);
        sleep(1);
        api.SendData_Master(3097,myApp::RespondOpen);//ok
        sleep(1);
        //api.SendData_Master(3071,myApp::RespondOpen);//ok
        //api.SendData_Master(3071,0);
        sleep(1);
        api.SendData_Master(9012,0);
        sleep(1);
        api.SendData_Master(3052,myApp::RespondOpen);
        sleep(1);
        api.SendData_Master(3053,myApp::RespondOpen);  //OK
        sleep(1);
        api.SendData_Master(4011,myApp::RespondOpen);   //ok
        sleep(1);
        api.SendData_Master(9999,0);
        sleep(1);
        api.SendData_Master(2061,myApp::RespondOpen);
        sleep(1);
        api.SendData_Master(2031,myApp::RespondOpen);
        sleep(1);
        api.SendData_Master(3053,myApp::RespondOpen);
        sleep(1);
        api.SendData_Master(3081,myApp::RespondOpen);//无市电
        sleep(1);
        api.SendData_Master(3082,myApp::RespondOpen);//有市电
        }

    }
}


void DB_Clear::run()
{
        QString sql;
        QString temp;
        QSqlQuery query,query1;
        QDateTime Now,Before_d;
        Now=QDateTime::currentDateTime();
        Before_d=Now;
        while(true)
        {
            if(Now.date().day()!=Before_d.date().day())
            {
                Before_d=Now;

                temp=Now.addDays(-10).toString("yyyyMMddhhmmsszzz");
                sql=QString("delete from [MessageSend] where [QN]<'%1'").arg(temp);
                query.exec(sql);
                sql=QString("delete from [MessageReceived] where [QN]<'%1'").arg(temp);
                query.exec(sql);
                query.exec("select [Code] from [ParaInfo]");
                QList<QString> listtmp;
                while(query.next()){
                    listtmp.append(query.value(0).toString());
                }
                query.clear();

                for(int i=0;i<listtmp.size();i++){
                    temp=Now.addDays(-366).toString("yyyy-MM-dd hh:mm:ss");
                    sql=QString("delete from [Mins_%1] where [GetTime]<'%2'")
                            .arg(listtmp.at(i))
                            .arg(temp);
                        if(!query1.exec(sql)){
                            qDebug()<<query1.lastError();
                        }
                    query1.clear();

                }
                query.exec("VACUUM");

            }
            Now=QDateTime::currentDateTime();
            sleep(2);
        }

}

extern float ad_version;
extern float ad_value[ANALOG_CNT];
extern float ad_midvalue[ANALOG_CNT];
extern stAnalog_para Analog[ANALOG_CNT];
void SPI_Read_ad::run()
{
    QSqlQuery query;
    myAPI api;
    QString Code;
    QString Name;
    QString  Unit;
    int iLoop,jLoop;
    int Dec;
    int Port;
    float IaValue;
    static float ad[ANALOG_CNT][8];
    static float ad_sort[ANALOG_CNT][8];
    double Rtd;
    QString flag="D";
    SPI_Init();
    memset(ad,0,sizeof(ad));
    memset(ad_sort,0,sizeof(ad_sort));
    
    while(true)
    {
        if(spi_read_ad()==true)
            flag="N";
        else
            flag="D";
        
        if(ad_version > 0){
            for(jLoop=0; jLoop<ANALOG_CNT; jLoop++){
                /*取最新的8个值*/
                for(iLoop=7; iLoop>0; iLoop--){
                    ad[jLoop][iLoop] = ad[jLoop][iLoop-1];
                }
                ad[jLoop][0] = ad_value[jLoop];
                //qDebug()<<QString("Port:")<<jLoop;
                //qDebug()<<QString("ad:     [%1][%2][%3][%4][%5][%6][%7][%8]").arg(ad[jLoop][0]).arg(ad[jLoop][1]).arg(ad[jLoop][2]).arg(ad[jLoop][3]).arg(ad[jLoop][4]).arg(ad[jLoop][5]).arg(ad[jLoop][6]).arg(ad[jLoop][7]);
                memcpy(ad_sort[jLoop],ad[jLoop],sizeof(ad[jLoop]));
                qSort(ad_sort[jLoop],ad_sort[jLoop]+8);
                //qDebug()<<QString("ad_sort:[%1][%2][%3][%4][%5][%6][%7][%8]").arg(ad_sort[jLoop][0]).arg(ad_sort[jLoop][1]).arg(ad_sort[jLoop][2]).arg(ad_sort[jLoop][3]).arg(ad_sort[jLoop][4]).arg(ad_sort[jLoop][5]).arg(ad_sort[jLoop][6]).arg(ad_sort[jLoop][7]);
                ad_midvalue[jLoop] = (ad_sort[jLoop][2] + ad_sort[jLoop][3] + ad_sort[jLoop][4] + ad_sort[jLoop][5])/float(4.00);
                //qDebug()<<QString("ad_midvalue:")<<ad_midvalue[jLoop];
            }
        }
        query.exec("select * from [ParaInfo] where [UseChannel] like 'AN%'");
        while(query.next())
        {
            Name=query.value(0).toString();
            Code=query.value(1).toString();
            Unit=query.value(2).toString();
            Port=query.value(3).toString().right(1).toInt();
            Dec=query.value(15).toInt();
            if(ad_version > 0){
                IaValue = frmcalibration::AD_to_Ia(ad_midvalue[Port],&Analog[Port],1);
            }else{
                IaValue = ad_value[Port];
            }
            Rtd=api.AnalogConvert((double)IaValue,query.value(7).toDouble(),query.value(8).toDouble(),query.value(6).toString());
            api.CacheDataProc(Rtd,0,flag,Dec,Name,Code,Unit);
        }
        msleep(2000);
    }
}
