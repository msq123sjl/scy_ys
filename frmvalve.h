#ifndef FRMVALVE_H
#define FRMVALVE_H

#include <QWidget>

namespace Ui {
class frmValve;
}

class frmValve : public QWidget
{
    Q_OBJECT
    
public:
    explicit frmValve(QWidget *parent = 0);
    ~frmValve();

    bool Valve_Open_Set();
    void Valve_Open_Clear();
    bool Valve_Close_Set();
    void Valve_Close_Clear();
    bool Catchment_Valve_Open_Set();
    void Catchment_Valve_Open_Clear();
    bool Catchment_Valve_Close_Set();
    void Catchment_Valve_Close_Clear();

private slots:

    void on_btn_ValveOpen_clicked();

    void on_btn_ValveClose_clicked();

    void on_btn_Cancel_clicked();

    void on_btn_CatchValveOpen_clicked();

    void on_btn_CatchValveClose_clicked();

private:
    Ui::frmValve *ui;

    void InitStyle();
    void Delay_MSec(unsigned int msec);//非阻塞延时


};

#endif // FRMVALVE_H
