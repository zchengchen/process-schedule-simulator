#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include<QMainWindow>
#include"pcb.h"
#include"mem_block.h"
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
    void on_add_job_button_clicked();
    void on_run_button_clicked();
    void on_suspend_button_clicked();
    void on_unsuspend_button_clicked();
    void on_start_button_clicked();
    void on_terminate_button_clicked();
    void on_syn_button_clicked();
    void on_checkBox_clicked(bool checked);


private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
