#include "include.h"
#include "ui_mainwindow.h"

// Handles
void MainWindow::on_bt_station_apply_clicked() {
    this->station->set_id(this->ui->le_station_id->text());
    this->station->server()->set_url(
        QHostAddress(this->ui->le_ip->text()),
        this->ui->sb_port->value(),
        this->station->get_id()
    );
    this->station->set_position(
        this->ui->dsb_latitude->value(),
        this->ui->dsb_longitude->value(),
        this->ui->dsb_altitude->value()
    );
    this->station->set_darkness_limit(this->ui->dsb_darkness_limit->value());
    this->station->set_humidity_limits(
        this->ui->dsb_humidity_limit_lower->value(),
        this->ui->dsb_humidity_limit_upper->value()
    );

    // Store all new values in permanent settings
    this->settings->setValue("server/ip", this->station->server()->address().toString());
    this->settings->setValue("server/port", this->station->server()->port());
    this->settings->setValue("station/id", this->station->get_id());
    this->settings->setValue("station/latitude", this->station->latitude());
    this->settings->setValue("station/longitude", this->station->longitude());
    this->settings->setValue("station/altitude", this->station->altitude());

    this->settings->setValue("limits/darkness", this->station->darkness_limit());
    this->settings->setValue("limits/humidity_lower", this->station->humidity_limit_lower());
    this->settings->setValue("limits/humidity_upper", this->station->humidity_limit_upper());

    this->settings->sync();

    this->button_station_toggle(false);
}

void MainWindow::on_bt_station_reset_clicked() {
    this->ui->le_station_id->setText(this->station->get_id());
    this->ui->le_ip->setText(this->station->server()->address().toString());
    this->ui->sb_port->setValue(this->station->server()->port());

    this->ui->dsb_latitude->setValue(this->station->latitude());
    this->ui->dsb_longitude->setValue(this->station->longitude());
    this->ui->dsb_altitude->setValue(this->station->altitude());

    this->ui->dsb_darkness_limit->setValue(this->station->darkness_limit());
    this->ui->dsb_humidity_limit_lower->setValue(this->station->humidity_limit_lower());
    this->ui->dsb_humidity_limit_upper->setValue(this->station->humidity_limit_upper());
}

void MainWindow::station_edited(void) {
    this->button_station_toggle(
        (this->ui->le_station_id->text() != this->station->get_id()) ||
        (this->ui->dsb_latitude->value() != this->station->latitude()) ||
        (this->ui->dsb_longitude->value() != this->station->longitude()) ||
        (this->ui->dsb_altitude->value() != this->station->altitude()) ||
        (this->ui->le_ip->text() != this->station->server()->address().toString()) ||
        (this->ui->sb_port->value() != this->station->server()->port()) ||
        (this->ui->dsb_darkness_limit->value() != this->station->darkness_limit()) ||
        (this->ui->dsb_humidity_limit_lower->value() != this->station->humidity_limit_lower()) ||
        (this->ui->dsb_humidity_limit_upper->value() != this->station->humidity_limit_upper())
    );
}

void MainWindow::button_station_toggle(bool enable) {
    this->ui->bt_station_apply->setText(QString("%1 changes").arg(enable ? "Apply" : "No"));
    this->ui->bt_station_apply->setEnabled(enable);
}
