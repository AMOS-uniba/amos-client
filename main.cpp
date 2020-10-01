#include "include.h"

#include <QApplication>

MainWindow* main_window;
Log logger(main_window, "client.log");

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
