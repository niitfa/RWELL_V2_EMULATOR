#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "rwell_emulator.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void createEmulator(std::string ip, uint16_t port);
private:
    void setupTimer();
    void setupActivitySwitch();
    void printActualActivity(int activity);
    void printActualTemperature(int temperature);
    void printActualPressure(int pressure);
    void printActualVoltage(int voltage);
private slots:
    void update();
    void on_switch_activityEnable_clicked();

    void on_lineEdit_temperature_editingFinished();

    void on_lineEdit_pressure_editingFinished();

    void on_lineEdit_voltage_editingFinished();

    void on_pushButton_temperature_clicked();

    void on_pushButton_pressure_clicked();

    void on_pushButton_voltage_clicked();

private:
    Ui::MainWindow *ui;
    RWELLEmulator *emulator = nullptr;
    QTimer* timer = new QTimer();
    int updPeriod = 50;
};

#endif // MAINWINDOW_H
