#include "frmvalve.h"
#include "ui_frmvalve.h"
#include "api/gpio.h"
#include "api/myhelper.h"
#include "api/myapi.h"


enum Status{IsOpen,IsClose,IsError}Valve_CurrentStatus,Valve_RightStatus,Catchment_CurrentStatus,Catchment_RightStatus,reflux_CurrentStatus;
bool Set_Flag;
bool Catchment_Set_Flag;
frmValve::frmValve(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::frmValve)
{
    ui->setupUi(this);

    myHelper::FormInCenter(this,myApp::DeskWidth,myApp::DeskHeigth);//窗体居中显示
    this->InitStyle();
}

frmValve::~frmValve()
{
    delete ui;
}

void frmValve::InitStyle()
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
}
//************************************
// Method:    Delay_MSec
// FullName:  serial::Delay_MSec
// Access:    private
// Qualifier: //非阻塞延时
// Parameter: unsigned int msec
// Returns:   void
// Author:	  Administrator[QuFeng]
// Date:	  2018年8月27日
//************************************
void frmValve::Delay_MSec(unsigned int msec)//非阻塞延时
{
    QTime _Timer = QTime::currentTime().addMSecs(msec);

    while (QTime::currentTime() < _Timer)

        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}
//排水阀门开启输出使能
bool frmValve::Valve_Open_Set()
{
    qDebug()<<QString("Valve Open start");
    if(myApp::Out_drain_close == 26){
        qDebug()<<QString("Valve Open air valve");
        SwitchOut_On(myApp::Out_drain_open);
    }else{
        Valve_Close_Clear();
        Set_Flag=true;
        SwitchOut_On(myApp::Out_drain_open);
        Valve_RightStatus=IsOpen;
        int Valve_Set_Count=0;

        while(Valve_CurrentStatus!=Valve_RightStatus)//判断控制结果
        {
            Valve_Set_Count++;
            Delay_MSec(5000);
            if(Valve_Set_Count>=60)//失败
            {
                Valve_Open_Clear();
                Valve_Close_Clear();
                qDebug()<<QString("Valve Open fail");
                return false;
            }
        }
        if(Valve_RightStatus==IsOpen){
            Valve_Open_Clear();
        }
        if(Valve_RightStatus==IsClose){
            Valve_Close_Clear();
        }
    }
    qDebug()<<QString("Valve Open success");
    return true;
}




//排水阀门关闭输出使能
bool frmValve::Valve_Close_Set()
{
    qDebug()<<QString("Valve Close start");
    if(myApp::Out_drain_close == 26){
        qDebug()<<QString("Valve Close air valve");
        SwitchOut_Off(myApp::Out_drain_open);
    }else{
        Valve_Open_Clear();
        Set_Flag=true;
        SwitchOut_On(myApp::Out_drain_close);
        Valve_RightStatus=IsClose;
        int Valve_Set_Count=0;
        while(Valve_CurrentStatus!=Valve_RightStatus)//判断控制结果
        {
            Valve_Set_Count++;
            Delay_MSec(5000);
            if(Valve_Set_Count>=60)//失败
            {
                Valve_Open_Clear();
                Valve_Close_Clear();
                qDebug()<<QString("Valve Close fail");
                return false;
            }
        }
        if(Valve_RightStatus==IsOpen){
            Valve_Open_Clear();
        }
        if(Valve_RightStatus==IsClose){
            Valve_Close_Clear();
        }
    }
    qDebug()<<QString("Valve Close success");
    return true;
}




//集水阀门开启输出使能
bool frmValve::Catchment_Valve_Open_Set()
{
    qDebug()<<QString("Catchment Valve Open start");
    if(myApp::Out_catchment_close == 26){
        qDebug()<<QString("Catchment Valve Open air valve");
        SwitchOut_On(myApp::Out_catchment_open);
    }else{
        int   Catch_Set_Count=0;
        Catchment_Valve_Close_Clear();
        Catchment_Set_Flag=true;
        SwitchOut_On(myApp::Out_catchment_open);
        Catchment_RightStatus=IsOpen;

        while(Catchment_CurrentStatus!=Catchment_RightStatus)//判断控制结果
        {
            Catch_Set_Count++;
            Delay_MSec(5000);
            if(Catch_Set_Count>=60)//失败
            {
                Catchment_Valve_Close_Clear();
                Catchment_Valve_Open_Clear();
                qDebug()<<QString("Catchment Valve Open fail");
                return false;
            }
        }
        if(Catchment_RightStatus==IsOpen){
            Catchment_Valve_Open_Clear();
        }
        if(Catchment_RightStatus==IsClose){
            Catchment_Valve_Close_Clear();
        }
    }
    qDebug()<<QString("Catchment Valve Open success");
    return true;

}


