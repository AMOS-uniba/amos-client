#include "include.h"

#include <QApplication>

MainWindow* main_window;
EventLogger logger(main_window, "events.log");
//QSettings settings(QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation), "settings.ini"), QSettings::IniFormat);

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setApplicationName("AMOS client");
    a.setOrganizationName("AMOS");
    logger.initialize();

    MainWindow w;
    w.showMaximized();
    return a.exec();
}

