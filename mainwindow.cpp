#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "include.h"

extern EventLogger logger;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {
    this->ui->setupUi(this);

    this->universe = new Universe();

    this->ui->tb_log->setColumnWidth(0, 140);
    this->ui->tb_log->setColumnWidth(1, 72);
    this->ui->tb_log->setColumnWidth(2, 72);

    this->display_serial_ports();

    logger.set_display_widget(this->ui->tb_log);
    logger.info(Concern::Operation, "Client initialized");

    // connect signals for handling of edits of station position
    this->settings = new QSettings(QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation), "settings.ini"), QSettings::IniFormat, this);
    this->settings->setValue("run/last_run", QDateTime::currentDateTimeUtc());
    this->load_settings();

    this->create_timers();

    this->connect(this->ui->dsb_latitude, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::on_station_edited);
    this->connect(this->ui->dsb_longitude, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::on_station_edited);
    this->connect(this->ui->dsb_altitude, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::on_station_edited);

    // connect signals for handling of edits of server address
    this->connect(this->ui->le_station_id, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &MainWindow::on_station_edited);
    this->connect(this->ui->le_ip, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &MainWindow::on_station_edited);
    this->connect(this->ui->sb_port, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::on_station_edited);

    // connect signals for handling of edits of safety limits
    this->connect(this->ui->dsb_darkness_limit, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::on_station_edited);
    this->connect(this->ui->dsb_humidity_limit_lower, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::on_station_edited);
    this->connect(this->ui->dsb_humidity_limit_upper, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::on_station_edited);

    this->create_actions();
    this->create_tray_icon();
    this->set_icon(StationState::NOT_OBSERVING);

    this->tray_icon->show();

    this->connect(this->tray_icon, &QSystemTrayIcon::messageClicked, this, &MainWindow::message_clicked);
    this->connect(this->tray_icon, &QSystemTrayIcon::activated, this, &MainWindow::icon_activated);

    this->connect(this->station->dome(), &Dome::state_updated_S, this, &MainWindow::display_basic_data);
    this->connect(this->station->dome(), &Dome::state_updated_T, this, &MainWindow::display_env_data);
    this->connect(this->station->dome(), &Dome::state_updated_Z, this, &MainWindow::display_shaft_position);

    this->connect(this->station, &Station::state_changed, this, &MainWindow::show_message);
    this->connect(this->ui->cb_manual, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), this, &MainWindow::process_watchdog_timer);

    this->display_cover_status();
    this->display_station_config();
    this->display_storage_status();
    this->display_ufo_state();
    this->display_window_title();
}

MainWindow::~MainWindow() {
    logger.info(Concern::Operation, "Terminating normally");

    delete this->ui;
    delete this->timer_display;
    delete this->timer_heartbeat;
    delete this->timer_watchdog;

    delete this->universe;
    delete this->station;
}

void MainWindow::message_clicked() {
}

void MainWindow::on_actionExit_triggered() {
    QApplication::quit();
}

void MainWindow::on_button_send_heartbeat_pressed() {
    this->heartbeat();
}

