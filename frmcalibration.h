#ifndef FRMCALIBRATION_H
#define FRMCALIBRATION_H

#include <QWidget>
#include <QTimer>
#include <QDebug>
#include <QMessageBox>
#include "myapp.h"


namespace Ui {
class frmcalibration;
}

class frmcalibration : public QWidget
{
    Q_OBJECT
    
public:
    explicit frmcalibration(QWidget *parent = 0);
    ~frmcalibration();
    static float AD_to_Ia(float AdValue,stAnalog_para *pAnalog,int flag){
        float Ia = 20.00;
        for(int iLoop = 0; iLoop < CALIBRATION_CNT; iLoop++){
            if(AdValue >= pAnalog->para[iLoop].min_ad \
                && AdValue < pAnalog->para[iLoop].max_ad){
            
                Ia = pAnalog->para[iLoop].slope * (AdValue - pAnalog->para[iLoop].min_ad) + pAnalog->para[iLoop].Ia_base;
                qDebug()<<QString("slope[%1] ad_value[%2] max[%3] min[%4] Ia[%5]").arg(pAnalog->para[iLoop].slope).arg(AdValue).arg(pAnalog->para[iLoop].max_ad).arg(pAnalog->para[iLoop].min_ad).arg(Ia);
                break;
            }
        }
        if(flag){
            Ia = Ia > pAnalog->max_Ia ? pAnalog->max_Ia : Ia;
            Ia = Ia < pAnalog->min_Ia ? pAnalog->min_Ia : Ia;
        }
        return Ia;
    }
    
private slots:
    void on_btnCancel_clicked();

    void on_btnCalibration_clicked();

    void on_btnMeasure_clicked();

    void ShowDate();
private:
    Ui::frmcalibration *ui;

    int  SampleResistor;
    int   channel;
    int  columnWidths[4];        //列宽数组
    float value;
    QString Unit;
    //QTimer *timerDate;
    QMessageBox *msg;
    void InitForm();
    void InitCalibrationTable();
    void InitStyle();
    void CalibrationOrMeasure(char flag);
    int ShowMessageBoxMeasureQuesion(QString info);
};

#endif // FRMCALIBRATION_H
