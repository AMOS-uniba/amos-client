#include "include.h"
#include "ui_mainwindow.h"

extern EventLogger logger;

void MainWindow::load_settings(void) {
    try {
        QString ip = this->settings->value("server/ip", "127.0.0.1").toString();
        unsigned short port = this->settings->value("server/port", 4805).toInt();
        QString station_id = this->settings->value("station/id", "none").toString();

        this->station = new Station(station_id);
        this->station->set_server(new Server(QHostAddress(ip), port, station_id));

        // Create the UFO manager
        this->station->set_ufo_manager(new UfoManager(
            this->settings->value("ufo/path", "C:\\Program Files\\UFO\\UFO.exe").toString(),
            this->settings->value("ufo/autostart", false).toBool()
        ));
        this->display_ufo_settings();

        this->station->set_storages(
            QDir(this->settings->value("storage/primary", "C:\\Data").toString()),
            QDir(this->settings->value("storage/permanent", "D:\\Data").toString())
        );
        this->station->set_scanner(
            QDir(this->settings->value("storage/watch", "C:\\Data").toString())
        );
        this->station->set_position(
            this->settings->value("station/latitude", 0).toDouble(),
            this->settings->value("station/longitude", 0).toDouble(),
            this->settings->value("station/altitude", 0).toDouble()
        );
        this->station->set_darkness_limit(this->settings->value("limits/darkness", -12.0).toDouble());
        this->station->set_humidity_limits(
            this->settings->value("limits/humidity_lower", 75.0).toDouble(),
            this->settings->value("limits/humidity_upper", 80.0).toDouble()
        );

        bool debug = this->settings->value("debug", false).toBool();
        logger.set_level(debug ? Level::Debug : Level::Info);
        this->ui->cb_debug->setChecked(debug);

        this->station->set_manual_control(this->settings->value("manual", false).toBool());
        this->ui->cb_manual->setChecked(this->station->is_manual());
        this->ui->cb_safety_override->setEnabled(this->station->is_manual());

        logger.debug("Now setting serial ports...");
        serial_ports = QSerialPortInfo::availablePorts();

        for (QSerialPortInfo sp: this->serial_ports) {
            if (sp.portName() == this->settings->value("dome/port")) {
                this->ui->co_serial_ports->setCurrentText(sp.portName());
                this->station->dome()->set_serial_port(sp.portName());
            }
        }
    } catch (ConfigurationError &e) {
        QString postmortem = QString("Fatal configuration error: %1").arg(e.what());
        QMessageBox box;
        box.setText(postmortem);
        box.setIcon(QMessageBox::Icon::Critical);
        box.setWindowIcon(QIcon(":/images/blue.ico"));
        box.setWindowTitle("Configuration error");
        box.setStandardButtons(QMessageBox::Ok);
        box.exec();

        logger.fatal(postmortem);
        exit(-4);
    }
}

// Handle changes in station settings
void MainWindow::on_bt_station_apply_clicked() {
    // Update ID, if changed
    if (this->ui->le_station_id->text() != this->station->get_id()) {
        this->station->set_id(this->ui->le_station_id->text());
    }

    // If IP or port are changed, update the server settings
    if ((this->ui->le_ip->text() != this->station->server()->address().toString()) ||
        (this->ui->sb_port->value() != this->station->server()->port())) {
        this->station->server()->set_url(
            QHostAddress(this->ui->le_ip->text()),
            this->ui->sb_port->value(),
            this->station->get_id()
        );
    }

    // Update the position, if changed
    if (
        (this->ui->dsb_latitude->value() != this->station->latitude()) ||
        (this->ui->dsb_longitude->value() != this->station->longitude()) ||
        (this->ui->dsb_altitude->value() != this->station->altitude())
    ) {
        this->station->set_position(
            this->ui->dsb_latitude->value(),
            this->ui->dsb_longitude->value(),
            this->ui->dsb_altitude->value()
        );
    }

    // Update the darkness limit, if changed
    if (this->ui->dsb_darkness_limit->value() != this->station->darkness_limit()) {
        this->station->set_darkness_limit(this->ui->dsb_darkness_limit->value());
    }

    // Update the humidity limits, if changed
    if (
        (this->ui->dsb_humidity_limit_upper->value() != this->station->humidity_limit_upper()) ||
        (this->ui->dsb_humidity_limit_lower->value() != this->station->humidity_limit_lower())
    ) {
        this->station->set_humidity_limits(
            this->ui->dsb_humidity_limit_lower->value(),
            this->ui->dsb_humidity_limit_upper->value()
        );
    }

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
    logger.debug("Discard changes to station settings");
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

void MainWindow::on_station_edited(void) {
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

void MainWindow::button_station_toggle(bool changed) {
    this->ui->bt_station_apply->setText(QString("%1 changes").arg(changed ? "Apply" : "No"));
    this->ui->bt_station_apply->setEnabled(changed);
}
