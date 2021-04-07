#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_btnPlay_clicked()
{
    QString url = ui->lineUrl->text();
    if(url.isEmpty()){
        QMessageBox::information(this,tr("Warning"),"Please input url first.",QMessageBox::Ok);
        return;
    }
    ui->wgtPlayer->play(url);
}

void MainWindow::on_btnStop_clicked()
{
    ui->wgtPlayer->stop();
}

void MainWindow::on_spinBoxContrast_valueChanged(int c)
{
    contrast=c;
    ui->wgtPlayer->setFilterDescr(c,brightness);
}

void MainWindow::on_spinBoxLightness_valueChanged(int b)
{
    brightness=b;
    ui->wgtPlayer->setFilterDescr(contrast,b);
}
