#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "settings.h"
#include "include.h"

#include <QLabel>
#include <QPushButton>

extern EventLogger logger;
extern QSettings *settings;

void MainWindow::display_time(void) {
    statusBar()->showMessage(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    this->ui->lb_uptime->setText(
        this->format_duration(this->m_start_time.secsTo(QDateTime::currentDateTimeUtc()))
    );
    this->ui->server->display_countdown();
}

void MainWindow::display_window_title(void) {
#if OLD_PROTOCOL
    QString protocol = " <Senec>";
#else
    QString protocol = "";
#endif
    this->setWindowTitle(QString("AMOS client%1 [%2 mode]%3").arg(
                             protocol,
                             this->ui->station->is_manual() ? "manual" : "automatic",
                             this->ui->station->is_safety_overridden() ? " [safety override]" : ""
    ));
}
