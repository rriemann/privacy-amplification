#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    if(a.arguments().size() > 1)
        w.fileOpen(a.arguments().at(1));
    w.show();
    
    return a.exec();
}
