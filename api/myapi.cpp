#include "myapi.h"
#include "api/myhelper.h"
#include "myapp.h"
#include "api/mythread.h"
#include <unistd.h>
#include <QStandardItemModel>
#include <api/gpio.h>
#include <QSqlError>
extern QextSerialPort *myCom[6];
extern Com_para COM[6];
extern Tcp_para TCP[4];
extern enum Status{IsOpen,IsClose,IsError}Valve_CurrentStatus,Valve_RightStatus,Catchment_CurrentStatus,Catchment_RightStatus,reflux_CurrentStatus;

QList<QString> list;

myAPI::myAPI(QObject *parent) :
    QObject(parent)
{

}


//查找统计数据表内某时间段内某记录是否存在
int myAPI::CountRecordIsExist(QString tableName,QString Time)
{
    QSqlQuery query;
    QString sql;

    sql="select count(*) from "+tableName+" where [GetTime]='"+Time+"'";
    query.exec(sql);
    query.next();

    return (query.value(0).toInt());
}

//查找缓存数据表内某污染物记录是否存在
int myAPI::CacheRecordIsExist(QString Code)
{
    QSqlQuery query(QSqlDatabase::database("memory",true));
    QString sql;
    sql="select count(*) from [CacheRtd]";
    sql+=" where [Code]='"+Code+"'";
    query.exec(sql);
    query.next();

    return (query.value(0).toInt());
}

//查找某数据表是否存在
bool myAPI::TableIsExist(QString tableName)
{
    QSqlQuery query;
    QString sql;

    sql="select count(*) from sqlite_master where type='table' and name='"+tableName+"'";
    query.exec(sql);
    query.next();

    return (query.value(0).toBool());
}

//创建实时数据表
void myAPI::RtdTableCreate(QString tableName)
{
    QSqlQuery query;
    QString sql;
    sql="create table "+tableName+"(GetTime DATETIME(20),";
    sql+="Rtd NVARCHAR(20),Flag NVARCHAR(5),Total NVARCHAR(20))";
    query.exec(sql);

}
//创建统计数据表
void myAPI::CountDataTableCreate(QString tableName)
{
    QSqlQuery query;
    QString sql;
    sql="create table "+tableName+"(GetTime DATETIME(20),";
    sql+="Max NVARCHAR(20),Min NVARCHAR(20),Avg NVARCHAR(20),Cou NVARCHAR(20))";
    query.exec(sql);
}

//统计数据
double myAPI::GetCountDataFromSql(QString tableName,QString StartTime,QString EndTime,QString field,QString func)
{
    QSqlQuery query;
    QString sql;
    double d_value= 0;

    sql="select "+func+"(cast("+field+ " as double)) from "+'['+tableName+']';
    sql+=" where [GetTime]>='"+StartTime+"'"+" and [GetTime]<='"+EndTime+"'";
    sql+=" and [Flag]='N'";

    query.exec(sql);
    if(query.next()){
        QByteArray str=query.value(0).toByteArray();
        d_value=myHelper::Str_To_Double(str.data());
    }
    return d_value;
}

//统计数据
double myAPI::GetCountDataFromSql1(QString tableName,QString StartTime,QString EndTime,QString field,QString func)
{
    QSqlQuery query;
    QString sql;
    double d_value=0;

    sql="select "+func+"(cast("+field+ " as double)) from "+'['+tableName+']';
    sql+=" where [GetTime]>='"+StartTime+"'"+" and [GetTime]<='"+EndTime+"'";
    query.exec(sql);
    if(query.next()){
        QByteArray str=query.value(0).toByteArray();
        d_value=myHelper::Str_To_Double(str.data());
    }
    return d_value;
}

void myAPI::CacheCard(QString cardtype,QString cardno)
{
    QSqlQuery query;
    QString sql;
    QString currenttime=QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    sql=QString("insert into [DoorRecord] ([GetTime],[CardType],[CardNo],[SendTime]) values('%1','%2','%3','0')")
            .arg(currenttime,cardtype,cardno);
    query.exec(sql);
    query.clear();
}


void myAPI::CacheDataProc(double rtd,double total,QString flag,int dec,QString name,QString code,QString unit)
{
    QSqlQuery query(QSqlDatabase::database("memory",true));
    QString sql;
    char str[20];
    int errorNums=0;
    QString format=QString("%.%1f").arg(dec);
    QDateTime dt=QDateTime::currentDateTime();

    if(flag=="D"){
        query.exec(QString("select [ErrorNums] from [CacheRtd] where [Code]='%1'").arg(code));
        if(query.next()){
            errorNums=query.value(0).toInt();
            if(errorNums<5){
                errorNums++;
                sql = "update [CacheRtd] set [ErrorNums]='"+QString::number(errorNums)+"' where [Code]='"+code+"'";
                query.exec(sql);//更新显示数据
                return;
            }else{
                errorNums=5;
            }
        }
    }else{
        errorNums=0;
    }

    if(CacheRecordIsExist(code)==0){
            sql = "insert into [CacheRtd]";
            sql+="([GetTime],[Name],[Code],[Rtd],[Total],[Flag],[ErrorNums])values('";
            sql+= QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")+ "','";
            sql+= name + "','";
            sql+= code + "','";
            sprintf(str,format.toLatin1().data(),rtd);
            sql+= QString(str) +" "+unit+ "','";
            if(total==0){
                sql+= "--','";
            }else{
                sprintf(str,format.toLatin1().data(),total);
                sql+= QString(str) + "','";
            }
            sql+= flag + "','";
            sql+= QString::number(errorNums) + "')";
            query.exec(sql);//插入显示数据

        }
    else{
            sql = "update [CacheRtd] set";
            sql+=" [GetTime]='"+dt.toString("yyyy-MM-dd hh:mm:ss")+"',";
            sql+= "[Name]='"+name + "',";
            sprintf(str,format.toLatin1().data(),rtd);
            sql+= "[Rtd]='"+QString(str) +" "+unit+ "',";
            if(total==0){
                sql+= "[Total]='--',";
            }else
            {
                sprintf(str,format.toLatin1().data(),total);
                sql+= "[Total]='"+QString(str) + "',";
            }
            sql+= "[Flag]='"+flag + "',";
            sql+= "[ErrorNums]='"+QString::number(errorNums)+"'";
            sql+= " where [Code]='"+code+"'";
            query.exec(sql);//更新显示数据

    }
}

void myAPI::RtdProc()
{
    QSqlQuery query1;
    QSqlQuery query(QSqlDatabase::database("memory",true));
    QString sql;
    QString temp,code,rtd,total,dt;

    sql="select * from [CacheRtd] where [flag]='N'";
    query.exec(sql);
    while(query.next())
    {
        dt=query.value(0).toString();
        code=query.value(2).toString();
        rtd=query.value(3).toString().split(" ")[0];
        total=query.value(4).toString();

        temp=QString("Rtd_%1_%2%3%4")
                .arg(code)
                .arg(dt.left(4))
                .arg(dt.mid(5,2))
                .arg(dt.mid(8,2));
        if(!TableIsExist(temp)){
            RtdTableCreate(temp);
        }
        sql = "insert into "+temp+"([GetTime],[Rtd],[Flag],[Total])values('";
        sql+= dt+ "','";
        sql+= rtd + "','";
        sql+= query.value(5).toString() + "','";
        sql+= total + "')";
        query1.exec(sql);//插入实时数据表
    }

}

//水流量数据统计--分钟数据
void myAPI::MinsDataProc_WaterFlow(QString startTime,QString endTime)
{
    bool max_flag,min_flag,avg_flag,cou_flag;
    QString code,format;
    double max_value=0,min_value=0,avg_value=0,cou_value=0;
    char str[20];
    QSqlQuery query,query1;
    QString sql;
    QString temp;
    double total1=0,total2=0;
    query.exec("select * from ParaInfo where [Code]='w00000'");
    while(query.next())
    {
        code=query.value(1).toString();
        if(!TableIsExist(QString("Mins_%1").arg(code))){
            CountDataTableCreate(QString("Mins_%1").arg(code));
        }
        if(CountRecordIsExist(QString("Mins_%1").arg(code),startTime)==true)continue;
        max_flag=query.value(11).toBool();
        min_flag=query.value(12).toBool();
        avg_flag=query.value(13).toBool();
        cou_flag=query.value(14).toBool();
        format=QString("%.%1f").arg(query.value(15).toInt());
        temp=QString("Rtd_%1_%2%3%4")
                .arg(code)
                .arg(startTime.left(4))
                .arg(startTime.mid(5,2))
                .arg(startTime.mid(8,2));
        max_value=GetCountDataFromSql(temp,startTime,endTime,"Rtd","MAX");
        min_value=GetCountDataFromSql(temp,startTime,endTime,"Rtd","MIN");
        avg_value=GetCountDataFromSql(temp,startTime,endTime,"Rtd","AVG");

        sql="select [Total] from "+temp+" where [GetTime]>='"+startTime+"' and [GetTime]<='"+endTime+"'  order by [GetTime] asc";
        query1.exec(sql);
        if(query1.first()){
            total1=myHelper::Str_To_Double(query1.value(0).toByteArray().data());
            query1.last();
            total2=myHelper::Str_To_Double(query1.value(0).toByteArray().data());
        }
        cou_value=total2-total1;

        sql = "insert into Mins_"+code;
        sql+= "([GetTime],[Max],[Min],[Avg],[Cou])values('";
        sql+= startTime+ "','";
        if(max_flag){
            sprintf(str,format.toLatin1().data(),max_value);
            sql+= QString(str) + "','";
        }else{
            sql+= "--','";
        }
        if(min_flag){
            sprintf(str,format.toLatin1().data(),min_value);
            sql+= QString(str) + "','";
        }else{
            sql+="--','";
        }
        if(avg_flag){
            sprintf(str,format.toLatin1().data(),avg_value);
            sql+= QString(str) + "','";
        }else{
            sql+="--','";
        }
        if(cou_flag){
            sprintf(str,format.toLatin1().data(),cou_value);
            sql+= QString(str) + "')";
        }else{
            sql+="--')";
        }
        query1.exec(sql);//插入分钟数据表

    }
}

//水污染因子数据统计--分钟数据
void myAPI::MinsDataProc_WaterPara(QString startTime,QString endTime)
{
    bool max_flag,min_flag,avg_flag,cou_flag;
    QString code,format;
    double max_value=0,min_value=0,avg_value=0,cou_value;
    char str[20];
    QSqlQuery query,query1;
    QString sql;
    QString temp;
    double total=0;
    //query.exec("select * from ParaInfo where [Code]<>'B01'");
    query.exec("select * from ParaInfo where [Code] like 'w_____' and [Code]<>'w00000'");
//    query.exec("select * from ParaInfo where [Code]='B01'");

    while(query.next())
    {
        code=query.value(1).toString();
        if(!TableIsExist(QString("Mins_%1").arg(code))){
            CountDataTableCreate(QString("Mins_%1").arg(code));
        }
        if(CountRecordIsExist(QString("Mins_%1").arg(code),startTime)==true)continue;
        max_flag=query.value(11).toBool();
        min_flag=query.value(12).toBool();
        avg_flag=query.value(13).toBool();
        cou_flag=query.value(14).toBool();
        format=QString("%.%1f").arg(query.value(15).toInt());
        temp=QString("Rtd_%1_%2%3%4")
                .arg(code)
                .arg(startTime.left(4))
                .arg(startTime.mid(5,2))
                .arg(startTime.mid(8,2));
        max_value=GetCountDataFromSql(temp,startTime,endTime,"Rtd","MAX");
        min_value=GetCountDataFromSql(temp,startTime,endTime,"Rtd","MIN");
        avg_value=GetCountDataFromSql(temp,startTime,endTime,"Rtd","AVG");
        sql="select [Cou] from [Mins_w00000] where [GetTime]>='"+startTime+"' and [GetTime]<='"+endTime+"'";
        query1.exec(sql);
        if(query1.next()){
            total=myHelper::Str_To_Double(query1.value(0).toByteArray().data());
        }
        cou_value=total*avg_value*0.001;

        sql = "insert into Mins_"+code;
        sql+= "([GetTime],[Max],[Min],[Avg],[Cou])values('";
        sql+= startTime+ "','";
        if(max_flag){
            sprintf(str,format.toLatin1().data(),max_value);
            sql+= QString(str) + "','";
        }else{
            sql+= "--','";
        }
        if(min_flag){
            sprintf(str,format.toLatin1().data(),min_value);
            sql+= QString(str) + "','";
        }else{
            sql+="--','";
        }
        if(avg_flag){
            sprintf(str,format.toLatin1().data(),avg_value);
            sql+= QString(str) + "','";
        }else{
            sql+="--','";
        }
        if(cou_flag){
            sprintf(str,format.toLatin1().data(),cou_value);
            sql+= QString(str) + "')";
        }else{
            sql+="--')";
        }
        query1.exec(sql);//插入分钟数据表

    }
}

