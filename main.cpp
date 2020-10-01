#include "mainwindow.h"

#include <QApplication>

MainWindow* main_window;

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
