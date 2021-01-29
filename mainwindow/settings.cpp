#include "include.h"
#include "ui_mainwindow.h"

extern EventLogger logger;
extern QSettings *settings;

void MainWindow::load_settings(void) {
    try {
        logger.info(Concern::Operation, QString("Reading settings from %1").arg(settings->fileName()));

        QString ip = settings->value("server/ip", "127.0.0.1").toString();
        unsigned short port = settings->value("server/port", 4805).toInt();
        QString station_id = settings->value("station/id", "none").toString();

        this->station = new Station(station_id);
        this->station->set_server(new Server(QHostAddress(ip), port, station_id));

        // Create the UFO manager
        this->station->set_ufo_manager(new UfoManager(
            settings->value("ufo/path", "C:\\Program Files\\UFO\\UFO.exe").toString(),
            settings->value("ufo/autostart", false).toBool()
        ));
        this->display_ufo_settings();

        this->load_settings_storage();


        bool debug = settings->value("debug", false).toBool();
        logger.set_level(debug ? Level::Debug : Level::Info);
        this->ui->cb_debug->setChecked(debug);

        this->station->set_manual_control(settings->value("manual", false).toBool());
        this->ui->cb_manual->setChecked(this->station->is_manual());
        this->ui->cb_safety_override->setEnabled(this->station->is_manual());

        logger.debug(Concern::Configuration, "Initializing serial ports...");
        serial_ports = QSerialPortInfo::availablePorts();

        for (QSerialPortInfo sp: this->serial_ports) {
            if (sp.portName() == settings->value("dome/port").toString()) {
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

        logger.fatal(Concern::Configuration, postmortem);
        exit(-4);
    }
}

void MainWindow::load_settings_storage(void) {
    settings->beginGroup("storage");
    this->station->set_scanner(
        QDir(settings->value("watch/path", "C:\\Data").toString())
    );
    this->station->set_storages(
        QDir(settings->value("primary/path", "C:\\Data").toString()),
        QDir(settings->value("permanent/path", "D:\\Data").toString())
    );
    this->station->primary_storage()->set_enabled(
        settings->value("primary/enabled", true).toBool()
    );
    this->station->permanent_storage()->set_enabled(
        settings->value("permanent/enabled", true).toBool()
    );
    this->display_permanent_storage_current_directory();
    settings->endGroup();
}

void MainWindow::load_settings_station(void) {
    settings->beginGroup("station");
    this->station->set_position(
        settings->value("latitude", 0).toDouble(),
        settings->value("longitude", 0).toDouble(),
        settings->value("altitude", 0).toDouble()
    );
    this->station->set_darkness_limit(settings->value("limits/darkness", -12.0).toDouble());
    this->station->set_humidity_limits(
        settings->value("humidity/lower", 75.0).toDouble(),
        settings->value("humidity/upper", 80.0).toDouble()
    );
}

// Handle changes in station settings
void MainWindow::on_bt_station_apply_clicked() {
    // If ID, IP or port are changed, update the server settings
    if (
        (this->ui->le_ip->text() != this->station->server()->address().toString()) ||
        (this->ui->sb_port->value() != this->station->server()->port()) ||
        (this->ui->le_station_id->text() != this->station->get_id())
    ) {
        if (this->ui->le_station_id->text() != this->station->get_id()) {
            this->station->set_id(this->ui->le_station_id->text());
        }
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
        this->display_sun_longterm();
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
    settings->setValue("server/ip", this->station->server()->address().toString());
    settings->setValue("server/port", this->station->server()->port());
    settings->setValue("station/id", this->station->get_id());
    settings->setValue("station/latitude", this->station->latitude());
    settings->setValue("station/longitude", this->station->longitude());
    settings->setValue("station/altitude", this->station->altitude());

    settings->setValue("limits/darkness", this->station->darkness_limit());
    settings->setValue("limits/humidity_lower", this->station->humidity_limit_lower());
    settings->setValue("limits/humidity_upper", this->station->humidity_limit_upper());

    settings->sync();

    this->button_station_toggle(false);
}

void MainWindow::on_bt_station_reset_clicked() {
    logger.debug(Concern::Configuration, "Discard changes to station settings");
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

void MainWindow::slot_station_edited(void) {
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
    this->ui->bt_station_reset->setEnabled(changed);
}
