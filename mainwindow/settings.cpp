#include "include.h"
#include "ui_mainwindow.h"

extern EventLogger logger;
extern QSettings *settings;

void MainWindow::load_settings(void) {
    try {
        logger.info(Concern::Operation, QString("Reading settings from %1").arg(settings->fileName()));

        QString ip = settings->value("server/ip", "127.0.0.1").toString();
        QString station_id = settings->value("station/id", "none").toString();

        this->ui->station->set_server(this->ui->server);
        this->ui->station->set_storages(this->ui->storage_primary, this->ui->storage_permanent);
        this->ui->station->set_dome(this->ui->dome);
        this->ui->station->set_ufo_manager(this->ui->ufo_manager);

        this->load_settings_storage();
        logger.load_settings();

        // Load and set debug levels
        bool debug = settings->value("debug", false).toBool();
        logger.set_level(debug ? Level::Debug : Level::Info);
        this->ui->cb_debug->setChecked(debug);

        // Load and set manual/automatic mode
        this->ui->station->set_manual_control(settings->value("manual", false).toBool());
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

/**
 * @brief MainWindow::load_settings_storage
 * Loads and applies settings from the `storage` group
 */
void MainWindow::load_settings_storage(void) {
    this->ui->scanner->set_directory(QDir(settings->value("storage/scanner_path", "C:\\Data").toString()));
    this->ui->scanner->set_enabled(settings->value("storage/scanner_enabled", true).toBool());
    this->ui->scanner->scan_info();

    this->ui->storage_primary->set_directory(QDir(settings->value("storage/primary_path", "C:\\Data").toString()));
    this->ui->storage_primary->set_enabled(settings->value("storage/primary_enabled", true).toBool());
    this->ui->storage_primary->scan_info();

    this->ui->storage_permanent->set_directory(QDir(settings->value("storage/permanent_path", "D:\\Data").toString()));
    this->ui->storage_permanent->set_enabled(settings->value("storage/permanent_enabled", true).toBool());
    this->ui->storage_permanent->scan_info();
}

/**
 * @brief MainWindow::on_bt_station_apply_clicked
 * Handler of changes to the station settings in the GUI
 *//*
void MainWindow::on_bt_station_apply_clicked(void) {
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
        this->ui->sun_info->update_long_term();
    }

    // Store all new values in permanent settings
    settings->setValue("station/latitude", this->station->latitude());
    settings->setValue("station/longitude", this->station->longitude());
    settings->setValue("station/altitude", this->station->altitude());
    settings->setValue("station/darkness", this->station->darkness_limit());

    settings->sync();

    this->button_station_toggle(false);
}

void MainWindow::on_bt_station_reset_clicked() {
    logger.debug(Concern::Configuration, "Discard changes to station settings");
    this->ui->dsb_latitude->setValue(this->station->latitude());
    this->ui->dsb_longitude->setValue(this->station->longitude());
    this->ui->dsb_altitude->setValue(this->station->altitude());

    this->ui->dsb_darkness_limit->setValue(this->station->darkness_limit());
}*/
/*
void MainWindow::slot_station_edited(void) {
    this->button_station_toggle(
        (this->ui->dsb_latitude->value() != this->station->latitude()) ||
        (this->ui->dsb_longitude->value() != this->station->longitude()) ||
        (this->ui->dsb_altitude->value() != this->station->altitude()) ||
        (this->ui->dsb_darkness_limit->value() != this->station->darkness_limit())
    );
}

void MainWindow::button_station_toggle(bool changed) {
    this->ui->bt_station_apply->setText(QString("%1 changes").arg(changed ? "Apply" : "No"));
    this->ui->bt_station_apply->setEnabled(changed);
    this->ui->bt_station_reset->setEnabled(changed);
}
*/
