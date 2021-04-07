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
    ui->wgtPlayer->play();
}

void MainWindow::on_btnStop_clicked()
{
    ui->wgtPlayer->stop();
}
