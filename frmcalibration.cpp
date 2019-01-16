#include "frmcalibration.h"
#include "ui_frmcalibration.h"
#include "api/myapp.h"
#include "api/myhelper.h"
#include <QStandardItemModel>
#include <QTimer>
QStandardItemModel  *model_calibration;
extern float ad_midvalue[ANALOG_CNT];
extern stAnalog_para Analog[ANALOG_CNT];

frmcalibration::frmcalibration(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::frmcalibration)
{
    ui->setupUi(this);
    myHelper::FormInCenter(this,myApp::DeskWidth,myApp::DeskHeigth);//窗体居中显示
    this->InitForm();
    this->InitCalibrationTable();
}

frmcalibration::~frmcalibration()
{
    delete ui;
}

void frmcalibration::InitStyle()
{    
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);//
}

void frmcalibration::InitForm()
{
}

void frmcalibration::InitCalibrationTable()
{
    int iLoop;
    model_calibration = new QStandardItemModel(this);
    model_calibration->setColumnCount(4);
    model_calibration->setHeaderData( 0,Qt::Horizontal,QString::fromLocal8Bit("校准值"));
    model_calibration->setHeaderData( 1,Qt::Horizontal,QString::fromLocal8Bit("测量值"));
    model_calibration->setHeaderData( 2,Qt::Horizontal,QString::fromLocal8Bit("误差"));
    model_calibration->setHeaderData( 3,Qt::Horizontal,QString::fromLocal8Bit("状态"));
//    columnWidths[0]=180;
    for(iLoop = 0;iLoop <= 2;iLoop++){
        columnWidths[iLoop]=100;
    }
    columnWidths[3]=200;
    ui->tableView->setModel(model_calibration);
    //表头信息显示居中
    ui->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignHCenter);
    //设置列宽根据内容变化
    for(iLoop=0;iLoop<model_calibration->columnCount();iLoop++){
        ui->tableView->horizontalHeader()->setResizeMode(iLoop,QHeaderView::Interactive);
        ui->tableView->setColumnWidth(iLoop,columnWidths[iLoop]);
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

    //ui->tableView->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);//垂直滚动条按项移动
}

void frmcalibration::on_btnCancel_clicked()
{
    this->close();
}


void frmcalibration::on_btnCalibration_clicked()
{
    CalibrationOrMeasure(1);
}

void frmcalibration::on_btnMeasure_clicked()
{
    CalibrationOrMeasure(0);
}

