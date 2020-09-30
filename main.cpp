#include "mainwindow.h"

#include <QApplication>

MainWindow* main;

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    main = &w;
    w.show();
    return a.exec();
}