//水流量数据统计--小时数据
void myAPI::HourDataProc_WaterFlow(QString startTime,QString endTime)
{
    bool max_flag,min_flag,avg_flag,cou_flag;
    QString code,format;
    double max_value=0,min_value=0,avg_value=0,cou_value;
    char str[20];
    QSqlQuery query,query1;
    QString sql;
    QString temp;
    double total1=0,total2=0;

    query.exec("select * from ParaInfo where [Code]='w00000'");
    while(query.next())
    {
        code=query.value(1).toString();
        if(!TableIsExist(QString("Hour_%1").arg(code))){
            CountDataTableCreate(QString("Hour_%1").arg(code));
        }
        if(CountRecordIsExist(QString("Hour_%1").arg(code),startTime)==true)continue;
        max_flag=query.value(11).toBool();
        min_flag=query.value(12).toBool();
        avg_flag=query.value(13).toBool();
        cou_flag=query.value(14).toBool();
        format=QString("%.%1f").arg(query.value(15).toInt());
        temp=QString("Rtd_%1_%2%3%4")
                .arg(code)
                .arg(startTime.left(4))
                .arg(startTime.mid(5,2))
                .arg(startTime.mid(8,2));

        max_value=GetCountDataFromSql(temp,startTime,endTime,"Rtd","MAX");
        min_value=GetCountDataFromSql(temp,startTime,endTime,"Rtd","MIN");
        avg_value=GetCountDataFromSql(temp,startTime,endTime,"Rtd","AVG");
        sql="select [Total] from "+temp+" where [GetTime]>='"+startTime+"' and [GetTime]<='"+endTime+"'  order by [GetTime] asc";
        query1.exec(sql);
        if(query1.first()){
            total1=myHelper::Str_To_Double(query1.value(0).toByteArray().data());
            query1.last();
            total2=myHelper::Str_To_Double(query1.value(0).toByteArray().data());
        }
        cou_value=total2-total1;

        sql = "insert into Hour_"+code;
        sql+= "([GetTime],[Max],[Min],[Avg],[Cou])values('";
        sql+= startTime+ "','";
        if(max_flag){
            sprintf(str,format.toLatin1().data(),max_value);
            sql+= QString(str) + "','";
        }else{
            sql+= "--','";
        }
        if(min_flag){
            sprintf(str,format.toLatin1().data(),min_value);
            sql+= QString(str) + "','";
        }else{
            sql+="--','";
        }
        if(avg_flag){
            sprintf(str,format.toLatin1().data(),avg_value);
            sql+= QString(str) + "','";
        }else{
            sql+="--','";
        }
        if(cou_flag){
            sprintf(str,format.toLatin1().data(),cou_value);
            sql+= QString(str) + "')";
        }else{
            sql+="--')";
        }
        query1.exec(sql);//插入小时数据表
    }
}

//水污染因子数据统计--小时数据
void myAPI::HourDataProc_WaterPara(QString startTime,QString endTime)
{
    bool max_flag,min_flag,avg_flag,cou_flag;
    QString code,format;
    double max_value=0,min_value=0,avg_value=0,cou_value=0;
    char str[20];
    QSqlQuery query,query1;
    QString sql;
    QString temp;
    double total=0;

    query.exec("select * from ParaInfo where [Code] like 'w_____' and [Code]<>'w00000'");
    while(query.next())
    {
        code=query.value(1).toString();
        if(!TableIsExist(QString("Hour_%1").arg(code))){
            CountDataTableCreate(QString("Hour_%1").arg(code));
        }
        if(CountRecordIsExist(QString("Hour_%1").arg(code),startTime)==true)continue;
        max_flag=query.value(11).toBool();
        min_flag=query.value(12).toBool();
        avg_flag=query.value(13).toBool();
        cou_flag=query.value(14).toBool();
        format=QString("%.%1f").arg(query.value(15).toInt());
        temp=QString("Rtd_%1_%2%3%4")
                .arg(code)
                .arg(startTime.left(4))
                .arg(startTime.mid(5,2))
                .arg(startTime.mid(8,2));

        max_value=GetCountDataFromSql(temp,startTime,endTime,"Rtd","MAX");
        min_value=GetCountDataFromSql(temp,startTime,endTime,"Rtd","MIN");
        avg_value=GetCountDataFromSql(temp,startTime,endTime,"Rtd","AVG");
        sql="select [Cou] from [Hour_w00000] where [GetTime]>='"+startTime+"' and [GetTime]<='"+endTime+"'";
        query1.exec(sql);
        total=myHelper::Str_To_Double(query1.value(0).toByteArray().data());
        cou_value=total*avg_value*0.001;

        sql = "insert into Hour_"+code;
        sql+= "([GetTime],[Max],[Min],[Avg],[Cou])values('";
        sql+= startTime+ "','";
        if(max_flag){
            sprintf(str,format.toLatin1().data(),max_value);
            sql+= QString(str) + "','";
        }else{
            sql+= "--','";
        }
        if(min_flag){
            sprintf(str,format.toLatin1().data(),min_value);
            sql+= QString(str) + "','";
        }else{
            sql+="--','";
        }
        if(avg_flag){
            sprintf(str,format.toLatin1().data(),avg_value);
            sql+= QString(str) + "','";
        }else{
            sql+="--','";
        }
        if(cou_flag){
            sprintf(str,format.toLatin1().data(),cou_value);
            sql+= QString(str) + "')";
        }else{
            sql+="--')";
        }
        query1.exec(sql);//插入小时数据表
    }
}

void myAPI::DayDataProc(QString startTime,QString endTime)
{
    bool max_flag,min_flag,avg_flag,cou_flag;
    QString code,format;
    double max_value=0,min_value=0,avg_value=0,cou_value;
    char str[20];
    QSqlQuery query,query1;
    QString sql;
    QString temp;

    query.exec("select * from ParaInfo where [Code] like 'w_____'");
    while(query.next())
    {
        code=query.value(1).toString();
        if(!TableIsExist(QString("Day_%1").arg(code))){
            CountDataTableCreate(QString("Day_%1").arg(code));
        }
        if(CountRecordIsExist(QString("Day_%1").arg(code),startTime)==true)continue;
        max_flag=query.value(11).toBool();
        min_flag=query.value(12).toBool();
        avg_flag=query.value(13).toBool();
        cou_flag=query.value(14).toBool();
        format=QString("%.%1f").arg(query.value(15).toInt());
        temp=QString("Hour_%1").arg(code);
        max_value=GetCountDataFromSql1(temp,startTime,endTime,"Max","MAX");
        min_value=GetCountDataFromSql1(temp,startTime,endTime,"Min","MIN");
        avg_value=GetCountDataFromSql1(temp,startTime,endTime,"Avg","AVG");
        cou_value=GetCountDataFromSql1(temp,startTime,endTime,"Cou","SUM");

        sql = "insert into Day_"+code;
        sql+= "([GetTime],[Max],[Min],[Avg],[Cou])values('";
        sql+= startTime+ "','";
        if(max_flag){
            sprintf(str,format.toLatin1().data(),max_value);
            sql+= QString(str) + "','";
        }else{
            sql+= "--','";
        }
        if(min_flag){
            sprintf(str,format.toLatin1().data(),min_value);
            sql+= QString(str) + "','";
        }else{
            sql+="--','";
        }
        if(avg_flag){
            sprintf(str,format.toLatin1().data(),avg_value);
            sql+= QString(str) + "','";
        }else{
            sql+="--','";
        }
        if(cou_flag){
            sprintf(str,format.toLatin1().data(),cou_value);
            sql+= QString(str) + "')";
        }else{
            sql+="--')";
        }
        query1.exec(sql);//插入日数据表
    }
}

//添加事件记录
void myAPI::AddEventInfo(QString TriggerType, QString TriggerContent)
{
    QString sql = "insert into [EventInfo]([TriggerTime],[TriggerType],";
    sql+="[TriggerContent],[TriggerUser])values('";
    sql += QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + "','";
    sql += TriggerType + "','";
    sql += TriggerContent + "','";
    sql += myApp::CurrentUserName + "')";
    QSqlQuery query;
    query.exec(sql);
}

void myAPI::AddEventInfoUser(QString TriggerContent)
{
  this->AddEventInfo("用户操作", TriggerContent);
}


//读取监测污染物信息
QStandardItemModel  *model_rtd;
void myAPI::ShowRtd()
{
    QSqlQuery query(QSqlDatabase::database("memory",true));
    query.exec("select [Name],[Rtd],[Total],[Flag] from [CacheRtd]");
    int rows=0;
    while(query.next())
    {
        for(int i=0;i<model_rtd->columnCount();i++){
            model_rtd->setItem(rows,i,new QStandardItem(query.value(i).toString()));
            model_rtd->item(rows,i)->setTextAlignment(Qt::AlignCenter);           //设置字符位置
            if(rows%2==1){
                if(query.value(i).toString()=="T"){
                    model_rtd->item(rows,i)->setBackground(QBrush(QColor(150,0,0,80)));
                }else{
                    model_rtd->item(rows,i)->setBackground(QBrush(QColor(0,255,0,80)));
                }
            }
        }
        rows++;
    }

}

void myAPI::InsertList(QString str)
{
    if(list.size()>=20){
        list.clear();
    }
    list.append(str);

}

void myAPI::Insert_Message_Count(int CN,int flag,QString dt)
{
    QString content;
    QSqlQuery query,query1;
    bool max_flag,min_flag,avg_flag,cou_flag;
    QByteArray ch;
    QString code;
    QString sql;
    QString QN=QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");
    content="QN="+QN+";ST="+ST_to_Str[myApp::StType];
    content+=";CN="+QString::number(CN)+";PW=123456;MN="+myApp::MN;
    content+=";Flag="+QString::number(flag)+";CP=&&DataTime="+QString("%1%2%3%4%5%6")
            .arg(dt.mid(0,4))
            .arg(dt.mid(5,2))
            .arg(dt.mid(8,2))
            .arg(dt.mid(11,2))
            .arg(dt.mid(14,2))
            .arg(dt.mid(17,2));

        if(CN==3081||CN==3082){
        content+="&&";
        ch.resize(4);
        int len=content.size();
        int crc=myHelper::CRC16_GB212(content.toLatin1().data(),len);
        sprintf(ch.data(),"%.4X",crc);
        content.append(ch+"\r\n");
        ch[0] = (len/1000)+'0';
        ch[1] = (len%1000/100)+'0';
        ch[2] = (len%100/10)+'0';
        ch[3] = (len%10)+'0';
        content.insert(0,"##");
        content.insert(2,ch);
        qDebug()<<content;
        sql = "insert into MessageSend([QN],[CN],[Content],";
        sql+="[Target1_SendTimes],[Target2_SendTimes],[Target3_SendTimes],[Target4_SendTimes],[Target5_SendTimes],";
        sql+="[Target1_IsRespond],[Target2_IsRespond],[Target3_IsRespond],[Target4_IsRespond],[Target5_IsRespond])values('";
        sql+= QN+ "','"+QString::number(CN)+"','"+content+"','0','0','0','0','0','false','false','false','false','false')";\
        query.exec(sql);//插入发送数据表
        return;
    }
        query.exec("select * from ParaInfo");
        while(query.next())
        {
            code=query.value(1).toString();
            max_flag=query.value(11).toBool();
            min_flag=query.value(12).toBool();
            avg_flag=query.value(13).toBool();
            cou_flag=query.value(14).toBool();
            if(CN==2051){
                query1.exec(QString("select * from [Mins_%1] where [GetTime]='%2'").arg(code).arg(dt));
            }else if(CN==2061){
                query1.exec(QString("select * from [Hour_%1] where [GetTime]='%2'").arg(code).arg(dt));
            }else if(CN==2031){
                query1.exec(QString("select * from [Day_%1] where [GetTime]='%2'").arg(code).arg(dt));
            }else {
                return;
            }

            while(query1.next())
            {
                if(max_flag)
                {
                    content+=';'+code+"-Max="+query1.value(1).toString();
                }
                if(min_flag)
                {
                    content+=','+code+"-Min="+query1.value(2).toString();
                }
                if(avg_flag)
                {
                    content+=','+code+"-Avg="+query1.value(3).toString();
                }
                if(cou_flag)
                {
                    content+=','+code+"-Cou="+query1.value(4).toString();
                }
                content+=','+code+"-Flag=N;";

            }

        }

    content+="&&";

    ch.resize(4);
    int len=content.size();
    int crc=myHelper::CRC16_GB212(content.toLatin1().data(),len);
    sprintf(ch.data(),"%.4X",crc);
    content.append(ch+"\r\n");
    ch[0] = (len/1000)+'0';
    ch[1] = (len%1000/100)+'0';
    ch[2] = (len%100/10)+'0';
    ch[3] = (len%10)+'0';
    content.insert(0,"##");
    content.insert(2,ch);
    if(flag&0X01){
        sql = "insert into MessageSend([QN],[CN],[Content],";
        sql+="[Target1_SendTimes],[Target2_SendTimes],[Target3_SendTimes],[Target4_SendTimes],[Target5_SendTimes],";
        sql+="[Target1_IsRespond],[Target2_IsRespond],[Target3_IsRespond],[Target4_IsRespond],[Target5_IsRespond])values('";
        sql+= QN+ "','"+QString::number(CN)+"','"+content+"','0','0','0','0','0','false','false','false','false','false')";
    }
    else{
        sql = "insert into MessageSend([QN],[CN],[Content],";
        sql+="[Target1_SendTimes],[Target2_SendTimes],[Target3_SendTimes],[Target4_SendTimes],[Target5_SendTimes],";
        sql+="[Target1_IsRespond],[Target2_IsRespond],[Target3_IsRespond],[Target4_IsRespond],[Target5_IsRespond])values('";
        sql+= QN+ "','"+QString::number(CN)+"','"+content+"','0','0','0','0','0','--','--','--','--','--')";
    }
    query.exec(sql);//插入发送数据表

}

