#include "mainwindow.h"
#include "logging/eventlogger.h"
#include "logging/statelogger.h"

#include <QApplication>
#include <exception>
#include <cstdlib>

#include "utils/state/serialportstate.h"


MainWindow * main_window;
EventLogger logger(main_window, "events.log");
QSettings * settings;

static std::terminate_handler previous_terminate = nullptr;

/**
 * @brief Name the reason in the log before dying.
 * An exception that escapes a slot cannot be caught here -- crossing the event loop has already
 * terminated us by the time this runs -- but it can at least be written down. Every such crash was
 * previously indistinguishable from the process simply vanishing.
 */
static void log_and_terminate(void) {
    QString reason = "no active exception";

    if (std::exception_ptr current = std::current_exception()) {
        try {
            std::rethrow_exception(current);
        } catch (const std::exception & e) {
            reason = QString("uncaught exception: %1").arg(e.what());
        } catch (...) {
            reason = "uncaught exception of unknown type";
        }
    }

    // The file is written before the table widget is touched, so the line survives even if the GUI
    // is already past saving.
    logger.fatal(Concern::Operation, QString("Terminating abnormally: %1").arg(reason));

    if (previous_terminate != nullptr) {
        previous_terminate();
    }
    std::abort();
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setApplicationName("AMOS client");
    a.setOrganizationName("AMOS");

    qRegisterMetaType<SerialPortState>("SerialPortState");
    qRegisterMetaType<Concern>("Concern");
    qRegisterMetaType<Level>("Level");
    qRegisterMetaType<QVector<int>>("QVector<int>");

    logger.initialize();
    previous_terminate = std::set_terminate(log_and_terminate);

    main_window = new MainWindow();
    main_window->resize(2000, 1600);
    main_window->showMaximized();

    int ret = 0;
    try {
        ret = a.exec();
    } catch (const std::exception & e) {
        logger.fatal(Concern::Operation, QString("Exception escaped the event loop: %1").arg(e.what()));
        throw;
    }

    delete main_window;
    return ret;
}

