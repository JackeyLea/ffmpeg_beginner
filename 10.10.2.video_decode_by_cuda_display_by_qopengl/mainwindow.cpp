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

void MainWindow::on_btnPS_clicked()
{
    QString url = ui->lineUrl->text().trimmed();
    if(url.isEmpty()){
        QMessageBox::information(this,tr("Warning"),"Please input url",QMessageBox::Ok);
        return;
    }
    ui->widget->play(url);
}