//Per=0表示关阀请求，Per=100表示开阀请求
void myAPI::Insert_Message_Control(int CN,int flag,QString dt,int valvenum,int per,bool isok,int contyp,QString str_tmp)
{
    QString content;
    QSqlQuery query;
    QByteArray ch;
    int result=0;
    QString sql;
    QString QN=QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");
    content="QN="+QN+";ST="+ST_to_Str[myApp::StType];
    content+=";CN="+QString::number(CN)+";PW=123456;MN="+myApp::MN;
    content+=";Flag="+QString::number(flag)+";CP=&&DataTime="+QString("%1%2%3%4%5%6")
            .arg(dt.mid(0,4))
            .arg(dt.mid(5,2))
            .arg(dt.mid(8,2))
            .arg(dt.mid(11,2))
            .arg(dt.mid(14,2))
            .arg(dt.mid(17,2));

    if(CN==3052){
        content+=QString(";ValveNum=%1;Per=%2;").arg(valvenum).arg(per) ;
        if(isok==true)
        {
            result=1;
        }
        else
        {
            result=0;
        }
        content+=QString("IsSucceed=%1;ConType=%2;").arg(result).arg(contyp) ;

        if(contyp==1&&valvenum==2&&per==0) //超标
        {
            content+=str_tmp;
        }
        content+="&&";
        ch.resize(4);
        int len=content.size();
        int crc=myHelper::CRC16_GB212(content.toLatin1().data(),len);
        sprintf(ch.data(),"%.4X",crc);
        content.append(ch+"\r\n");
        ch[0] = (len/1000)+'0';
        ch[1] = (len%1000/100)+'0';
        ch[2] = (len%100/10)+'0';
        ch[3] = (len%10)+'0';
        content.insert(0,"##");
        content.insert(2,ch);
        sql = "insert into MessageSend([QN],[CN],[Content],";
        sql+="[Target1_SendTimes],[Target2_SendTimes],[Target3_SendTimes],[Target4_SendTimes],[Target5_SendTimes],";
        sql+="[Target1_IsRespond],[Target2_IsRespond],[Target3_IsRespond],[Target4_IsRespond],[Target5_IsRespond])values('";
        sql+= QN+ "','"+QString::number(CN)+"','"+content+"','0','0','0','0','0','false','false','false','false','false')";\
        query.exec(sql);//插入发送数据表
        return;
    }

}

void myAPI::Insert_Message_VSErr(int CN,int flag,QString dt,QString str_tmp)
{
    QString content;
    QSqlQuery query;
    QByteArray ch;
    QString sql;
    QString QN=QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");
    content="QN="+QN+";ST="+ST_to_Str[myApp::StType];
    content+=";CN="+QString::number(CN)+";PW=123456;MN="+myApp::MN;
    content+=";Flag="+QString::number(flag)+";CP=&&DataTime="+QString("%1%2%3%4%5%6")
            .arg(dt.mid(0,4))
            .arg(dt.mid(5,2))
            .arg(dt.mid(8,2))
            .arg(dt.mid(11,2))
            .arg(dt.mid(14,2))
            .arg(dt.mid(17,2));

        content+=";";
        content+=str_tmp ;
        content+="&&";
        ch.resize(4);
        int len=content.size();
        int crc=myHelper::CRC16_GB212(content.toLatin1().data(),len);
        sprintf(ch.data(),"%.4X",crc);
        content.append(ch+"\r\n");
        ch[0] = (len/1000)+'0';
        ch[1] = (len%1000/100)+'0';
        ch[2] = (len%100/10)+'0';
        ch[3] = (len%10)+'0';
        content.insert(0,"##");
        content.insert(2,ch);
        sql = "insert into MessageSend([QN],[CN],[Content],";
        sql+="[Target1_SendTimes],[Target2_SendTimes],[Target3_SendTimes],[Target4_SendTimes],[Target5_SendTimes],";
        sql+="[Target1_IsRespond],[Target2_IsRespond],[Target3_IsRespond],[Target4_IsRespond],[Target5_IsRespond])values('";
        sql+= QN+ "','"+QString::number(CN)+"','"+content+"','0','0','0','0','0','false','false','false','false','false')";\
        query.exec(sql);//插入发送数据表

}

void myAPI::Insert_Message_Exertn(int CN,int flag,bool isok,QString qn) //CN=9012
{
    QString content;
    QSqlQuery query;
    QByteArray ch;
    int result=0;
    QString sql;
    QString QN=qn;
    if(isok==true)
    {
        result=1;
    }
    else
    {
        result=0;
    }
    content="QN="+QN+";ST="+ST_to_Str[myApp::StType];
    content+=";CN="+QString::number(CN)+";PW=123456;MN="+myApp::MN;
    content+=";Flag="+QString::number(flag)+";CP=&&ExeRtn="+QString::number(result);

    content+="&&";
    ch.resize(4);
    int len=content.size();
    int crc=myHelper::CRC16_GB212(content.toLatin1().data(),len);
    sprintf(ch.data(),"%.4X",crc);
    content.append(ch+"\r\n");
    ch[0] = (len/1000)+'0';
    ch[1] = (len%1000/100)+'0';
    ch[2] = (len%100/10)+'0';
    ch[3] = (len%10)+'0';
    content.insert(0,"##");
    content.insert(2,ch);
    sql = "insert into MessageSend([QN],[CN],[Content],";
    sql+="[Target1_SendTimes],[Target2_SendTimes],[Target3_SendTimes],[Target4_SendTimes],[Target5_SendTimes],";
    sql+="[Target1_IsRespond],[Target2_IsRespond],[Target3_IsRespond],[Target4_IsRespond],[Target5_IsRespond])values('";
    sql+= QN+ "','"+QString::number(CN)+"','"+content+"','0','0','0','0','0','--','--','--','--','--')";\
    query.exec(sql);//插入发送数据表

}

void myAPI::Insert_Message_Rtd(int flag,QString dt)
{
    QString content;
    QSqlQuery query1;
    QSqlQuery query(QSqlDatabase::database("memory",true));
    QByteArray ch;
    QString sql;
    QString QN=QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");

    content="QN="+QN+";ST="+ST_to_Str[myApp::StType];
    content+=";CN=2011;PW=123456;MN="+myApp::MN+";Flag="+QString::number(flag)+";CP=&&DataTime="+QString("%1%2%3%4%5%6")
            .arg(dt.mid(0,4))
            .arg(dt.mid(5,2))
            .arg(dt.mid(8,2))
            .arg(dt.mid(11,2))
            .arg(dt.mid(14,2))
            .arg(dt.mid(17,2));
    query.exec("select * from [CacheRtd]");
    while(query.next())
    {
        content+=";"+query.value(2).toString()+"-Rtd="+query.value(3).toString().split(" ")[0];
        content+=","+query.value(2).toString()+"-Flag="+query.value(5).toString();
        if(query.value(2).toString() == "w01018"){
            content+=","+query.value(2).toString()+"-DataTime="+myApp::CODSampleTime;
        }
    }
    content+="&&";

    ch.resize(4);
    int len=content.size();
    int crc=myHelper::CRC16_GB212(content.toLatin1().data(),len);
    sprintf(ch.data(),"%.4X",crc);
    content.append(ch+"\r\n");
    ch[0] = (len/1000)+'0';
    ch[1] = (len%1000/100)+'0';
    ch[2] = (len%100/10)+'0';
    ch[3] = (len%10)+'0';
    content.insert(0,"##");
    content.insert(2,ch);

    if(flag){
        sql = "insert into MessageSend([QN],[CN],[Content],";
        sql+="[Target1_SendTimes],[Target2_SendTimes],[Target3_SendTimes],[Target4_SendTimes],[Target5_SendTimes],";
        sql+="[Target1_IsRespond],[Target2_IsRespond],[Target3_IsRespond],[Target4_IsRespond],[Target5_IsRespond])values('";
        sql+= QN+ "','2011','"+content+"','0','0','0','0','0','false','false','false','false','false')";
    }
    else{
        sql = "insert into MessageSend([QN],[CN],[Content],";
        sql+="[Target1_SendTimes],[Target2_SendTimes],[Target3_SendTimes],[Target4_SendTimes],[Target5_SendTimes],";
        sql+="[Target1_IsRespond],[Target2_IsRespond],[Target3_IsRespond],[Target4_IsRespond],[Target5_IsRespond])values('";
        sql+= QN+ "','2011','"+content+"','0','0','0','0','0','--','--','--','--','--')";
    }
    query1.exec(sql);//插入发送实时数据表

}

void myAPI::Update_Respond(QString QN,QString From)
{
    QSqlQuery query;
    QString sql;

    if(From==QString("%1:%2").arg(TCP[0].ServerIP).arg(TCP[0].ServerPort)){
        sql = "update [MessageSend] set [Target1_IsRespond]='true' where [QN]='"+QN+"'";
        query.exec(sql);//更新接收标记
    }else if(From==QString("%1:%2").arg(TCP[1].ServerIP).arg(TCP[1].ServerPort)){
        sql = "update [MessageSend] set [Target2_IsRespond]='true' where [QN]='"+QN+"'";
        query.exec(sql);//更新接收标记
    }else if(From==QString("%1:%2").arg(TCP[2].ServerIP).arg(TCP[2].ServerPort)){
        sql = "update [MessageSend] set [Target3_IsRespond]='true' where [QN]='"+QN+"'";
        query.exec(sql);//更新接收标记
    }else if(From==QString("%1:%2").arg(TCP[3].ServerIP).arg(TCP[3].ServerPort)){
        sql = "update [MessageSend] set [Target4_IsRespond]='true' where [QN]='"+QN+"'";
        query.exec(sql);//更新接收标记
    }else if(From=="COM3"){
        sql = "update [MessageSend] set [Target5_IsRespond]='true' where [QN]='"+QN+"'";
        query.exec(sql);//更新接收标记
    }

    sql = "update [MessageReceived] set [IsProcessed]='true' where [QN]='"+QN+"'";
    query.exec(sql);//更新处理标记

}

void myAPI::Insert_Message_Received(QString QN,int CN,QString From,QString Content)
{
    QString sql;
    QSqlQuery query;

    sql = QString("insert into MessageReceived([ReceivedTime],[QN],[CN],[From],[IsProcessed],[Content])values('%1','%2','%3','%4','false','%5')")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
            .arg(QN)
            .arg(CN)
            .arg(From)
            .arg(Content);

    query.exec(sql);//插入接收数据表
}


extern QTcpSocket *tcpSocket1;
extern QTcpSocket *tcpSocket2;
extern QTcpSocket *tcpSocket3;
extern QTcpSocket *tcpSocket4;
extern Tcp_para     TCP[4];
extern QextSerialPort *myCom[6];
void myAPI::SendData_Status(int flag)
{
    QString content;
    QByteArray ch;
    QString dt;
    QString QN=QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");
    dt=QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    content="QN="+QN+";ST="+ST_to_Str[myApp::StType];
    content+=";CN=3071;PW=123456;MN="+myApp::MN+";Flag="+QString::number(flag)+";CP=&&DataTime="+QString("%1%2%3%4%5%6;")
            .arg(dt.mid(0,4))
            .arg(dt.mid(5,2))
            .arg(dt.mid(8,2))
            .arg(dt.mid(11,2))
            .arg(dt.mid(14,2))
            .arg(dt.mid(17,2));
    if(Catchment_CurrentStatus==IsClose){
        content+="Per1=0;";
    }
    else{
        content+="Per1=100;";
    }
    if(Valve_CurrentStatus==IsClose){
        content+="Per2=0;";
    }
    else{
        content+="Per2=100;";

    }
    if(reflux_CurrentStatus==IsClose){
        content+="Pump=OFF;";
    }
    else{
        content+="Pump=ON;";
    }
    content+="Gate="+myApp::DoorStatus+";";
    content+="LV="+myApp::LVStatus+";";
    content+="BotID="+QString::number(myApp::BottleId);
    content+="&&";

    ch.resize(4);
    int len=content.size();
    int crc=myHelper::CRC16_GB212(content.toLatin1().data(),len);
    sprintf(ch.data(),"%04X",crc);
    content.append(ch+"\r\n");
    ch[0] = (len/1000)+'0';
    ch[1] = (len%1000/100)+'0';
    ch[2] = (len%100/10)+'0';
    ch[3] = (len%10)+'0';
    content.insert(0,"##");
    content.insert(2,ch);
    if(TCP[0].ServerOpen==true)
    {
            if(TCP[0].isConnected){
                 tcpSocket1->write(content.toAscii());
                 tcpSocket1->flush();
            }

    }
    if(TCP[1].ServerOpen==true)
    {
            if(TCP[1].isConnected){
                 tcpSocket2->write(content.toAscii());
                 tcpSocket2->flush();
            }

    }
    if(TCP[2].ServerOpen==true)
    {
            if(TCP[2].isConnected){
                 tcpSocket3->write(content.toAscii());
                 tcpSocket3->flush();
            }

    }
    if(TCP[3].ServerOpen==true)
    {
            if(TCP[3].isConnected){
                 tcpSocket4->write(content.toAscii());
                 tcpSocket4->flush();
            }

    }


    if(myApp::COM3ToServerOpen==true)
    {
           if(myCom[1]->isOpen()){
               myCom[1]->write(content.toAscii());
               myCom[1]->flush();
           }
    }

}

