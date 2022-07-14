#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , rtsp(new RTSPData)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    if(rtsp->isRunning()){
        rtsp->requestInterruption();
        rtsp->quit();
        rtsp->deleteLater();
    }
    delete ui;
}


void MainWindow::on_btnRun_clicked()
{
    rtsp->rtspInit("rtsp://192.168.1.31/test");
    rtsp->start();
}