void frmcalibration::CalibrationOrMeasure(char flag)
{
    float step = ui->comboBox_step->currentText().toFloat() * 1000;
    channel = ui->comboBox_port->currentText().toInt() - 1;
    SampleResistor = ui->txt_sampleResistor->text().toInt();
    float value_min,value_max,error;
    float Ia = 0,AdValue = 0;
    int row=0;
    //int column=0;
    unsigned int iLoop;
    //QString Unit;
    QRegExp Ex;
    QFont ft;
    ft.setPointSize(12);
    Ex.setPattern("(\\d+)-(\\d+)([A-Za-z]+)");
    if(Ex.indexIn(ui->comboBox_range->currentText()) != -1){
        value_min=Ex.cap(1).toFloat() * 1000;
        value_max=Ex.cap(2).toFloat() * 1000;
        Unit           =Ex.cap(3);
    }
    qDebug()<<QString("value_min:%1 value_max:%2 Unit:%3 step:%4 SampleResistor:%5").arg(value_min).arg(value_max).arg(Unit).arg(step).arg(frmcalibration::SampleResistor);
    ui->tableView->scrollToTop();
    for(value = value_min; value <= value_max;){
        //column = 0;
        model_calibration->setItem(row,0,new QStandardItem(QString("%1%2").arg(value/1000.0).arg(Unit)));
        model_calibration->item(row,0)->setTextAlignment(Qt::AlignCenter);           //设置字符位置
        model_calibration->setItem(row,3,new QStandardItem(QObject::trUtf8("测量中...")));
        model_calibration->item(row,3)->setTextAlignment(Qt::AlignCenter);        //设置字符位置
        //ui->tableView->scrollToBottom();//滚动到底部
        ui->tableView->scrollTo(model_calibration->index(row,3));

        if(ShowMessageBoxMeasureQuesion(QObject::trUtf8("标准值：%1%2\n测量值：%3%4").arg(value/1000.0).arg(Unit).arg(((ad_midvalue[channel] *5 * 1000)/SampleResistor)/(float)65536).arg(Unit)) == QMessageBox::AcceptRole){
            sleep(10);
            AdValue = 0;
            for(iLoop=0;iLoop<8;iLoop++){
                AdValue += ad_midvalue[channel];
                qDebug()<<QString("sum:[%1][%2]").arg(AdValue).arg(ad_midvalue[channel]);
                sleep(1);
            }
            AdValue = AdValue / 8;
            if(flag){
                Analog[channel].para[row].max_ad = (unsigned int)AdValue;
                if(0 == row){
                    Analog[channel].para[row].min_ad = 0;
                    Analog[channel].para[row].slope  = 0;
                    Analog[channel].para[row].Ia_base = 0;
                }else{
                    Analog[channel].para[row].slope  = step / (float)(Analog[channel].para[row].max_ad - Analog[channel].para[row].min_ad);
                    if(qAbs(Analog[channel].para[row].slope -Analog[channel].para[row-1].slope)  >  1.5 ){
                        continue;
                    }
                }
                Analog[channel].para[row+1].min_ad = (unsigned int)AdValue;
                Analog[channel].para[row+1].Ia_base = value;
                Analog[channel].para[row+1].slope = Analog[channel].para[row].slope;
                Analog[channel].para[row+1].max_ad = 65535;
            }
            Ia = AD_to_Ia(AdValue, &Analog[channel], 0);
            if((error = myHelper::float_div_float(qAbs(value - Ia), value_max-value_min)) > 20){
                continue;
            }
            //column++;
            //qDebug()<<QString("IA:%1%2").arg(QString::number(Ia/1000.00,'f',2)).arg(Unit);
            model_calibration->setItem(row,1,new QStandardItem(QString("%1%2").arg(QString::number(Ia/1000.00,'f',2)).arg(Unit)));
            model_calibration->item(row,1)->setTextAlignment(Qt::AlignCenter);           //设置字符位置
            //column++;
            //qDebug()<<QString("err:%1‰").arg(QString::number(myHelper::float_div_float(qAbs(value - Ia),value), 'f',2));
            model_calibration->setItem(row,2,new QStandardItem(QString("%1‰").arg(QString::number(myHelper::float_div_float(qAbs(value - Ia),value_max-value_min),'f',1))));
            model_calibration->item(row,2)->setTextAlignment(Qt::AlignCenter);           //设置字符位置
            if(flag){
                model_calibration->setItem(row,3,new QStandardItem(QObject::trUtf8("校准完成")));
            }else{
                model_calibration->setItem(row,3,new QStandardItem(QObject::trUtf8("测量完成")));
            }
            model_calibration->item(row,3)->setTextAlignment(Qt::AlignCenter);        //设置字符位置
            row++;
        }else{
            qDebug()<<QString("other");
        }
        sleep(1);
        value = value + step;
    }
    myApp::WriteConfig();
}

//显示询问框,确定和取消按钮
int frmcalibration::ShowMessageBoxMeasureQuesion(QString info)
{
    //msg = new QMessageBox(this);
   QMessageBox msg1;
   msg = &msg1;
   msg->clearFocus();
   msg->clearMask();
    //msg.setStyleSheet("font: 12pt '宋体';");
    msg->setWindowTitle("询问");
    msg->setText(info);
    msg->setIcon(QMessageBox::Question);
    msg->setCursor(QCursor(Qt::PointingHandCursor));
    msg->addButton("确定",QMessageBox::ActionRole);
    msg->addButton("取消",QMessageBox::RejectRole);
    msg->setWindowFlags(Qt::FramelessWindowHint);//去掉标题栏
    //timerDate=new QTimer(this);
    QTimer timerDate(&msg1);
    connect(&timerDate,SIGNAL(timeout()),this,SLOT(ShowDate()));
    timerDate.start(1000);
    return msg->exec();
}

//显示数据
void frmcalibration::ShowDate()
{
    qDebug()<<QString("message:[%1]").arg(ad_midvalue[channel]);
    msg->setText(QObject::trUtf8("标准值：%1%2\n测量值：%3%4").arg(value/1000.0).arg(Unit).arg(((ad_midvalue[channel] *5 * 1000)/SampleResistor)/(float)65536).arg(Unit));
}