void myAPI::SendData_Master(int CN,int flag)
{
    QString sql,data;
    QSqlQuery query,query1;
    if(TCP[0].ServerOpen==true)
    {
        if(flag&0x01)
        {
            sql=QString("select [QN],[Content],[Target1_SendTimes] from [MessageSend] where [CN]='%1' and [Target1_SendTimes]<'%2' and [Target1_IsRespond]='false'")
                    .arg(CN).arg(myApp::ReCount);
        }else{
            sql=QString("select [QN],[Content], [Target1_SendTimes] from [MessageSend] where [CN]='%1' and [Target1_SendTimes]<'1'")
                    .arg(CN);
        }
        query.exec(sql);

        if(query.first())
        {
            if(TCP[0].isConnected){
                 tcpSocket1->write(query.value(1).toByteArray());
                 tcpSocket1->flush();
                 sql = QString("update [MessageSend] set [Target1_SendTimes]='%1' where [QN]='%2'")
                         .arg(query.value(2).toInt()+1).arg(query.value(0).toString());
                 query1.exec(sql);//更新发送次数
                 query1.clear();
                 data=QString("Tx[%1:%2]:%3").arg(TCP[0].ServerIP).arg(TCP[0].ServerPort).arg(query.value(1).toString());
                 InsertList(data);

            }else{
                if(CN==2011||CN==2051){//实时数据和分钟数据不自动补发
                    sql = QString("update [MessageSend] set [Target1_SendTimes]='%1' where [QN]='%2'")
                            .arg(query.value(2).toInt()+1).arg(query.value(0).toString());
                    query1.exec(sql);//更新发送次数
                    query1.clear();
                }
            }

        }
    }

    if(TCP[1].ServerOpen==true)
    {
        if(flag&0x01)
        {
            sql=QString("select [QN],[Content],[Target2_SendTimes] from [MessageSend] where [CN]='%1' and [Target2_SendTimes]<'%2' and [Target2_IsRespond]='false'")
                    .arg(CN).arg(myApp::ReCount);
        }else{
            sql=QString("select [QN],[Content], [Target2_SendTimes] from [MessageSend] where [CN]='%1' and [Target2_SendTimes]<'1'")
                    .arg(CN);
        }
        query.exec(sql);
        if(query.first())
        {
            if(TCP[1].isConnected){
                 tcpSocket2->write(query.value(1).toByteArray());
                 tcpSocket2->flush();
                 sql = QString("update [MessageSend] set [Target2_SendTimes]='%1' where [QN]='%2'")
                         .arg(query.value(2).toInt()+1).arg(query.value(0).toString());
                 query1.exec(sql);//更新发送次数
                 query1.clear();
                 data=QString("Tx[%1:%2]:%3").arg(TCP[1].ServerIP).arg(TCP[1].ServerPort).arg(query.value(1).toString());
                 InsertList(data);

            }else{
                if(CN==2011||CN==2051){//实时数据和分钟数据不自动补发
                    sql = QString("update [MessageSend] set [Target2_SendTimes]='%1' where [QN]='%2'")
                            .arg(query.value(2).toInt()+1).arg(query.value(0).toString());
                    query1.exec(sql);//更新发送次数
                    query1.clear();
                }
            }

        }
    }

    if(TCP[2].ServerOpen==true)
    {
        if(flag&0x01)
        {
            sql=QString("select [QN],[Content],[Target3_SendTimes] from [MessageSend] where [CN]='%1' and [Target3_SendTimes]<'%2' and [Target3_IsRespond]='false'")
                    .arg(CN).arg(myApp::ReCount);
        }else{
            sql=QString("select [QN],[Content], [Target3_SendTimes] from [MessageSend] where [CN]='%1' and [Target3_SendTimes]<'1'")
                    .arg(CN);
        }
        query.exec(sql);
        if(query.first())
        {
            if(TCP[2].isConnected){
                 tcpSocket3->write(query.value(1).toByteArray());
                 tcpSocket3->flush();
                 sql = QString("update [MessageSend] set [Target3_SendTimes]='%1' where [QN]='%2'")
                         .arg(query.value(2).toInt()+1).arg(query.value(0).toString());
                 query1.exec(sql);//更新发送次数
                 query1.clear();
                 data=QString("Tx[%1:%2]:%3").arg(TCP[2].ServerIP).arg(TCP[2].ServerPort).arg(query.value(1).toString());
                 InsertList(data);

            }else{
                if(CN==2011||CN==2051){//实时数据和分钟数据不自动补发
                    sql = QString("update [MessageSend] set [Target3_SendTimes]='%1' where [QN]='%2'")
                            .arg(query.value(2).toInt()+1).arg(query.value(0).toString());
                    query1.exec(sql);//更新发送次数
                    query1.clear();
                }
            }

        }
    }

    if(TCP[3].ServerOpen==true)
    {
        if(flag&0x01)
        {
            sql=QString("select [QN],[Content],[Target4_SendTimes] from [MessageSend] where [CN]='%1' and [Target4_SendTimes]<'%2' and [Target4_IsRespond]='false'")
                    .arg(CN).arg(myApp::ReCount);
        }else{
            sql=QString("select [QN],[Content], [Target4_SendTimes] from [MessageSend] where [CN]='%1' and [Target4_SendTimes]<'1'")
                    .arg(CN);
        }
        if(!query.exec(sql)){
            qDebug()<<query.lastError();
        }
        if(query.first())
        {
            if(TCP[3].isConnected){
                 tcpSocket4->write(query.value(1).toByteArray());
                 tcpSocket4->flush();
                 data=QString("Tx[%1:%2]:%3").arg(TCP[3].ServerIP).arg(TCP[3].ServerPort).arg(query.value(1).toString());
                 InsertList(data);
                 qDebug()<<data;
                 sql = QString("update [MessageSend] set [Target4_SendTimes]='%1' where [QN]='%2'")
                         .arg(query.value(2).toInt()+1).arg(query.value(0).toString());
                 if(!query1.exec(sql)){
                     qDebug()<<query1.lastError();
                 }
                 query1.clear();

            }else{
                if(CN==2011||CN==2051){//实时数据和分钟数据不自动补发
                    sql = QString("update [MessageSend] set [Target4_SendTimes]='%1' where [QN]='%2'")
                            .arg(query.value(2).toInt()+1).arg(query.value(0).toString());
                    query1.exec(sql);//更新发送次数
                    query1.clear();
                }
            }

        }
    }



    if(myApp::COM3ToServerOpen==true)
    {
        if(flag&0x01)
        {
            sql=QString("select [QN],[Content],[Target5_SendTimes] from [MessageSend] where [CN]='%1' and [Target5_SendTimes]<'%2' and [Target5_IsRespond]='false'")
                    .arg(CN).arg(myApp::ReCount);
        }else{
            sql=QString("select [QN],[Content],[Target5_SendTimes] from [MessageSend] where [CN]='%1' and [Target5_SendTimes]<'1'")
                    .arg(CN);
        }
        query.exec(sql);
        if(query.first())
        {
           if(myCom[1]->isOpen()){
               myCom[1]->write(query.value(1).toByteArray());
               sql = QString("update [MessageSend] set [Target5_SendTimes]='%1' where [QN]='%2'")
                       .arg(query.value(2).toInt()+1).arg(query.value(0).toString());
               query1.exec(sql);//更新发送次数
               query1.clear();
               data=QString("Tx[COM3]:%1").arg(query.value(1).toString());
               InsertList(data);

           }else{
                if(CN==2011||CN==2051){
                    sql = QString("update [MessageSend] set [Target5_SendTimes]='%1' where [QN]='%2'")
                            .arg(query.value(2).toInt()+1).arg(query.value(0).toString());
                    query1.exec(sql);//更新发送次数
                    query1.clear();
                }
           }
        }
    }

}
//**************************************************************************************
//模拟采集电压转成实际值
double myAPI::AnalogConvert(double adValue,double RangeUp,double RangeLow,QString Signal)
{
    double realValue=0;

    if(Signal=="4-20mA")
    {
        realValue=(RangeUp-RangeLow)*(adValue*10-4)/16+RangeLow;
    }
    else if(Signal=="0-20mA"){
        realValue=(RangeUp-RangeLow)*(adValue*10-0)/20+RangeLow;
    }
    else if(Signal=="1-5V"){
        realValue=(RangeUp-RangeLow)*(adValue-1)/4+RangeLow;
    }
    else if(Signal=="0-5V"){
        realValue=(RangeUp-RangeLow)*(adValue-0)/5+RangeLow;
    }



    return realValue;
}

//**************************************************************************************
//GB212协议
extern Message message_api;

void myAPI::Protocol_1()
{
    int  tt=0;
    QByteArray temp;
    QByteArray readbuf;
    do{
        temp=myCom[1]->readAll();
        readbuf+=temp;
        if(temp.size()==0)tt++;
        else tt=0;
        if(tt>COM[1].Timeout/20)break;
        usleep(20000);
    }while(!readbuf.endsWith("\r\n"));

    if(tt<=COM[1].Timeout/20&&readbuf.size()>0){
        qDebug()<<QString("COM3 received:%1").arg(readbuf.data());

        message_api.messageProc(readbuf,myCom[1],NULL);
    }
}




//明渠流量计
void myAPI::Protocol_2(int port,int Address,int Dec,QString Name,QString Code,QString Unit)
{
#ifdef _DEBUG
        double rtd=0;
        static double total=0;
        rtd = 10.0;    
        total += 0.01;
        CacheDataProc(rtd,total,"N",Dec,Name,Code,Unit);
#else
    double rtd=0;
    double total=0;
    QString flag="D";
    QByteArray readbuf;
    QByteArray sendbuf;
    int check=0;
   volatile char s[4];

    sendbuf.resize(8);
    sendbuf[0]=Address;
    sendbuf[1]=0x03;
    sendbuf[2]=0x00;
    sendbuf[3]=0x08;
    sendbuf[4]=0x00;
    sendbuf[5]=0x04;
    check = myHelper::CRC16_Modbus(sendbuf.data(),6);
    sendbuf[6]=(char)(check);
    sendbuf[7]=(char)(check>>8);
    myCom[port]->write(sendbuf);
    sleep(1);
    readbuf=myCom[port]->readAll();
    if(readbuf.length()>=13){
        if(Address==readbuf[0])
        {
                check = myHelper::CRC16_Modbus(readbuf.data(),11);
                if((readbuf[11]==(char)(check&0xff))&&(readbuf[12]==(char)(check>>8)))
                {
                    s[0]=readbuf[6];
                    s[1]=readbuf[5];
                    s[2]=readbuf[4];
                    s[3]=readbuf[3];
                    rtd=*(float *)s;        //瞬时流量单位M3/H
                    s[0]=readbuf[10];
                    s[1]=readbuf[9];
                    s[2]=readbuf[8];
                    s[3]=readbuf[7];
                    total=*(float *)s;        //累计流量单位M3
                    flag='N';
                }
        }
    }

    CacheDataProc(rtd,total,flag,Dec,Name,Code,Unit);
#endif
}
//C10电导率
void myAPI::Protocol_3(int port,int Address,int Dec,QString Name,QString Code,QString Unit)
{
#ifdef _DEBUG
    double rtd=0;
    rtd = 10.0;    
    CacheDataProc(rtd,0,"N",Dec,Name,Code,Unit);
#else
    #ifdef _TEST
    static int SendCNT = 0;
    static int SendSuccessCNT = 0;
    static int SendZeroCNT = 0;
    static int SendNegativeCNT = 0;
    #endif
    double rtd=0;
    QString flag="D";
    QByteArray readbuf;
    QByteArray sendbuf;
    char check=0;
    char ch_temp;
    volatile char s[4];
    sendbuf.resize(14);
    sendbuf[0]=0x40;
    sendbuf[1]=((Address >> 4) & 0x0f)+0x30;
    sendbuf[2]=(Address&0x0f)+0x30;
    sendbuf[3]='C';
    sendbuf[4]='8';
    sendbuf[5]='0';
    sendbuf[6]='0';
    sendbuf[7]='0';
    sendbuf[8]='0';
    sendbuf[9]='0';
    sendbuf[10]='8';
    check=myHelper::XORValid(&sendbuf.data()[1],10);
    ch_temp = (check  >> 4) & 0x0F;  //取高位数；
    if (ch_temp < 10)                       //低于10的数
        ch_temp = ch_temp  +  '0';
    else
        ch_temp = (ch_temp - 10 ) +  'A';   //不低于10的16进制数，如：A、B、C、D、E、F
    sendbuf[11]=ch_temp;
    ch_temp = check & 0x0F;  //取低位数；
    if (ch_temp < 10) ch_temp = ch_temp  +  '0';
    else ch_temp = (ch_temp - 10 )+  'A';
    sendbuf[12]=ch_temp;
    sendbuf[13]=0x0D;
    readbuf=myCom[port]->readAll();
    myCom[port]->write(sendbuf);
    #ifdef _TEST
        SendCNT++;
    #endif
    sleep(3);
    readbuf=myCom[port]->readAll();
    qDebug()<<QString("COM%1 received:").arg(port+2)<<readbuf.toHex().toUpper();
    sleep(1);
    if(readbuf.length()>=24&&0x40==readbuf[0]){
        if(Address==(readbuf[1]-0x30)*16+readbuf[2]-0x30)
        {
            check = myHelper::XORValid(readbuf.mid(1,readbuf.length()-4),readbuf.length()-4);
            ch_temp = (check  >> 4) & 0x0F;  //取高位数；
            if (ch_temp < 10)                       //低于10的数
                ch_temp = ch_temp  +  '0';
            else
                ch_temp = (ch_temp - 10 ) +  'A';   //不低于10的16进制数，如：A、B、C、D、E、F
            if(readbuf[readbuf.length()-3]!=ch_temp) return;

            ch_temp = check & 0x0F;  //取低位数；
            if (ch_temp < 10) ch_temp = ch_temp  +  '0';
            else ch_temp = (ch_temp - 10 )+  'A';
            if(readbuf[readbuf.length()-2]!=ch_temp) return;
            s[0]=myHelper::HexStrValue(readbuf[11],readbuf[12]);
            s[1]=myHelper::HexStrValue(readbuf[9],readbuf[10]);
            s[2]=myHelper::HexStrValue(readbuf[7],readbuf[8]);
            s[3]=myHelper::HexStrValue(readbuf[5],readbuf[6]);
            rtd=*(float *)s;        //瞬时电导率
            #ifdef _TEST
            if(rtd ==0){
                SendZeroCNT++;
            }else if(rtd < 0){
                SendNegativeCNT++;
            }
            SendSuccessCNT++;
            #endif
            flag='N';
            CacheDataProc(rtd,0,flag,Dec,Name,Code,Unit);
        }
    }
    #ifdef _TEST
    qDebug()<<QString("COM%1[SendZeroCNT:%2,SendNegativeCNT:%3,SendSuccessCNT:%4,SendCNT:%5]").arg(port+2).arg(SendZeroCNT).arg(SendNegativeCNT).arg(SendSuccessCNT).arg(SendCNT);    
    #endif
    sleep(2);
#endif
}