//集水阀门关闭输出使能
bool frmValve::Catchment_Valve_Close_Set()
{
    qDebug()<<QString("Catchment Valve close start");
    if(myApp::Out_catchment_close == 26){
        qDebug()<<QString("Catchment Valve close air valve");
        SwitchOut_Off(myApp::Out_catchment_open);
    }else{
        Catchment_Valve_Open_Clear();
        Catchment_Set_Flag=true;
        SwitchOut_On(myApp::Out_catchment_close);
        Catchment_RightStatus=IsClose;
        int Catch_Set_Count=0;

        while(Catchment_CurrentStatus!=Catchment_RightStatus)//判断控制结果
        {
            Catch_Set_Count++;
            Delay_MSec(5000);
            if(Catch_Set_Count>=60)//失败
            {
                Catchment_Valve_Close_Clear();
                Catchment_Valve_Open_Clear();
                qDebug()<<QString("Catchment Valve close fail");
                return false;
            }
        }
        if(Catchment_RightStatus==IsOpen){
            Catchment_Valve_Open_Clear();
        }
        if(Catchment_RightStatus==IsClose){
            Catchment_Valve_Close_Clear();
        }
    }
    qDebug()<<QString("Catchment Valve close success");
    return true;
}




//集水阀门开启输出清除
void frmValve::Catchment_Valve_Open_Clear()
{
    SwitchOut_Off(myApp::Out_catchment_open);
    Catchment_Set_Flag=false;
}

//集水阀门关闭输出清除
void frmValve::Catchment_Valve_Close_Clear()
{
    SwitchOut_Off(myApp::Out_catchment_close);
    Catchment_Set_Flag=false;
}

//阀门开启输出清除
void frmValve::Valve_Open_Clear()
{
    SwitchOut_Off(myApp::Out_drain_open);
    Set_Flag=false;
}

//阀门关闭输出清除
void frmValve::Valve_Close_Clear()
{
    SwitchOut_Off(myApp::Out_drain_close);
    Set_Flag=false;
}

//手动开排水阀
void frmValve::on_btn_ValveOpen_clicked()
{
    if(myApp::manual_control==false){
        myHelper::ShowMessageBoxError("请切换到运维模式");
        return ;
    }
    if (myHelper::ShowMessageBoxQuesion("确定开启排水阀吗?")==0){
        int result;
        result=Valve_Open_Set();
        myAPI *api=new myAPI;
        api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),2,100,result,0,";");
        delete api;
    }
}
//手动关排水阀
void frmValve::on_btn_ValveClose_clicked()
{
    if(myApp::manual_control==false){
        myHelper::ShowMessageBoxError("请切换到运维模式");
        return ;
    }
    if (myHelper::ShowMessageBoxQuesion("确定关闭排水阀吗?")==0){
        int result;
        result=Valve_Close_Set();
        myAPI *api=new myAPI;
        api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),2,0,result,0,";");
        delete api;

    }
}
//手动开集水阀
void frmValve::on_btn_CatchValveOpen_clicked()
{
    if(myApp::manual_control==false){
        myHelper::ShowMessageBoxError("请切换到运维模式");
        return ;
    }
    if (myHelper::ShowMessageBoxQuesion("确定开启集水阀吗?")==0){
        int result;
        result=Catchment_Valve_Open_Set();
        myAPI *api=new myAPI;
        api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),1,100,result,0,";");
        delete api;

    }
}


//手动关集水阀
void frmValve::on_btn_CatchValveClose_clicked()
{
    if(myApp::manual_control==false){
        myHelper::ShowMessageBoxError("请切换到运维模式");
        return ;
    }

    if (myHelper::ShowMessageBoxQuesion("确定关闭集水阀吗?")==0){
        int result;
        result=Catchment_Valve_Close_Set();
        myAPI *api=new myAPI;
        api->Insert_Message_Control(3052,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),1,0,result,0,";");
        delete api;

    }
}

void frmValve::on_btn_Cancel_clicked()
{
    this->close();
}