void MainWindow::set_storage(Storage *storage, QLineEdit *edit) {
    QString new_dir = QFileDialog::getExistingDirectory(
        this,
        QString("Select %1 storage directory").arg(storage->name()),
        storage->root_directory().path(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (new_dir == "") {
        logger.debug(Concern::Configuration, "Directory selection aborted");
        return;
    } else {
        storage->set_root_directory(new_dir);
        edit->setText(new_dir);
        this->display_storage_status();
        this->settings->setValue(QString("storage/%1").arg(storage->name()), new_dir);
    }
}

void MainWindow::on_bt_permanent_clicked() {
    this->set_storage(this->station->permanent_storage(), this->ui->le_permanent);
}

void MainWindow::on_cb_debug_stateChanged(int debug) {
    this->settings->setValue("debug", bool(debug));

    logger.set_level(debug ? Level::Debug : Level::Info);
    logger.warning(Concern::Operation, QString("Logging of debug information %1").arg(debug ? "ON" : "OFF"));
}

void MainWindow::on_cb_manual_stateChanged(int enable) {
    if (enable) {
        logger.warning(Concern::Operation, "Switched to manual mode");
    } else {
        logger.warning(Concern::Operation, "Switched to automatic mode");
    }

    this->station->set_manual_control((bool) enable);

    this->settings->setValue("manual", this->station->is_manual());

    this->ui->cb_safety_override->setEnabled(this->station->is_manual());
    this->ui->cb_safety_override->setCheckState(Qt::CheckState::Unchecked);

    this->display_device_buttons_state();
    this->display_cover_status();
    this->display_window_title();
}

void MainWindow::on_cb_safety_override_stateChanged(int state) {
    if (state == Qt::CheckState::Checked) {
        QMessageBox box(
                        QMessageBox::Icon::Warning,
                        "Safety mechanism override",
                        "You are about to override the safety mechanisms!",
                        QMessageBox::Ok | QMessageBox::Cancel,
                        this
                    );
        box.setInformativeText("Turning on the image intensifier during the day with open cover may result in permanent damage.");
        box.setWindowIcon(QIcon(":/images/blue.ico"));
        int choice = box.exec();

        switch (choice) {
            case QMessageBox::Ok:
                this->station->set_safety_override(true);
                break;
            case QMessageBox::Cancel:
            default:
                this->ui->cb_safety_override->setCheckState(Qt::CheckState::Unchecked);
                break;
        }
    } else {
        this->station->set_safety_override(false);
    }
    this->display_window_title();
}

void MainWindow::on_bt_lens_heating_clicked(void) {
    if (this->station->dome()->state_S().lens_heating_active()) {
        logger.info(Concern::Operation, "Manual command: turn off the hotwire");
        this->station->turn_off_hotwire();
    } else {
        logger.info(Concern::Operation, "Manual command: turn on the hotwire");
        this->station->turn_on_hotwire();
    }
}

void MainWindow::on_bt_intensifier_clicked(void) {
    if (this->station->dome()->state_S().intensifier_active()) {
        this->station->turn_off_intensifier();
        logger.info(Concern::Operation, "Manual command: turn off the intensifier");
    } else {
        this->station->turn_on_intensifier();
        logger.info(Concern::Operation, "Manual command: turn on the intensifier");
    }
}

void MainWindow::on_bt_fan_clicked(void) {
    if (this->station->dome()->state_S().fan_active()) {
        this->station->turn_off_fan();
        logger.info(Concern::Operation, "Manual command: turn off the fan");
    } else {
        this->station->turn_on_fan();
        logger.info(Concern::Operation, "Manual command: turn on the fan");
    }
}

void MainWindow::on_bt_cover_open_clicked(void) {
    logger.info(Concern::Operation, "Manual command: open the cover");
    this->station->open_cover();
}

void MainWindow::on_bt_cover_close_clicked(void) {
    logger.info(Concern::Operation, "Manual command: close the cover");
    this->station->close_cover();
}

void MainWindow::on_actionManual_control_triggered() {
    this->ui->cb_manual->click();
}

void MainWindow::on_bt_change_ufo_clicked(void) {
    QString filename = QFileDialog::getOpenFileName(
        this,
        "Select UFO executable",
        QString(),
        "Executable file (*.exe)"
    );

    if (filename == "") {
        logger.debug(Concern::UFO, "Directory selection aborted");
        return;
    } else {
        if (filename == this->station->ufo_manager()->path()) {
            logger.debug(Concern::UFO, "Path not changed");
        } else {
            logger.debug(Concern::UFO, "Path changed");
            this->settings->setValue("ufo/path", filename);
            this->station->ufo_manager()->set_path(filename);
            this->display_ufo_settings();
        }
    }
}

void MainWindow::on_cb_ufo_auto_stateChanged(int enable) {
    this->station->ufo_manager()->set_autostart((bool) enable);
    this->settings->setValue("ufo/autostart", this->station->ufo_manager()->autostart());
}

void MainWindow::on_bt_ufo_clicked() {
    if (this->station->ufo_manager()->is_running()) {
        this->station->ufo_manager()->start_ufo();
    } else {
        this->station->ufo_manager()->stop_ufo();
    }
}

void MainWindow::on_co_serial_ports_activated(int index) {
    QString port = this->serial_ports[index].portName();

    if (this->ui->co_serial_ports->count() == 0) {
        logger.warning(Concern::SerialPort, "No serial ports found");
        this->station->dome()->clear_serial_port();
    } else {
        if (index == -1) {
            logger.warning(Concern::SerialPort, "Index is -1");
            this->station->dome()->clear_serial_port();
        } else {
            logger.info(Concern::SerialPort, QString("Serial port set to %1").arg(port));
            this->station->dome()->set_serial_port(port);
            this->settings->setValue("dome/port", port);
        }
    }

}

void MainWindow::on_bt_watchdir_change_clicked() {
    QString new_dir = QFileDialog::getExistingDirectory(
        this,
        "Select UFO output directory to watch",
        this->station->scanner()->directory().path(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (new_dir == "") {
        logger.debug(Concern::Storage, "Watch directory selection aborted");
    } else {
        this->station->set_scanner(new_dir);
        this->ui->le_primary->setText(new_dir);
        this->display_storage_status();
        this->settings->setValue(QString("storage/watch"), new_dir);
        logger.info(Concern::Storage, QString("Watch directory set to %1").arg(new_dir));
    }
}

void MainWindow::on_bt_watchdir_open_clicked() {
    QDesktopServices::openUrl(this->station->scanner()->directory().path());
}

void MainWindow::on_bt_permanent_open_clicked() {
    QDesktopServices::openUrl(this->station->primary_storage()->current_directory().path());
}