//微兰COD
int myAPI::Protocol_4_read(int port,int Address,double *rtd){
#ifdef _DEBUG
    *rtd = 10.0;    
    return 1;
#else
    QByteArray readbuf;
    QByteArray sendbuf;
    int head_flag = 0;
    int check=0;
    int iLoop,len;
    volatile char s[4];
    //状态读取
    //01 03 00 03 00 02 0B 34
    sendbuf.resize(8);
    sendbuf[0]=Address;
    sendbuf[1]=0x03;
    sendbuf[2]=0x00;
    sendbuf[3]=0x02;
    sendbuf[4]=0x00;
    sendbuf[5]=0x13;
    /*sendbuf[1]=0x03;
    sendbuf[2]=0x00;
    sendbuf[3]=0x03;
    sendbuf[4]=0x00;
    sendbuf[5]=0x02;*/
    //01040000000D31CF
    check = myHelper::CRC16_Modbus(sendbuf.data(),6);
    sendbuf[6]=(char)(check>>8);
    sendbuf[7]=(char)(check);

    myCom[port]->readAll();
    qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
    myCom[port]->write(sendbuf);
    myCom[port]->flush();
    sleep(2);
    readbuf=myCom[port]->readAll();
    qDebug()<<QString("COM%1 received:").arg(port+2)<<readbuf.toHex().toUpper();
    len = readbuf.length();
    if(len>=43){
        for(iLoop = 0; iLoop <= len - 43; iLoop++){
            if(Address==readbuf[iLoop] && 0x03==readbuf[iLoop+1] && 38==readbuf[iLoop+2])
            {
                head_flag = 1;
                check = myHelper::CRC16_Modbus(readbuf.data() + iLoop,41);
                //CRC校验
                if((readbuf[iLoop+42]==(char)(check&0xff))&&(readbuf[iLoop+41]==(char)(check>>8)))
                { 
                    //测量结果判断
                    if(0==readbuf[iLoop+3]&&1==readbuf[iLoop+4]){
                        //COD状态判断
                        if(0==readbuf[iLoop+39]&&0==readbuf[iLoop+40]) myApp::COD_Isok=true;

                        s[0]=readbuf[iLoop+8];
                        s[1]=readbuf[iLoop+7];
                        s[2]=readbuf[iLoop+6];
                        s[3]=readbuf[iLoop+5];
                        *rtd=*(float *)s;
                        QDate data(readbuf[iLoop+14]+2000,readbuf[iLoop+16],readbuf[iLoop+18]);
                        QTime time(readbuf[iLoop+20],readbuf[iLoop+22],readbuf[iLoop+24]);
                        myApp::CODSampleTime = data.toString("yyyyMMdd") + time.toString("hhmmss");
                        /*myApp::CODSampleTime.sprintf("%4d%2d%2d%2d%2d%2d",readbuf[iLoop+14],readbuf[iLoop+16],\
                                                                          readbuf[iLoop+18],readbuf[iLoop+20],\
                                                                          readbuf[iLoop+22],readbuf[iLoop+24]);*/
                        qDebug()<<QString("Protocol_4 RTD[%1] CODSampleTime[%2]").arg(*rtd).arg(myApp::CODSampleTime);
                        /*myApp::CODSampleTime = QString("%1-%2-%3 %4:%5:%6").arg(readbuf[iLoop+14]).arg(readbuf[iLoop+16])\
                                                                  .arg(readbuf[iLoop+18]).arg(readbuf[iLoop+20])\
                                                                  .arg(readbuf[iLoop+22]).arg(readbuf[iLoop+24]);*/
                        return 1;
                    }else{
                        qDebug()<<QString("Protocol_4 no data");
                    }
                }else{
                    qDebug()<<QString("COM%1 received check err").arg(port+2);
                }
                break;
            }
        }
        if(0 == head_flag){
            qDebug()<<QString("COM%1 received head not found").arg(port+2);
        }
    }
    return 0;
#endif
}
void myAPI::Protocol_4_control(int port,int Address){
    QByteArray readbuf;
    QByteArray sendbuf;

    sendbuf.resize(11);
    sendbuf[0]=Address;  //发送做样命令
    sendbuf[1]=0x10;
    sendbuf[2]=0x00;
    sendbuf[3]=0x73;
    sendbuf[4]=0x00;
    sendbuf[5]=0x01;
    sendbuf[6]=0x02;
    sendbuf[7]=0x00;
    sendbuf[8]=0x01;
    
    sendbuf[9]=0x93;
    sendbuf[10]=0x6c;
    qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
    myCom[port]->write(sendbuf);
    sleep(1);
    readbuf=myCom[port]->readAll();

}


/*void myAPI::Protocol_4(int port,int Address,int Dec,QString Name,QString Code,QString Unit,double alarm_max)
{
    double rtd=0;
    QString flag="D";
    QByteArray readbuf;
    QByteArray sendbuf;
    int check=0;
    volatile char s[4];
    int interval=0;
    int cod_over_count=0;
    readbuf=myCom[port]->readAll();
    sendbuf[0]=Address;  //读取COD
    sendbuf[1]=0x03;
    sendbuf[2]=0x00;
    sendbuf[3]=0x00;
    sendbuf[4]=0x00;
    sendbuf[5]=0x02;
    check = myHelper::CRC16_Modbus(sendbuf.data(),6);
    sendbuf[6]=(char)(check);
    sendbuf[7]=(char)(check>>8);
    myCom[port]->write(sendbuf);
    qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
    sleep(2);
    readbuf=myCom[port]->readAll();
    qDebug()<<QString("COM%1 received:").arg(port+2)<<readbuf.toHex().toUpper();
    if(readbuf.length()>=17){
        if(Address==readbuf[0])
        {
            check = myHelper::CRC16_Modbus(readbuf.data(),15);
            if((readbuf[15]==(char)(check&0xff))&&(readbuf[16]==(char)(check>>8)))
            {
                s[0]=readbuf[4];
                s[1]=readbuf[3];
                s[2]=readbuf[6];
                s[3]=readbuf[5];
                rtd=*(float *)s;        //瞬时COD
                flag='N';
                qDebug()<<QString("COM%1 received:").arg(port+2);
                CacheDataProc(rtd,0,flag,Dec,Name,Code,Unit);
            }
        }
    }

    if(myApp::Pro_Rain>=2&&myApp::Pro_Rain<6)   //
    {
        sendbuf[0]=Address;  //发送做样命令
        sendbuf[1]=0x06;
        sendbuf[2]=0x00;
        sendbuf[3]=0x33;
        sendbuf[4]=0x00;
        sendbuf[5]=0x01;
        check = myHelper::CRC16_Modbus(sendbuf.data(),6);
        sendbuf[6]=(char)(check);
        sendbuf[7]=(char)(check>>8);
        qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
        myCom[port]->write(sendbuf);
        sleep(1);
        readbuf=myCom[port]->readAll();
        sleep(30);

        sendbuf[0]=Address;  //读取COD
        sendbuf[1]=0x03;
        sendbuf[2]=0x00;
        sendbuf[3]=0x00;
        sendbuf[4]=0x00;
        sendbuf[5]=0x02;
        check = myHelper::CRC16_Modbus(sendbuf.data(),6);
        sendbuf[6]=(char)(check);
        sendbuf[7]=(char)(check>>8);
        qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
        myCom[port]->write(sendbuf);
        sleep(2);
        readbuf=myCom[port]->readAll();
        qDebug()<<QString("COM%1 received:").arg(port+2)<<readbuf.toHex().toUpper();
        if(readbuf.length()>=9){
            if(Address==readbuf[0])
            {
                check = myHelper::CRC16_Modbus(readbuf.data(),7);
                if((readbuf[7]==(char)(check&0xff))&&(readbuf[8]==(char)(check>>8)))
                {
                    s[0]=readbuf[4];
                    s[1]=readbuf[3];
                    s[2]=readbuf[6];
                    s[3]=readbuf[5];
                    rtd=*(float *)s;        //瞬时COD
                    flag='N';
                    CacheDataProc(rtd,0,flag,Dec,Name,Code,Unit);
                    if(rtd>alarm_max)   //超标
                    {
                        interval=1;
                        cod_over_count=1;
                    }
                    //TODO //连续降雨合格开排水阀
                    else                //合格
                    {
                            myApp *rain_pro=new myApp;
                            rain_pro->PronumberChange(3);   //恢复合格
                            delete rain_pro;
                            interval=myApp::CODinterval*60;
                    }
                }
            }
        }
        sleep(interval);
    }
    else  //非降雨周期
    {
        myApp::cod_overproof=0;
    }

    myApp Cod_Pro;
    Cod_Pro.CodOverproofChange(cod_over_count);

    if(myApp::cod_overproof>=3)   //超标累计大于3次
    {
        myApp::cod_overproof=3;
        QString str_tmp;
        str_tmp=QString("PolID=w01018,Value=%1").arg(rtd);
        emit ProtocolOver(2,2,1,1,str_tmp);   //发送留样关阀信号
        myApp *rain_pro=new myApp;
        rain_pro->PronumberChange(7);   //等待降雨
        delete rain_pro;

    }
//远程&本地操控做样
    if(24 != myApp::In_level && GetSwitchStatus(myApp::In_level)==true){
        return;
    }
    if(myApp::COD_Flag){
        sendbuf[0]=Address;  //发送做样命令
        sendbuf[1]=0x06;
        sendbuf[2]=0x00;
        sendbuf[3]=0x33;
        sendbuf[4]=0x00;
        sendbuf[5]=0x01;
        check = myHelper::CRC16_Modbus(sendbuf.data(),6);
        sendbuf[6]=(char)(check);
        sendbuf[7]=(char)(check>>8);
        qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
        myCom[port]->write(sendbuf);
        sleep(1);
        readbuf=myCom[port]->readAll();
        if(0x33==readbuf[3]) myApp::COD_Flag=0;
    }
}*/
//微兰COD
void myAPI::Protocol_4(int port,int Address,int Dec,QString Name,QString Code,QString Unit,double alarm_max)
{
    double rtd=0;
    QString flag="D";
    int interval=0;
    int cod_over_count=0;
    
    int iLoop;
//状态读取
    if(1 == Protocol_4_read(port, Address, &rtd)){
        flag='N';
    }else{
        flag='D';
    }
    CacheDataProc(rtd,0,flag,Dec,Name,Code,Unit);


    qDebug()<<QString("COD In_level[%1]").arg(myApp::In_level);
    if(24 != myApp::In_level && GetSwitchStatus(myApp::In_level)==false){
        return;
    }
    myApp Cod_Pro;
    if(myApp::Pro_Rain>=2&&myApp::Pro_Rain<6)   //
    {
        //发送做样命令
        Protocol_4_control(port,Address);
        myApp::COD_Isok = false;
        sleep(30);
        flag='D';
        for(iLoop=0;iLoop<30;iLoop++){
            if(1 == Protocol_4_read(port, Address, &rtd)){
                flag='N';
                if(rtd>alarm_max)   //超标
                {
                    interval=1;
                    cod_over_count=1;
                }
                //TODO //连续降雨合格开排水阀
                else                //合格
                {
                        Cod_Pro.CodOverproofChange(0);
                        myApp *rain_pro=new myApp;
                        rain_pro->PronumberChange(3);   //恢复合格
                        delete rain_pro;
                        interval=myApp::CODinterval*60;
                }
                break;
            }
            sleep(2);
        }
        CacheDataProc(rtd,0,flag,Dec,Name,Code,Unit);   
        sleep(interval);
    }else{  //非降雨周期
        Cod_Pro.CodOverproofChange(0);
    }

    Cod_Pro.CodOverproofPlus(cod_over_count);

    if(myApp::cod_overproof>=3)   //超标累计大于3次
    {
        Cod_Pro.CodOverproofChange(3);
        QString str_tmp;
        str_tmp=QString("PolID=w01018,Value=%1").arg(rtd);
        emit ProtocolOver(2,2,1,1,str_tmp);   //发送留样关阀信号
        myApp *rain_pro=new myApp;
        rain_pro->PronumberChange(7);   //等待降雨
        delete rain_pro;

    }
    //添加COD状态检测 空闲状态         myApp::COD_Isok=true;
    qDebug()<<QString("COD COD_Flag[%1] COD_Isok[%2]").arg(myApp::COD_Flag).arg(myApp::COD_Isok);
    if(myApp::COD_Flag&&myApp::COD_Isok==true){
        qDebug()<<QString("COD start");
        myApp::COD_Flag=0;
        myApp::COD_Isok=false;
        //添加COD取样指令
        Protocol_4_control(port,Address);
    }

}



