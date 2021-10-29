#include"mainwindow.h"
#include"mem_block.h"
#include"pcb.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
