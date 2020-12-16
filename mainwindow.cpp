#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "include.h"


extern EventLogger logger;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {
    this->ui->setupUi(this);

    this->universe = new Universe();

    this->ui->tb_log->setColumnWidth(0, 140);
    this->ui->tb_log->setColumnWidth(1, 80);

    this->settings = new QSettings("settings.ini", QSettings::IniFormat, this);
    this->settings->setValue("run/last_run", QDateTime::currentDateTimeUtc());
    this->load_settings();

    this->create_timers();

    logger.set_display_widget(this->ui->tb_log);
    logger.info("Client initialized");

    // connect signals for handling of edits of station position
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

    this->init_serial_ports();
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

void MainWindow::init_serial_ports(void) {
    this->ui->co_serial_ports->clear();
    this->serial_ports = QSerialPortInfo::availablePorts();
    for (QSerialPortInfo sp: this->serial_ports) {
        this->ui->co_serial_ports->addItem(sp.portName());
    }
}

MainWindow::~MainWindow() {
    logger.info("Terminating normally");

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
    this->send_heartbeat();
}

void MainWindow::set_storage(Storage *storage, QLineEdit *edit) {
    QString new_dir = QFileDialog::getExistingDirectory(
        this,
        QString("Select %1 storage directory").arg(storage->get_name()),
        storage->get_directory().path(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (new_dir == "") {
        logger.debug("Directory selection aborted");
        return;
    } else {
        storage->set_directory(new_dir);
        edit->setText(new_dir);
        this->display_storage_status();
        this->settings->setValue(QString("storage/%1").arg(storage->get_name()), new_dir);
    }
}

void MainWindow::on_bt_primary_clicked() {
    this->set_storage(this->station->primary_storage(), this->ui->le_primary);
}

void MainWindow::on_bt_permanent_clicked() {
    this->set_storage(this->station->permanent_storage(), this->ui->le_permanent);
}

void MainWindow::on_cb_debug_stateChanged(int debug) {
    this->settings->setValue("debug", debug);

    logger.set_level(debug ? Level::Debug : Level::Info);
    logger.warning(QString("Logging of debug information %1").arg(debug ? "ON" : "OFF"));
}

void MainWindow::on_cb_manual_stateChanged(int enable) {
    if (enable) {
        logger.warning("Switched to manual mode");
    } else {
        logger.warning("Switched to automatic mode");
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
        QMessageBox box;
        box.setText("You are about to override the safety mechanisms!");
        box.setInformativeText("Turning on the image intensifier during the day with open cover may result in permanent damage.");
        box.setIcon(QMessageBox::Icon::Warning);
        box.setWindowIcon(QIcon(":/images/blue.ico"));
        box.setWindowTitle("Safety mechanism override");
        box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
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
    }
    this->display_window_title();
}

void MainWindow::on_co_serial_ports_currentIndexChanged(int index) {
    if (index == -1) {
        logger.warning("No serial ports found");
        this->station->dome()->clear_serial_port();
    } else {
        logger.info(QString("Serial port set to %1").arg(this->serial_ports[index].portName()));
        this->station->dome()->reset_serial_port(this->serial_ports[index].portName());
    }
}

void MainWindow::on_bt_lens_heating_clicked(void) {
    if (this->station->dome()->state_S().lens_heating_active()) {
        logger.info("Manual command: turn off the hotwire");
        this->station->turn_off_hotwire();
    } else {
        logger.info("Manual command: turn on the hotwire");
        this->station->turn_on_hotwire();
    }
}

void MainWindow::on_bt_intensifier_clicked(void) {
    if (this->station->dome()->state_S().intensifier_active()) {
        this->station->turn_off_intensifier();
        logger.info("Manual command: turn off the intensifier");
    } else {
        this->station->turn_on_intensifier();
        logger.info("Manual command: turn on the intensifier");
    }
}

void MainWindow::on_bt_fan_clicked(void) {
    if (this->station->dome()->state_S().fan_active()) {
        this->station->turn_off_fan();
        logger.info("Manual command: turn off the fan");
    } else {
        this->station->turn_on_fan();
        logger.info("Manual command: turn on the fan");
    }
}

void MainWindow::on_bt_cover_open_clicked(void) {
    logger.info("Manual command: open the cover");
    this->station->open_cover();
}

void MainWindow::on_bt_cover_close_clicked(void) {
    logger.info("Manual command: close the cover");
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
        logger.debug("Directory selection aborted");
        return;
    } else {
        this->settings->setValue("ufo/path", filename);
        this->station->ufo_manager()->set_path(filename);
        this->display_ufo_state();
    }
}

void MainWindow::on_checkBox_stateChanged(int enable) {

}