//雨水刷卡设备
void myAPI::Protocol_5(int port)
{
    QByteArray readbuf;
    QByteArray sendbuf;
    QByteArray ch;
    ch.resize(4);
    int len=0;
    int crc=0;
    //查询刷卡记录
    readbuf=myCom[port]->readAll();
    sendbuf="##FC=5;CRC=7340\r\n";
    myCom[port]->write(sendbuf);
    myCom[port]->flush();
    qDebug()<<sendbuf;
    sleep(2);
    readbuf=myCom[port]->readAll();
    qDebug()<<QString("##FC=5:")<<readbuf;
    if(readbuf.data()!=NULL){
        QRegExp Ex;
        QString cardtype;
        QString cardno;
        Ex.setPattern("CardType=([0-9]+)");
        if(Ex.indexIn(readbuf) != -1){
            cardtype=Ex.cap(1);
            Ex.setPattern("CardNO=([0-9A-Za-z]+)");
            if(Ex.indexIn(readbuf) != -1){
                cardno=Ex.cap(1);
                CacheCard(cardtype,cardno);
                QString str_tmp=QString("DataTime=%1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"));
                QString dt=QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00");
                str_tmp="DataTime="+QString("%1%2%3%4%5%6;")
                            .arg(dt.mid(0,4))
                            .arg(dt.mid(5,2))
                            .arg(dt.mid(8,2))
                            .arg(dt.mid(11,2))
                            .arg(dt.mid(14,2))
                            .arg(dt.mid(17,2));
                if(cardtype=="1"){
                    cardtype="maintain";
                }
                else if(cardtype=="2"){
                    cardtype="admin";
                }
                else{
                    cardtype="other";
                }
                str_tmp+=QString("CardNo=%1;CardType=%2;Gate=ON").arg(cardno).arg(cardtype);
                Insert_Message_VSErr(3097,5,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:00"),str_tmp);
//                myApp::Cardrecord_FLG=1;
            }
        }
    }
 //查询门禁状态
    sendbuf="##FC=4;CRC=7640\r\n";
    myCom[port]->write(sendbuf);
    myCom[port]->flush();
    qDebug()<<sendbuf;
    sleep(2);
    readbuf=myCom[port]->readAll();
    qDebug()<<QString("##FC=4:")<<readbuf;
    if(readbuf.data()!=NULL){
        QRegExp Ex;

        Ex.setPattern("Gate=([A-Z]+)");
        if(Ex.indexIn(readbuf) != -1){
         myApp::DoorStatus=Ex.cap(1);
        }
    }
    else{
        myApp::DoorStatus="--";
    }

    if(myApp::Door_FLG){
        //发送开门协议
        readbuf=myCom[port]->readAll();
        sendbuf="##FC=6;Gate=ON;CRC=8581\r\n";
        myCom[port]->write(sendbuf);
        myCom[port]->flush();
        qDebug()<<sendbuf;
        sleep(2);
        readbuf=myCom[port]->readAll();
        qDebug()<<QString("##FC=6:")<<readbuf;
        if(readbuf.contains("FC=6;ExeRtn=1;")){
            //执行成功；
            myApp::Door_FLG=0;
        }
        else{
            //执行失败
            myApp::Door_FLG=1;
        }
    }

    //开卡
    if(myApp::Addcard_FLG){
        QString sql;
        QSqlQuery query,query1;
        QString sendms;
        QString CardNo;
        QString CardType;

        sql="select [CardNo], [CardType],[SendCount] from CardNumber where [SendCount]='0'";
        query.exec(sql);

        while(query.next()){
         CardNo=query.value(0).toString();
         CardType=query.value(1).toString();
        myCom[port]->readAll();
        sendms=QString("##FC=1;CardType=%1;CardNO=%2;CRC=").arg(CardType).arg(CardNo);
        len=sendms.size();
        crc=myHelper::CRC16_GB212(&sendms.toLatin1().data()[2],len-2);
        sprintf(ch.data(),"%04X",crc);
        sendms.append(ch+"\r\n");
        myCom[port]->write(sendms.toLatin1());
        myCom[port]->flush();
        qDebug()<<sendms.toLatin1();
        sleep(2);
        readbuf=myCom[port]->readAll();
        qDebug()<<QString("##FC=1:")<<readbuf;
        if(readbuf.contains("FC=1;ExeRtn=1;")){
            //执行成功；
            myApp::Addcard_FLG=0;
            sql = QString("update CardNumber set [SendCount]='1'where [CardNo]='%1'").arg(CardNo);
            //qDebug()<<QString("update CardNumber set [SendCount]='1'where [CardNo]='%1'").arg(CardNo);
            query1.exec(sql);//更新处理标记
        }
        else{
            //执行失败
            myApp::Addcard_FLG=1;
        }
        }
    }
    //销卡
    if(myApp::Deletecard_FLG){
        QString sql2;
        QSqlQuery query2,query3;
        QString sendms2;
        QString CardNo2;
        QString CardType2;
        sql2=QString("select [CardNo], [CardType],[SendCount] from CardNumber where [Attribute]='delete'");
        query2.exec(sql2);
        while(query2.next()){
            CardNo2=query2.value(0).toString();
            CardType2=query2.value(1).toString();
            myCom[port]->readAll();
            sendms2=QString("##FC=2;CardType=%1;CardNO=%2;CRC=").arg(CardType2).arg(CardNo2);
            len=sendms2.size();
            crc=myHelper::CRC16_GB212(&sendms2.toLatin1().data()[2],len-2);
            sprintf(ch.data(),"%04X",crc);
            sendms2.append(ch+"\r\n");
            myCom[port]->write(sendms2.toLatin1());
            myCom[port]->flush();
            sleep(2);
            readbuf=myCom[port]->readAll();
            if(readbuf.contains("FC=2;ExeRtn=1;")){
                //执行成功；
                sql2=QString("delete from [CardNumber] where [Attribute]='delete' and [CardNo]='%1'").arg(CardNo2);
                qDebug()<<sql2;
                query3.exec(sql2);
                myApp::Deletecard_FLG=0;
            }
            else{
                //执行失败
                myApp::Deletecard_FLG=1;

            }
        }
    }
}




//雨水采样仪
void myAPI::Protocol_6(int port)
{
    QString sendbuf;
     QString readbuf;
     sendbuf="DR@01HB";
     myCom[port]->readAll();
     myCom[port]->write(sendbuf.toLatin1());
     myCom[port]->flush();
     qDebug()<<sendbuf;
     myHelper::Sleep(2000);
     readbuf=myCom[port]->readAll();
     if(readbuf.length()>=99){
         myApp::BottleId=readbuf.mid(27,2).toInt();
         qDebug()<<myApp::BottleId;
    }

    if(myApp::Sample_Flag){
        qDebug()<<QString("sample_flg=%1").arg(myApp::Sample_Flag);
        sendbuf="DR@24HB";
        myCom[0]->readAll();
        myCom[0]->write(sendbuf.toLatin1());
        myCom[0]->flush();
        qDebug()<<sendbuf;
        myHelper::Sleep(1000);
        readbuf=myCom[0]->readAll();
        qDebug()<<readbuf;
        if(readbuf=="A1DR@24OKHB"){
            sendbuf="DR@26HB";
            myCom[0]->write(sendbuf.toLatin1());
            myCom[0]->flush();
            myHelper::Sleep(1000);
            readbuf=myCom[0]->readAll();
            qDebug()<<readbuf;
            if(readbuf=="A1DR@26OKHB")
            {
                myApp::Sample_Flag=0;
            }

        }

    }
    sleep(2);
}
//南控液位计
void myAPI::Protocol_7(int port,int Address,int Dec,QString Name,QString Code,QString Unit,double alarm_min,double alarm_max)
{
    #ifdef _TEST
        static int SendCNT = 0;
        static int SendSuccessCNT = 0;
        static int SendZeroCNT = 0;
        static int SendNegativeCNT = 0;
    #endif
    double rtd=0;
    QString flag="D";
    QByteArray readbuf;
    QByteArray sendbuf;
    int check=0;
    volatile char s[4];
    sendbuf.resize(8);
    int timecount=0;
    sendbuf[0]=Address;  //读取COD
    sendbuf[1]=0x03;
    sendbuf[2]=0x00;
    sendbuf[3]=0x00;
    sendbuf[4]=0x00;
    sendbuf[5]=0x02;
    check = myHelper::CRC16_Modbus(sendbuf.data(),6);
    sendbuf[6]=(char)(check);
    sendbuf[7]=(char)(check>>8);
    do{
    myCom[port]->write(sendbuf);
    #ifdef _TEST
        SendCNT++;
    #endif
    myCom[port]->flush();
    //qDebug()<<QString("COM%1 send[%2]:").arg(port+2).arg(timecount)<<sendbuf.toHex().toUpper();
    sleep(2);
    readbuf=myCom[port]->readAll();
    qDebug()<<QString("COM%1 received:").arg(port+2)<<readbuf.toHex().toUpper();
    if(readbuf.length()>=9){
        if(Address==readbuf[0])
        {
                check = myHelper::CRC16_Modbus(readbuf.data(),7);
                if((readbuf[7]==(char)(check&0xff))&&(readbuf[8]==(char)(check>>8)))
                {
                    s[0]=readbuf[6];
                    s[1]=readbuf[5];
                    s[2]=readbuf[4];
                    s[3]=readbuf[3];
                    rtd=*(float *)s;
                    #ifdef _TEST
                        if(rtd ==0){
                            SendZeroCNT++;
                        }else if(rtd < 0){
                            SendNegativeCNT++;
                        }
                        SendSuccessCNT++;
                    #endif
                    flag='N';
                    CacheDataProc(rtd,0,flag,Dec,Name,Code,Unit);
                    if(rtd>alarm_min)
                    {
                        //回流泵常闭
                       Reflux_Valve_Open_Set();

                    }
                    else
                    {
                        //回流泵常开
                        Reflux_Valve_Close_Set();
                    }
                    if(rtd>=alarm_max)myApp::LVStatus="High";
                    if(rtd<alarm_max&&rtd>alarm_min)myApp::LVStatus="Normal";
                    if(rtd<=alarm_min)myApp::LVStatus="Low";
                }
        }
    }
    timecount++;
    }while(timecount<5&&flag=="D");
    #ifdef _TEST
    qDebug()<<QString("COM%1[SendZeroCNT:%2,SendNegativeCNT:%3,SendSuccessCNT:%4,SendCNT:%5]").arg(port+2).arg(SendZeroCNT).arg(SendNegativeCNT).arg(SendSuccessCNT).arg(SendCNT);    
    #endif
    if(flag=="D")myApp::LVStatus="Error";

}

//CE9628JM流量计
void myAPI::Protocol_8(int port,int Dec,QString Name,QString Code,QString Unit)
{
#ifdef _DEBUG
    double rtd=0;
    static double total=0;
    rtd = 10.0;    
    total += 0.01;
    CacheDataProc(rtd,total,"N",Dec,Name,Code,Unit);
#else
    double rtd=0;
    double total;
    int check=0;
    QString flag="D";
    QByteArray readbuf;
    QByteArray sendbuf;
    sendbuf.resize(1);
    sendbuf[0]=0x4E;  //读取流量计
    myCom[port]->write(sendbuf);
    sleep(2);
    readbuf=myCom[port]->readAll();
    if(readbuf.length()>=25&&readbuf.startsWith(0x02)&&readbuf.endsWith(0x03)){
        check=myHelper::SUM_8bit(&readbuf.data()[1],readbuf.length()-3);
        if(check==readbuf[23]){
            total=myHelper::HexValue(readbuf[1]);
            total=total*16+myHelper::HexValue(readbuf[2]);
            total=total*16+myHelper::HexValue(readbuf[3]);
            total=total*16+myHelper::HexValue(readbuf[4]);
            total=total*16+myHelper::HexValue(readbuf[5]);
            total=total*16+myHelper::HexValue(readbuf[6]);
            total=total*16+myHelper::HexValue(readbuf[7]);
            total=total*16+myHelper::HexValue(readbuf[8]);

            rtd=myHelper::HexValue(readbuf[9]);
            rtd=rtd*16+myHelper::HexValue(readbuf[10]);
            rtd=rtd*16+myHelper::HexValue(readbuf[11]);
            rtd=rtd*16+myHelper::HexValue(readbuf[12]);
            rtd=rtd/100;
            flag="N";
            CacheDataProc(rtd,total,flag,Dec,Name,Code,Unit);
        }
    }
#endif
}

//PH-P206
void myAPI::Protocol_9(int port,int Address,int Dec,QString Name,QString Code,QString Unit)
{
#ifdef _DEBUG
        double rtd=0;
        rtd = 10.0;    
        CacheDataProc(rtd,0,"N",Dec,Name,Code,Unit);
#else
    #ifdef _TEST
            static int SendCNT = 0;
            static int SendSuccessCNT = 0;
            static int SendZeroCNT = 0;
            static int SendNegativeCNT = 0;
    #endif
    double rtd=0;
    QString flag="D";
    QByteArray readbuf;
    QByteArray sendbuf;
    int check=0;
    volatile char s[4];
    int timecount=0;
    sendbuf.resize(8);
    sendbuf[0]=Address;
    sendbuf[1]=0x04;
    sendbuf[2]=0x00;
    sendbuf[3]=0x02;
    sendbuf[4]=0x00;
    sendbuf[5]=0x02;
    check = myHelper::CRC16_Modbus(sendbuf.data(),6);
    sendbuf[6]=(char)(check);
    sendbuf[7]=(char)(check>>8);

   do{
    myCom[port]->write(sendbuf);
    myCom[port]->flush();
    #ifdef _TEST
        SendCNT++;
    #endif
    //qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
    sleep(2);
    readbuf=myCom[port]->readAll();
    qDebug()<<QString("COM%1 received:").arg(port+2)<<readbuf.toHex().toUpper();
    if(readbuf.length()>=9){
        if(Address==readbuf[0])
        {
                check = myHelper::CRC16_Modbus(readbuf.data(),7);
                if((readbuf[7]==(char)(check&0xff))&&(readbuf[8]==(char)(check>>8)))
                {
                    s[0]=readbuf[4];
                    s[1]=readbuf[3];
                    s[2]=readbuf[6];
                    s[3]=readbuf[5];
                    rtd=*(float *)s;        //瞬时PH
                    #ifdef _TEST
                        if(rtd ==0){
                            SendZeroCNT++;
                        }else if(rtd < 0){
                            SendNegativeCNT++;
                        }
                        SendSuccessCNT++;
                    #endif
                    flag='N';
                    CacheDataProc(rtd,0,flag,Dec,Name,Code,Unit);
                }
        }
    }
    timecount++;
    }while (timecount<5&&flag=="D") ;
    #ifdef _TEST
        qDebug()<<QString("COM%1[SendZeroCNT:%2,SendNegativeCNT:%3,SendSuccessCNT:%4,SendCNT:%5]").arg(port+2).arg(SendZeroCNT).arg(SendNegativeCNT).arg(SendSuccessCNT).arg(SendCNT);    
    #endif
#endif
}

