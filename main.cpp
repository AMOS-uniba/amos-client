#include "include.h"

#include <QApplication>

MainWindow* main_window;
EventLogger logger(main_window, "events.log");

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setApplicationName("AMOS client");
    a.setOrganizationName("AMOS");
    logger.initialize();

    MainWindow w;
    w.show();
    return a.exec();
}

