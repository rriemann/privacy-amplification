#include <QtGui/QApplication>
#include "mainwindow.h"
#include "demomode.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if(a.arguments().size() > 2 && a.arguments().at(1) == "-demo") {
        a.arguments().removeAt(1);
        DemoMode *demoMode = new DemoMode(a.arguments().last(), &a);
        a.connect(demoMode, SIGNAL(finished()), SLOT(quit()));
    } else {
        MainWindow *w = new MainWindow();
        if(a.arguments().size() > 1)
            w->fileOpen(a.arguments().at(1));
        w->show();
    }

    return a.exec();
}