//天泽COD
void myAPI::Protocol_10(int port,int Address,int Dec,QString Name,QString Code,QString Unit,int COD_or_NH3)
{
    double rtd=0;
    QString flag="D";
    QByteArray readbuf;
    QByteArray sendbuf;
    int check=0;
    volatile char s[4];
//状态读取
    sendbuf.resize(8);
    sendbuf[0]=Address;
    sendbuf[1]=0x03;
    sendbuf[2]=0x03;
    sendbuf[3]=0xBC;
    sendbuf[4]=0x00;
    sendbuf[5]=0x01;
    check = myHelper::CRC16_Modbus(sendbuf.data(),6);
    sendbuf[6]=(char)(check);
    sendbuf[7]=(char)(check>>8);
    myCom[port]->readAll();
    qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
    myCom[port]->write(sendbuf);
    myCom[port]->flush();
    sleep(2);
    readbuf=myCom[port]->readAll();
    qDebug()<<QString("COM%1 received:").arg(port+2)<<readbuf.toHex().toUpper();
    if(readbuf.length()>=7){
        if(Address==readbuf[0])
        {
                check = myHelper::CRC16_Modbus(readbuf.data(),readbuf.length()-2);
                if((readbuf[readbuf.length()-2]==(char)(check&0xff))&&(readbuf[readbuf.length()-1]==(char)(check>>8)))
                {
                    if(0==readbuf[4]&&0==readbuf[3]){
                        if(0 == COD_or_NH3){
                            myApp::COD_Isok=true;
                        }else{
                            myApp::NH3_Isok=true;
                        }
                    }
                }
        }
    }
//瞬时数据读取
    sendbuf[0]=Address;
    sendbuf[1]=0x03;
    sendbuf[2]=0x00;
    sendbuf[3]=0xEE;
    sendbuf[4]=0x00;
    sendbuf[5]=0x02;
    check = myHelper::CRC16_Modbus(sendbuf.data(),6);
    sendbuf[6]=(char)(check);
    sendbuf[7]=(char)(check>>8);
    myCom[port]->readAll();
    qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
    myCom[port]->write(sendbuf);
    myCom[port]->flush();
    sleep(1);
    readbuf=myCom[port]->readAll();
    qDebug()<<QString("COM%1 received:").arg(port+2)<<readbuf.toHex().toUpper();
    if(readbuf.length()>=9){
        if(Address==readbuf[0])
        {
                check = myHelper::CRC16_Modbus(readbuf.data(),readbuf.length()-2);
                if((readbuf[readbuf.length()-2]==(char)(check&0xff))&&(readbuf[readbuf.length()-1]==(char)(check>>8)))
                {
                    s[0]=readbuf[6];
                    s[1]=readbuf[5];
                    s[2]=readbuf[4];
                    s[3]=readbuf[3];
                    rtd=*(float *)s;
                    flag='N';
                }
        }
    }
    CacheDataProc(rtd,0,flag,Dec,Name,Code,Unit);
    if(24 != myApp::In_level && GetSwitchStatus(myApp::In_level)==false){
        return;
    }
    //添加COD/NH3状态检测 空闲状态         myApp::COD_Isok=true;
    
    if(0 == COD_or_NH3){
        qDebug()<<QString("COD Flag[%1] Isok[%2]").arg(myApp::COD_Flag).arg(myApp::COD_Isok);
        if(0 == myApp::COD_Flag || myApp::COD_Isok==false){
            return;
        }
        myApp::COD_Flag=0;
        myApp::COD_Isok=false;
    }else{
        qDebug()<<QString("NH3 Flag[%1] Isok[%2]").arg(myApp::NH3_Flag).arg(myApp::NH3_Isok);
        if(0 == myApp::NH3_Flag || myApp::NH3_Isok==false){
            return;
        }
        myApp::NH3_Flag=0;
        myApp::NH3_Isok=false;

    }
    qDebug()<<QString("COD_or_NH3 start");
    //添加COD/NH3取样指令
    sendbuf.resize(11);
    sendbuf[0]=Address;
    sendbuf[1]=0x10;
    sendbuf[2]=0x06;
    sendbuf[3]=0x72;
    sendbuf[4]=0x00;
    sendbuf[5]=0x01;
    sendbuf[6]=0x02;
    sendbuf[7]=0x00;
    sendbuf[8]=0x01;
    check = myHelper::CRC16_Modbus(sendbuf.data(),9);
    sendbuf[9]=(char)(check);
    sendbuf[10]=(char)(check>>8);
    myCom[port]->readAll();
    qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
    myCom[port]->write(sendbuf);
    myCom[port]->flush();
    sleep(2);
    readbuf=myCom[port]->readAll();
    qDebug()<<QString("COM%1 received:").arg(port+2)<<readbuf.toHex().toUpper();

}

//承德流量计
void myAPI::Protocol_11(int port,int Address,int Dec,QString Name,QString Code,QString Unit)
{
    double rtd=0;
    double total=0;
    QString flag="D";
    QByteArray readbuf;
    QByteArray sendbuf;
    uchar check=0;
    char s[4];

    sendbuf.resize(14);
    sendbuf[0]=0x68;
    sendbuf[1]=(char)(Address);
    sendbuf[2]=(char)(Address>>8);
    sendbuf[3]=(char)(Address>>16);
    sendbuf[4]=(char)(Address>>24);
    sendbuf[5]=0x00;
    sendbuf[6]=0x00;
    sendbuf[7]=0x68;
    sendbuf[8]=0x01;
    sendbuf[9]=0x02;
    sendbuf[10]=0x18;
    sendbuf[11]=0xC0;
    check=0;
    for(int j=0;j<12;j++)
    {
        check+=sendbuf.data()[j];
    }
    sendbuf[12]=(uchar)check;
    sendbuf[13]=0x16;
    myCom[port]->write(sendbuf);
    myCom[port]->flush();
    sleep(4);
    readbuf=myCom[port]->readAll();
    qDebug()<<QString("COM%1 received_rtd:").arg(port+2)<<readbuf.toHex();
    if(readbuf.length()==18){
        if(readbuf.endsWith(0x16)&&readbuf.startsWith(0x68)){
            check=0;
            for(int j=0;j<16;j++)
            {
                check+=readbuf.data()[j];
            }
            check=(uchar)check;
            if(check==readbuf[16]){
                s[0]=readbuf[12];
                s[1]=readbuf[13];
                s[2]=readbuf[14];
                s[3]=readbuf[15];
                rtd=*(float *)s;
                flag="N";
            }

        }
    }

    sendbuf[0]=0x68;
    sendbuf[1]=(char)(Address);
    sendbuf[2]=(char)(Address>>8);
    sendbuf[3]=(char)(Address>>16);
    sendbuf[4]=(char)(Address>>24);
    sendbuf[5]=0x00;
    sendbuf[6]=0x00;
    sendbuf[7]=0x68;
    sendbuf[8]=0x01;
    sendbuf[9]=0x02;
    sendbuf[10]=0x03;
    sendbuf[11]=0xC0;
    check=0;
    for(int j=0;j<12;j++)
    {
        check+=sendbuf.data()[j];
    }
    sendbuf[12]=(uchar)check;
    sendbuf[13]=0x16;
    myCom[port]->write(sendbuf);//读取累积流量
    myCom[port]->flush();
    sleep(4);
     readbuf = myCom[port]->readAll();
     qDebug()<<QString("COM%1 received_tl:").arg(port+2)<<readbuf.toHex();
    if(readbuf.length()==22){
        if(readbuf.endsWith(0x16)&&readbuf.startsWith(0x68))
        {
            check=0;
            for(int j=0;j<20;j++)
            {
                check+=readbuf.data()[j];
            }
            check=(uchar)check;
            if(check==readbuf[20]){
                total=  myHelper::to_natural_binary(readbuf[19])*100000000000LL+
                            myHelper::to_natural_binary(readbuf[18])*1000000000+
                            myHelper::to_natural_binary(readbuf[17])*10000000+
                            myHelper::to_natural_binary(readbuf[16])*100000+
                            myHelper::to_natural_binary(readbuf[15])*1000+
                            myHelper::to_natural_binary(readbuf[14])*10+
                            myHelper::to_natural_binary(readbuf[13])*0.1+
                            myHelper::to_natural_binary(readbuf[12])*0.001;
                if(flag=="N"&&total<=50000000)CacheDataProc(rtd,total,flag,Dec,Name,Code,Unit);
                else flag="D";
            }
        }
    }
}

//哈希COD
void myAPI::Protocol_12(int port,int Address,int Dec,QString Name,QString Code,QString Unit, int COD_or_NH3)
{
    float rtd=0;
    double total=0;
    QString flag="D";
    QByteArray readbuf;
    QByteArray sendbuf;
    int check=0;
    char s[4];
    
    //状态读取
    sendbuf.resize(8);
    sendbuf[0]=Address;
    sendbuf[1]=0x03;
    sendbuf[2]=0x00;
    sendbuf[3]=0x34;
    sendbuf[4]=0x00;
    sendbuf[5]=0x01;
    check = myHelper::CRC16_Modbus(sendbuf.data(),6);
    sendbuf[6]=(char)(check);
    sendbuf[7]=(char)(check>>8);
    myCom[port]->readAll();
    qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
    myCom[port]->write(sendbuf);
    myCom[port]->flush();
    sleep(2);
    readbuf=myCom[port]->readAll();
    qDebug()<<QString("COM%1 received:").arg(port+2)<<readbuf.toHex().toUpper();
    if(readbuf.length()>=7){
        if(Address==readbuf[0])
        {
                check = myHelper::CRC16_Modbus(readbuf.data(),readbuf.length()-2);
                if((readbuf[readbuf.length()-2]==(char)(check&0xff))&&(readbuf[readbuf.length()-1]==(char)(check>>8)))
                {
                    if(0x01==(readbuf[3] & 0x01)){
                        if(0 == COD_or_NH3){
                            myApp::COD_Isok=true;
                        }else{
                            myApp::NH3_Isok=true;
                        }
                    }
                }
        }
    }
    //读瞬时值
    sendbuf.resize(8);
    sendbuf[0]=Address;
    sendbuf[1]=0x03;
    sendbuf[2]=0x00;
    sendbuf[3]=0x00;
    sendbuf[4]=0x00;
    sendbuf[5]=0x06;
    check = myHelper::CRC16_Modbus(sendbuf.data(),6);
    sendbuf[6]=(char)(check);
    sendbuf[7]=(char)(check>>8);
    qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
    myCom[port]->write(sendbuf);
    myCom[port]->flush();
    sleep(2);
    readbuf=myCom[port]->readAll();
    qDebug()<<QString("COM%1 received:").arg(port+2)<<readbuf.toHex().toUpper();
    if(readbuf.length()>=17){
        if(Address==readbuf[0])
        {
            check = myHelper::CRC16_Modbus(readbuf.data(),readbuf.length()-2);
            if((readbuf[readbuf.length()-2]==(char)(check&0xff))&&(readbuf[readbuf.length()-1]==(char)(check>>8)))
            {
                s[0]=readbuf[4];
                s[1]=readbuf[3];
                s[2]=readbuf[6];
                s[3]=readbuf[5];
                rtd=*(float *)s;        //COD
                flag='N';
                CacheDataProc(rtd,total,flag,Dec,Name,Code,Unit);
            }
        }
    }
     if(24 != myApp::In_level && GetSwitchStatus(myApp::In_level)==true){
        return;
    }
    //添加COD/NH3状态检测 空闲状态         myApp::COD_Isok=true;
    if(0 == COD_or_NH3){
        if(0 == myApp::COD_Flag || myApp::COD_Isok==false){
            return;
        }
        myApp::COD_Flag=0;
        myApp::COD_Isok=false;
    }else{
        if(0 == myApp::NH3_Flag || myApp::NH3_Isok==false){
            return;
        }
        myApp::NH3_Flag=0;
        myApp::NH3_Isok=false;
    }
    //添加COD/NH3取样指令
    sendbuf.resize(8);
    sendbuf[0]=Address;
    sendbuf[1]=0x06;
    sendbuf[2]=0x00;
    sendbuf[3]=0x33;   
    sendbuf[4]=0x00;
    sendbuf[5]=0x01;
    check = myHelper::CRC16_Modbus(sendbuf.data(),6);
    sendbuf[6]=(char)(check);
    sendbuf[7]=(char)(check>>8);
    myCom[port]->readAll();
    qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
    myCom[port]->write(sendbuf);
    myCom[port]->flush();
}


//哈希氨氮

