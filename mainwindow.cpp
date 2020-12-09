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
    this->display_cover_status();
    this->display_station_config();
    this->display_storage_status();
    this->display_ufo_state();

    logger.set_display_widget(this->ui->tb_log);
    logger.info("Client initialized");

    // connect signals for handling of edits of station position
    this->connect(this->ui->dsb_latitude, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::station_edited);
    this->connect(this->ui->dsb_longitude, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::station_edited);
    this->connect(this->ui->dsb_altitude, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::station_edited);

    // connect signals for handling of edits of server address
    this->connect(this->ui->le_station_id, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &MainWindow::station_edited);
    this->connect(this->ui->le_ip, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &MainWindow::station_edited);
    this->connect(this->ui->sb_port, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::station_edited);

    // connect signals for handling of edits of safety limits
    this->connect(this->ui->dsb_darkness_limit, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::station_edited);
    this->connect(this->ui->dsb_humidity_limit_lower, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::station_edited);
    this->connect(this->ui->dsb_humidity_limit_upper, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::station_edited);

    this->init_serial_ports();
    this->create_actions();
    this->create_tray_icon();
    this->set_icon(StationState::NOT_OBSERVING);

    this->tray_icon->show();

    connect(this->tray_icon, &QSystemTrayIcon::messageClicked, this, &MainWindow::message_clicked);
    connect(this->tray_icon, &QSystemTrayIcon::activated, this, &MainWindow::icon_activated);

    /*connect(&this->comm_thread, &CommThread::response, this, [=](const QByteArray &m){ logger.info(m); });
    connect(&this->comm_thread, &CommThread::error, this, [=](const QString &m){ logger.error(m); });
    connect(&this->comm_thread, &CommThread::timeout, this, [=](const QString &m){ logger.warning(m); });*/

    this->connect(this->station->dome(), &Dome::state_updated_S, this, &MainWindow::display_basic_data);
    this->connect(this->station->dome(), &Dome::state_updated_T, this, &MainWindow::display_env_data);
    this->connect(this->station->dome(), &Dome::state_updated_Z, this, &MainWindow::display_shaft_position);

    this->connect(this->station, &Station::state_changed, this, &MainWindow::show_message);
    this->connect(this->ui->cb_manual, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), this, &MainWindow::process_watchdog_timer);
}

void MainWindow::init_serial_ports(void) {
    this->ui->co_serial_ports->clear();
    this->serial_ports = QSerialPortInfo::availablePorts();
    for (QSerialPortInfo sp: this->serial_ports) {
        this->ui->co_serial_ports->addItem(sp.portName());
    }
}

void MainWindow::load_settings(void) {
    try {
        QString ip = this->settings->value("server/ip", "127.0.0.1").toString();
        unsigned short port = this->settings->value("server/port", 4805).toInt();
        QString station_id = this->settings->value("station/id", "none").toString();

        this->ufo = new UfoManager();
        this->ufo->set_path(this->settings->value("ufo/path", "C:\\Program Files\\UFO\\UFO.exe").toString());

        this->station = new Station(station_id);
        this->station->set_server(new Server(QHostAddress(ip), port, station_id));
        this->station->set_storages(
            QDir(this->settings->value("storage/primary", "C:\\Data").toString()),
            QDir(this->settings->value("storage/permanent", "D:\\Data").toString())
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

void MainWindow::create_timers(void) {
    this->timer_heartbeat = new QTimer(this);
    this->timer_heartbeat->setInterval(60 * 1000);
    this->connect(this->timer_heartbeat, &QTimer::timeout, this, &MainWindow::send_heartbeat);
    this->timer_heartbeat->start();

    this->timer_display = new QTimer(this);
    this->timer_display->setInterval(100);
    this->connect(this->timer_display, &QTimer::timeout, this, &MainWindow::process_display_timer);
    this->timer_display->start();

    this->timer_watchdog = new QTimer(this);
    this->timer_watchdog->setInterval(1000);
    this->connect(this->timer_watchdog, &QTimer::timeout, this, &MainWindow::process_watchdog_timer);
    this->timer_watchdog->start();
}

MainWindow::~MainWindow() {
    logger.info("Terminating normally");

    delete this->ui;
    delete this->timer_display;
    delete this->timer_heartbeat;
    delete this->timer_watchdog;

    delete this->universe;
    delete this->station;
    delete this->ufo;
}

void MainWindow::message_clicked() {
}

void MainWindow::on_actionExit_triggered() {
    QApplication::quit();
}

void MainWindow::process_display_timer(void) {
    this->display_time();
    this->display_basic_data();
    this->display_env_data();
    this->display_cover_status();
    this->display_sun_properties();
    this->display_serial_port_info();
}

void MainWindow::process_watchdog_timer(void) {
    if (this->station->dome()->serial_port_info() != "open") {
        this->init_serial_ports();
    }

    this->setWindowTitle(QString("AMOS client [%1 mode]").arg(this->station->is_manual() ? "manual" : "automatic"));
    this->set_icon(this->station->determine_state());
    this->display_ufo_state();
}

void MainWindow::send_heartbeat(void) {
    this->station->log_state();
    this->station->send_heartbeat();
}

void MainWindow::on_button_send_heartbeat_pressed() {
    this->send_heartbeat();
}

void MainWindow::set_storage(Storage &storage, QLineEdit *edit) {
    QString new_dir = QFileDialog::getExistingDirectory(
        this,
        QString("Select %1 storage directory").arg(storage.get_name()),
        storage.get_directory().path(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (new_dir == "") {
        logger.debug("Directory selection aborted");
        return;
    } else {
        storage.set_directory(new_dir);
        edit->setText(new_dir);
        this->display_storage_status();
        this->settings->setValue(QString("storage/%1").arg(storage.get_name()), new_dir);
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
    this->ui->bt_camera_heating->setEnabled(this->station->is_manual());
    this->ui->bt_lens_heating->setEnabled(this->station->is_manual());
    this->ui->bt_intensifier->setEnabled(this->station->is_manual());
    this->ui->bt_fan->setEnabled(this->station->is_manual());

    this->ui->cb_safety_override->setEnabled(this->station->is_manual());
    this->ui->cb_safety_override->setCheckState(Qt::CheckState::Unchecked);

    this->display_cover_status();
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
        this->station->turn_off_hotwire();
    } else {
        this->station->turn_on_hotwire();
    }
}

void MainWindow::on_bt_intensifier_clicked(void) {
    if (this->station->dome()->state_S().intensifier_active()) {
        this->station->turn_off_intensifier();
    } else {
        this->station->turn_on_intensifier();
    }
}

void MainWindow::on_bt_fan_clicked(void) {
    if (this->station->dome()->state_S().fan_active()) {
        this->station->turn_off_fan();
    } else {
        this->station->turn_on_fan();
    }
}

void MainWindow::on_bt_cover_open_clicked(void) {
    logger.info("Manual command to open the cover");
    this->station->open_cover();
}

void MainWindow::on_bt_cover_close_clicked(void) {
    logger.info("Manual command to close the cover");
    this->station->close_cover();
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
        this->reset_ufo();
    }
}

void MainWindow::reset_ufo(void) {
    this->display_ufo_state();
}
