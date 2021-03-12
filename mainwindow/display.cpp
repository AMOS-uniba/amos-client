#include "include.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <QPushButton>

extern EventLogger logger;
extern QSettings *settings;

void MainWindow::display_cover_status(void) {
    /*
    const DomeStateS &stateS = this->station->dome()->state_S();
    QString text;
    QString colour;

    if (stateS.is_valid()) {
        if (stateS.servo_moving()) {
            text = stateS.servo_direction() ? "opening..." : "closing";
            colour = stateS.servo_direction() ? "hsv(120, 255, 96)" : "hsv(240, 255, 96)";
        } else {
            text = stateS.cover_safety_position() ? "peeking" :
                 stateS.dome_open_sensor_active() ? "open" :
                 stateS.dome_closed_sensor_active() ? "closed" : "inconsistent";
            colour = stateS.cover_safety_position() ? "hsv(180, 255, 160)" :
                 stateS.dome_open_sensor_active() ? "hsv(120, 255, 192)" :
                 stateS.dome_closed_sensor_active() ? "black" : "red";
        }
    } else {
        text = "no data";
        colour = "gray";
    }
    this->ui->lb_cover_state->setText(text);
    this->ui->lb_cover_state->setStyleSheet(QString("QLabel {color: %1; }").arg(colour));
    this->ui->lb_cover_comment->setText(this->station->state().display_string());

    // Set cover shaft position
    const DomeStateZ &stateZ = this->station->dome()->state_Z();

    this->ui->progress_cover->setEnabled(stateZ.is_valid());
    this->ui->progress_cover->setValue(stateZ.is_valid() ? stateZ.shaft_position() : 0);
    if (stateS.is_valid() && stateZ.is_valid()) {
        if (stateS.dome_open_sensor_active()) {
            this->ui->progress_cover->setMaximum(stateZ.shaft_position());
        }
    }
    */
}

void MainWindow::display_station_config(void) {
    this->ui->le_ip->setText(this->station->server()->address().toString());
    this->ui->sb_port->setValue(this->station->server()->port());
    this->ui->le_station_id->setText(this->station->get_id());

    this->ui->dsb_latitude->setValue(this->station->latitude());
    this->ui->dsb_longitude->setValue(this->station->longitude());
    this->ui->dsb_altitude->setValue(this->station->altitude());

    this->ui->dsb_darkness_limit->setValue(this->station->darkness_limit());
}

void MainWindow::display_ufo_settings(void) {
    this->ui->cb_ufo_auto->setChecked(this->station->ufo_manager()->autostart());
    this->ui->le_ufo_path->setText(this->station->ufo_manager()->path());
}

void MainWindow::display_ufo_state(void) {
    this->ui->lb_ufo_state->setText(this->station->ufo_manager()->state_string());
}

void MainWindow::display_time(void) {
    statusBar()->showMessage(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    ui->lb_next_heartbeat->setText(QString("%1 s").arg((double) this->timer_heartbeat->remainingTime() / 1000, 3, 'f', 1));
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
    this->setWindowTitle(QString("AMOS client %1 [%2 mode]%3").arg(
                             protocol,
                             this->station->is_manual() ? "manual" : "automatic",
                             this->station->is_safety_overridden() ? " [safety override]" : ""
    ));
}
