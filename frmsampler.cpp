#include "frmsampler.h"
#include "ui_frmsampler.h"
#include "api/myapp.h"
#include "api/myapi.h"
#include "api/myhelper.h"

extern Sampler sampler;
extern QextSerialPort *myCom[6];
frmSampler::frmSampler(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::frmSampler)
{
    ui->setupUi(this);
    myHelper::FormInCenter(this,myApp::DeskWidth,myApp::DeskHeigth);//窗体居中显示
    this->InitStyle();
    this->InitForm();
}

frmSampler::~frmSampler()
{
    delete ui;
}

void frmSampler::InitStyle()//此处作用？
{
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
}

void frmSampler::InitForm()
{
     myApp::ReadConfig();
    ui->txtSampleFlow->setText(QString::number(sampler.SampleFlow));                                                                                                                                  //采样量
    ui->comboBox_SampleStartBottle->setCurrentIndex(ui->comboBox_SampleStartBottle->findText(QString::number(sampler.SampleStartBottle)));//开始瓶号
}

//定量
bool frmSampler::SampleFixFlow()
{
    return true;
}


//获取采样瓶状态
bool frmSampler::GetSamplerStatus()
{

return true;
}

void frmSampler::showbottle()
{

}
bool frmSampler::CleanSamplerStatus()
{
    return true;
}

void frmSampler::on_btn_StartSampling_clicked()   //采样启动
{
    if(myApp::manual_control==false){
        myHelper::ShowMessageBoxError("请切换到运维模式");
        return ;
    }
    myAPI cy;
   cy.Sample_execute();
}


void frmSampler::on_btn_Cancel_clicked()
{
    this->close();
}


