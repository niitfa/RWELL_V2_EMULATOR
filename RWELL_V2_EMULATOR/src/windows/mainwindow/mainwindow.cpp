#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("RWELL Emulator 2.0");
    setFixedSize(390, 230);
    setupTimer();
    setupActivitySwitch();
    //QObject::connect(timer, SIGNAL(timeout()), this, SLOT(update()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createEmulator(std::string ip, uint16_t port)
{
    std::cout << "Running RWELL emulator " << ip << ":" << port << std::endl;
    emulator = new RWELLEmulator(ip, port);
    std::function<void()> f(std::bind(&MainWindow::update, this));
    emulator->setMessageUpdatedCallback(f);
    emulator->start();
}

void MainWindow::setupTimer()
{
    if(this->timer)
    {
        timer->setSingleShot(false);
        timer->setInterval(updPeriod);
        timer->start();
    }
}

void MainWindow::setupActivitySwitch()
{
    ui->switch_activityEnable->setText("");
    ui->switch_activityEnable->setChecked(true);
}

void MainWindow::printActualActivity(int activity)
{
     ui->label_actualActivity->setText("/ " + QString::number(activity) );
}

void MainWindow::printActualTemperature(int temperature)
{
    ui->label_actualTemperature->setText(
                "/ " + QString::number(temperature / 100., 'f', 2) + " ℃"
                );
}

void MainWindow::printActualPressure(int pressure)
{
    ui->label_actualPressure->setText(
                "/ " + QString::number(pressure / 10000., 'f', 2) + " бар"
                );
}

void MainWindow::printActualVoltage(int voltage)
{
    ui->label_actualVoltage->setText(
                "/ " + QString::number(voltage) + " В"
                );
}

void MainWindow::update()
{
    if(emulator)
    {
        printActualActivity( emulator->getActivity() );
        printActualTemperature( emulator->getTemperature() );
        printActualPressure( emulator->getPressure() );
        printActualVoltage( emulator->getVoltage() );
    }
}

void MainWindow::on_switch_activityEnable_clicked()
{
    if(emulator)
    {
        emulator->enableActivity(ui->switch_activityEnable->isChecked());
    }
}

void MainWindow::on_lineEdit_temperature_editingFinished()
{
    bool ok = false;
    ui->lineEdit_temperature->text().toDouble(&ok);
    if(!ok)
    {
         ui->lineEdit_temperature->setText(QString::number(0, 'f', 2));
    }

}

void MainWindow::on_lineEdit_pressure_editingFinished()
{
    bool ok = false;
    ui->lineEdit_pressure->text().toDouble(&ok);
    if(!ok)
    {
         ui->lineEdit_pressure->setText(QString::number(0, 'f', 2));
    }
}

void MainWindow::on_lineEdit_voltage_editingFinished()
{
    bool ok = false;
    ui->lineEdit_voltage->text().toDouble(&ok);
    if(!ok)
    {
         ui->lineEdit_voltage->setText(QString::number(0));
    }
}

void MainWindow::on_pushButton_temperature_clicked()
{
    if(emulator)
    {
        int temp = ui->lineEdit_temperature->text().toDouble() * 100;
        emulator->setTemperature(temp);
    }
}

void MainWindow::on_pushButton_pressure_clicked()
{
    if(emulator)
    {
        int pressure = ui->lineEdit_pressure->text().toDouble() * 10000;
        emulator->setPressure(pressure);
    }
}

void MainWindow::on_pushButton_voltage_clicked()
{
    if(emulator)
    {
        int voltage = ui->lineEdit_voltage->text().toInt();
        emulator->setVoltage(voltage);
    }
}
