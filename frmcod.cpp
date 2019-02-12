#include "frmcod.h"
#include "ui_frmcod.h"
#include "api/myhelper.h"
#include "api/myapi.h"


frmcod::frmcod(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::frmcod)
{
    ui->setupUi(this);
    myHelper::FormInCenter(this,myApp::DeskWidth,myApp::DeskHeigth);//窗体居中显示
    this->InitStyle();
}

frmcod::~frmcod()
{
    delete ui;
}

void frmcod::InitStyle()
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
}


void frmcod::on_btn_Cancel_clicked()
{
    this->close();
}

void frmcod::on_btn_StartWork_clicked()
{
    if(myApp::manual_control==false){
        myHelper::ShowMessageBoxError("请切换到运维模式");
        return ;
    }
    myApp::COD_Flag=1;
    myApp::TOC_Flag=1;
    myApp::NH3_Flag=1;
}
