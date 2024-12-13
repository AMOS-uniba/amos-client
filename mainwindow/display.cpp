#include "settings.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "utils/formatters.h"


void MainWindow::display_time(void) {
    this->statusBar()->showMessage(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    this->ui->lb_uptime->setText(
        Formatters::format_duration(this->ui->station->start_time().secsTo(QDateTime::currentDateTimeUtc()))
    );
    this->ui->server->display_countdown();
}

void MainWindow::display_window_title(void) {
#if OLD_PROTOCOL
    QString protocol = " <Senec>";
#else
    QString protocol = "";
#endif
    this->setWindowTitle(QString("AMOS client %1%2 [%3]%4").arg(
         VERSION_STRING,
         protocol,
         this->ui->station->is_manual() ? "manual" : "automatic",
         this->ui->station->is_safety_overridden() ? " [safety override]" : ""
    ));
}
