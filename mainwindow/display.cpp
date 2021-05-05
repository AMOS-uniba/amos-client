#include "include.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <QPushButton>

extern EventLogger logger;
extern QSettings *settings;

void MainWindow::display_time(void) {
    statusBar()->showMessage(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    this->ui->lb_uptime->setText(
        this->format_duration(settings->value("run/last_run").toDateTime().secsTo(QDateTime::currentDateTimeUtc()))
    );
}

void MainWindow::display_window_title(void) {
#ifdef OLD_PROTOCOL
    QString protocol = " (old protocol)";
#else
    QString protocol = "";
#endif
    this->setWindowTitle(QString("AMOS client%1 [%2 mode]%3").arg(
                             protocol,
                             this->ui->station->is_manual() ? "manual" : "automatic",
                             this->ui->station->is_safety_overridden() ? " [safety override]" : ""
    ));
}
