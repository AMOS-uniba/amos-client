#include "mainwindow.h"
#include "logging/eventlogger.h"
#include "logging/statelogger.h"

#include <QApplication>
#include "utils/state/serialportstate.h"


MainWindow * main_window;
EventLogger logger(main_window, "events.log");
QSettings * settings;

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setApplicationName("AMOS client");
    a.setOrganizationName("AMOS");

    qRegisterMetaType<SerialPortState>("SerialPortState");
    qRegisterMetaType<Concern>("Concern");
    qRegisterMetaType<Level>("Level");
    qRegisterMetaType<QVector<int> >("QVector<int>");

    logger.initialize();

    main_window = new MainWindow();
    main_window->showMaximized();

    int ret = a.exec();
    delete main_window;
    return ret;
}

