#include "include.h"

#include <QApplication>

MainWindow* main_window;
EventLogger logger(main_window, "events.log");
QSettings *settings;

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setApplicationName("AMOS client");
    a.setOrganizationName("AMOS");
    logger.initialize();

    main_window = new MainWindow();
    main_window->showMaximized();

    int ret = a.exec();
    delete main_window;
    return ret;
}

