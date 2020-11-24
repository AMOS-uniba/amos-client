#include "include.h"

#include <QApplication>

MainWindow* main_window;
EventLogger logger(main_window, "client.log");
StateLogger state_logger(main_window, "state.log");

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