void myAPI::Protocol_13(int port,int Address,int Dec,QString Name,QString Code,QString Unit)
{
    float rtd=0;
    double total=0;
    QString flag="D";
    QByteArray readbuf;
    QByteArray sendbuf;
    int check=0;
    char s[4];

    sendbuf.resize(8);
    sendbuf[0]=Address;
    sendbuf[1]=0x03;
    sendbuf[2]=0x00;
    sendbuf[3]=0x00;
    sendbuf[4]=0x00;
    sendbuf[5]=0x02;
    check = myHelper::CRC16_Modbus(sendbuf.data(),6);
    sendbuf[6]=(char)(check);
    sendbuf[7]=(char)(check>>8);
    qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
    myCom[port]->write(sendbuf);
    sleep(2);
    readbuf=myCom[port]->readAll();
    qDebug()<<QString("COM%1 received:").arg(port+2)<<readbuf.toHex().toUpper();

    if(readbuf.length()>=9){
        if(Address==readbuf[0])
        {
                check = myHelper::CRC16_Modbus(readbuf.data(),readbuf.length()-2);
                if((readbuf[readbuf.length()-2]==(char)(check&0xff))&&(readbuf[readbuf.length()-1]==(char)(check>>8)))
                {
                    s[0]=readbuf[4];
                    s[1]=readbuf[3];
                    s[2]=readbuf[6];
                    s[3]=readbuf[5];
                    rtd=*(float *)s;        //NH3-N
                    flag='N';
                    CacheDataProc(rtd,total,flag,Dec,Name,Code,Unit);
                }
        }
    }
}
//TOC-4200
void myAPI::Protocol_14_Rtu(int port,int Address,int Dec,QString Name,QString Code,QString Unit,int path)
{
    double rtd=0;
    QString flag="D";
    QByteArray readbuf;
    QByteArray sendbuf;
    int head_flag = 0;
    int check=0;
    unsigned short int Register = 0;
    int iLoop,len;
    volatile char s[4];
//状态读取
    Register = path * 20; 
    sendbuf.resize(8);
    sendbuf[0]=Address;
    sendbuf[1]=0x04;
    sendbuf[2]=(char)(Register>>8);
    sendbuf[3]=(char)(Register & 0xff);
    sendbuf[4]=0x00;
    sendbuf[5]=0x0d;
    //01040000000D31CF
    check = myHelper::CRC16_Modbus(sendbuf.data(),6);
    sendbuf[6]=(char)(check);
    sendbuf[7]=(char)(check>>8);
    myCom[port]->readAll();
    qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
    myCom[port]->write(sendbuf);
    myCom[port]->flush();
    sleep(2);
    readbuf=myCom[port]->readAll();
    qDebug()<<QString("COM%1 received:").arg(port+2)<<readbuf.toHex().toUpper();
    len = readbuf.length();
    if(len>=31){
        for(iLoop = 0; iLoop <= len - 31; iLoop++){
            if(Address==readbuf[iLoop] && 0x04==readbuf[iLoop+1] && 26==readbuf[iLoop+2])
            {
                head_flag = 1;
                check = myHelper::CRC16_Modbus(readbuf.data() + iLoop,29);
                //CRC校验
                if((readbuf[iLoop+29]==(char)(check&0xff))&&(readbuf[iLoop+30]==(char)(check>>8)))
                { 
                    //测量结果判断
                    if(0==readbuf[iLoop+3]&&1==readbuf[iLoop+4]){
                        //TOC状态判断
                        if(0==readbuf[iLoop+5]&&0==readbuf[iLoop+6]) myApp::TOC_Isok=true;

                        s[0]=readbuf[iLoop+22];
                        s[1]=readbuf[iLoop+21];
                        s[2]=readbuf[iLoop+20];
                        s[3]=readbuf[iLoop+19];
                        rtd=*(float *)s;
                        flag='N';
                        qDebug()<<QString("Protocol_14_Rtu RTD[%1]").arg(rtd);
                    }else{
                        qDebug()<<QString("Protocol_14_Rtu no data");
                    }
                }else{
                    qDebug()<<QString("COM%1 received check err").arg(port+2);
                }
                break;
            }
        }
        if(0 == head_flag){
            qDebug()<<QString("COM%1 received head not found").arg(port+2);
        }
    }
    CacheDataProc(rtd,0,flag,Dec,Name,Code,Unit);
//添加TOC状态检测 空闲状态         myApp::TOC_Isok=true;
    qDebug()<<QString("TOC In_level[%1]").arg(myApp::In_level);
    if(24 != myApp::In_level && GetSwitchStatus(myApp::In_level)==false){
        return;
    }
    qDebug()<<QString("TOC TOC_Flag[%1] TOC_Isok[%2]").arg(myApp::TOC_Flag).arg(myApp::TOC_Isok);
    if(myApp::TOC_Flag&&myApp::TOC_Isok==true){
        qDebug()<<QString("TOC start");
        myApp::TOC_Flag=0;
        myApp::TOC_Isok=false;
        //添加COD取样指令
        sendbuf.resize(13);
        sendbuf[0]=Address;
        sendbuf[1]=0x10;        
        sendbuf[2]=0x00;
        sendbuf[3]=0x00;      
        sendbuf[4]=0x00;
        sendbuf[5]=0x02;       
        sendbuf[6]=0x04;
        sendbuf[7]=0x00;
        sendbuf[8]=0x01;
        sendbuf[9]=0x00;
        sendbuf[10]=(char)(path + 1);
        
        check = myHelper::CRC16_Modbus(sendbuf.data(),11);
        sendbuf[11]=(char)(check);
        sendbuf[12]=(char)(check>>8);
        myCom[port]->readAll();
        qDebug()<<QString("COM%1 send:").arg(port+2)<<sendbuf.toHex().toUpper();
        myCom[port]->write(sendbuf);
        myCom[port]->flush();
        sleep(2);
        readbuf=myCom[port]->readAll();
        qDebug()<<QString("COM%1 received:").arg(port+2)<<readbuf.toHex().toUpper();
    }

}

//中绿   equipment: COD(1) NH3(2) 总磷(3)
void myAPI::Protocol_21(int port,int Dec,QString Name,QString Code,QString Unit,int COD_or_NH3,int equipment)
{
    double rtd=0;
    QString flag="D";
    QString sendbuf;
    QString readbuf;
    int    pindex;
    sendbuf="%0" +QString("%1").arg(equipment) + "#RDD0152101525**\r\n";
    myCom[port]->readAll();
    myCom[port]->write(sendbuf.toLatin1());
    myCom[port]->flush();
    qDebug()<<sendbuf;
    sleep(2);
    readbuf=myCom[port]->readAll();
    qDebug()<<readbuf;
    //%01$RDXXXX0803271521260000040718回车换行
    if(readbuf.length()>=34){
        pindex = readbuf.indexOf("$RD0000");
        if(pindex > -1 && readbuf.length() >= pindex + 34){
            rtd = (readbuf.mid(19+pindex,6) + "." + readbuf.mid(25+pindex,2)).toDouble();
        }
    }
    CacheDataProc(rtd,0,flag,Dec,Name,Code,Unit);
    if(24 != myApp::In_level && GetSwitchStatus(myApp::In_level)==true){
        return;
    }
    //添加COD/NH3状态检测 空闲状态         myApp::COD_Isok=true;
    if(0 == COD_or_NH3){
        if(0 == myApp::COD_Flag){
            return;
        }
        myApp::COD_Flag=0;
        //myApp::COD_Isok=false;
    }else{
        if(0 == myApp::NH3_Flag){
            return;
        }
        myApp::NH3_Flag=0;
        //myApp::NH3_Isok=false;
    }
    //添加COD/NH3做样指令
    sendbuf="%0" +QString("%1").arg(equipment) + "#WCSR06801**\r\n";
    myCom[port]->readAll();
    qDebug()<<sendbuf;
    myCom[port]->write(sendbuf.toLatin1());
    myCom[port]->flush();
}

double myAPI::HexToDouble(const unsigned char* bytes)
{
    quint64 data1 = bytes[0];
    quint64  data2 = bytes[1];
    quint64   data3 = bytes[2];
    quint64   data4 = bytes[3];
    quint64   data5 = bytes[4];
    quint64   data6 = bytes[5];
    quint64   data7 = bytes[6];
    quint64   data8 = bytes[7];

    quint64  data = data1 << 56 | data2 << 48 | data3 << 40 | data4<<32|data5<<24|data6<<16|data7<<8|data8;

    int nSign;
    if ((data>>63)==1)
    {
        nSign = -1;
    }
    else
    {
        nSign = 1;
    }
    quint64 nExp = data & (0x7FF0000000000000LL);
    nExp = nExp >> 52;

    double nMantissa = data & (0xFFFFFFFFFFFFFLL);

    if (nMantissa != 0)
        nMantissa = 1 + nMantissa / 4503599627370496LL;

    double value = nSign * nMantissa * (2 << (nExp - 2048));
    return value;
}



//与设备进行通讯
void myAPI::MessageFromCom(int port)
{
    QString Code;
    QString Name;
    QString  Unit;
    double alarm_max;
    double alarm_min;

    int Address;
    int Decimals;
    QSqlQuery query;
    QString sql=QString("select * from [ParaInfo] where [UseChannel]='COM%1'").arg(port+2);
    query.exec(sql);
    while(query.next())
    {
        Name=query.value(0).toString();
        Code=query.value(1).toString();
        Unit=query.value(2).toString();
        Address=query.value(4).toInt();
        Decimals=query.value(15).toInt();
        alarm_max=query.value(9).toDouble();
        alarm_min=query.value(10).toDouble();
        //qDebug()<<QString("protocol[%1] Name[%2]").arg(query.value(5).toInt()).arg(Name);
        switch (query.value(5).toInt())//通讯协议
        {
        case 1://明渠流量计
            Protocol_2(port,Address,Decimals,Name,Code,Unit);
        break;
        case 2://电导率
            Protocol_3(port,Address,Decimals,Name,Code,Unit);
        break;
        case 3://微兰COD
            Protocol_4(port,Address,Decimals,Name,Code,Unit,alarm_max);
        break;
        case 4://雨水刷卡设备
            Protocol_5(port);
        break;
        case 5://雨水采样仪
            Protocol_6(port);
        break;
        case 6://南控液位计
            Protocol_7(port,Address,Decimals,Name,Code,Unit,alarm_min,alarm_max);
        break;
        case 7://CE9628JM流量计
             Protocol_8(port,Decimals,Name,Code,Unit);
        break;
        case 8://PH-P206
            Protocol_9(port,Address,Decimals,Name,Code,Unit);
        break;
        case 9://天泽COD
            Protocol_10(port,Address,Decimals,Name,Code,Unit,0);
        break;
        case 10://承德流量计
            Protocol_11(port,Address,Decimals,Name,Code,Unit);
            break;
        case 11://哈希COD
            Protocol_12(port,Address,Decimals,Name,Code,Unit,0);
            break;
        case 12://哈希NH3
            Protocol_12(port,Address,Decimals,Name,Code,Unit,1);
            break;
        case 13://岛津TOC4200-1
            Protocol_14_Rtu(port,Address,Decimals,Name,Code,Unit,0);
            break;
        case 14://岛津TOC4200-2
            Protocol_14_Rtu(port,Address,Decimals,Name,Code,Unit,1);
            break;
        case 15://岛津TOC4200-3
            Protocol_14_Rtu(port,Address,Decimals,Name,Code,Unit,2);
            break;
        case 16://岛津TOC4200-4
            Protocol_14_Rtu(port,Address,Decimals,Name,Code,Unit,3);
            break;
        case 17://岛津TOC4200-5
            Protocol_14_Rtu(port,Address,Decimals,Name,Code,Unit,4);
            break;
        case 18://岛津TOC4200-6
            Protocol_14_Rtu(port,Address,Decimals,Name,Code,Unit,5);
            break;
        case 19://天泽氨氮
            Protocol_10(port,Address,Decimals,Name,Code,Unit,1);
            break;
        case 20://中绿COD
            Protocol_21(port,Decimals,Name,Code,Unit,0,1);
            break;
        case 21://中绿氨氮
            Protocol_21(port,Decimals,Name,Code,Unit,1,2);
            break;
        default: break;
        }
        usleep(COM[port].Interval*1000);
    }
}


bool myAPI::Get_Sampler_Status()
{
    QByteArray sendbuf;
    QByteArray readbuf;

    sendbuf[0]=0xAA;
    sendbuf[1]=0x3D;
    sendbuf[2]=0xBB;

    myCom[0]->readAll();
    myCom[0]->write(sendbuf);

    myHelper::Sleep(1000);
    readbuf=myCom[0]->readAll();

    if(0xCC==readbuf[0]&&0xDD==readbuf[1]&&0xAA==readbuf[2]&&0xBB==readbuf[6]){
         if(1<=readbuf[3]&&6>=readbuf[3]){
             if(0x06==readbuf[3]){
                myApp::Sample_Free=1;
            }
            else
            {
                myApp::Sample_Free=0;
            }
         }
         return true;
    }
    else return false;
}


bool myAPI::Get_Bottle_Status()
{
    QByteArray sendbuf;
    QByteArray readbuf;

    sendbuf[0]=0xAA;
    sendbuf[1]=0x35;
    sendbuf[2]=0xBB;

    myCom[0]->readAll();
    myCom[0]->write(sendbuf);
    qDebug()<<sendbuf.toHex();
    myHelper::Sleep(1000);
    readbuf=myCom[0]->readAll();
    qDebug()<<readbuf.toHex();
    return true;
}

//定量
bool myAPI::Sample_execute()
{
    QString sendbuf;
    QString readbuf;
    sendbuf="DR@24HB";
    myCom[0]->readAll();
    myCom[0]->write(sendbuf.toLatin1());
    myCom[0]->flush();
    qDebug()<<sendbuf;
    myHelper::Sleep(1000);
    readbuf=myCom[0]->readAll();
    qDebug()<<readbuf;
    if(readbuf=="A1DR@24OKHB"){
        sendbuf="DR@26HB";
        myCom[0]->write(sendbuf.toLatin1());
        myCom[0]->flush();
        myHelper::Sleep(1000);
        readbuf=myCom[0]->readAll();
        qDebug()<<readbuf;
        if(readbuf=="A1DR@26OKHB")
        {
            return true;
        }
        else return false;
    }
    else return false;

}


void myAPI::Insert_Message_Card_Add(QString QN,QString CardNo,QString CardType)
{
    QString sql;
    QSqlQuery query;

    sql = QString("insert into CardNumber([ReceivedTime],[QN],[CardNo],[CardType],[SendCount],[Attribute])values('%1','%2','%3','%4','0','add')")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
            .arg(QN)
            .arg(CardNo)
            .arg(CardType);

    query.exec(sql);//插入接收数据表
}

void myAPI::Update_Message_Card_delete(QString CardNo,QString CardType)
{
    QString sql;
    QSqlQuery query,query1;
    sql=QString("select [CardNo], [CardType] from CardNumber where [CardNo]='%1' and [CardType]='%2'")
            .arg(CardNo).arg(CardType);
    query.exec(sql);
    if(query.first())
    {
        sql = QString("update CardNumber set [Attribute]='delete'where [CardNo]='%1'").arg(CardNo);
        query1.exec(sql);
    }
    query1.clear();
    query.clear();
}


