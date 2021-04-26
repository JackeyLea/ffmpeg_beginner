#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnPlay_clicked();

    void on_btnStop_clicked();

    void on_spinBoxContrast_valueChanged(int c);

    void on_spinBoxLightness_valueChanged(int b);

private:
    Ui::MainWindow *ui;

    int contrast=5,brightness=5;
};
#endif // MAINWINDOW_H
