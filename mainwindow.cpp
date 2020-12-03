#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "include.h"

#include <random>
#include <chrono>

#include <QTimer>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSerialPortInfo>
#include <QMessageLogger>


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

void MainWindow::closeEvent(QCloseEvent *event) {
    if (this->tray_icon->isVisible()) {
        this->hide();
       // event->ignore();
    }
}
void MainWindow::create_tray_icon() {
    this->trayIconMenu = new QMenu(this);
    this->trayIconMenu->addAction(minimizeAction);
    this->trayIconMenu->addAction(maximizeAction);
    this->trayIconMenu->addAction(restoreAction);
    this->trayIconMenu->addSeparator();
    this->trayIconMenu->addAction(quitAction);

    this->tray_icon = new QSystemTrayIcon(this);
    this->tray_icon->setContextMenu(trayIconMenu);
}

void MainWindow::create_actions() {
    minimizeAction = new QAction("Mi&nimize", this);
    connect(minimizeAction, &QAction::triggered, this, &QWidget::hide);

    maximizeAction = new QAction("Ma&ximize", this);
    connect(maximizeAction, &QAction::triggered, this, &QWidget::showMaximized);

    restoreAction = new QAction("&Restore", this);
    connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    quitAction = new QAction("&Quit", this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void MainWindow::set_icon(const StationState &state) {
    QIcon icon;
    QString tooltip;
    switch (state) {
        case StationState::MANUAL:
            icon = QIcon(":/images/green.ico");
            tooltip = "Manual control enabled";
            break;
        case StationState::DAY:
            icon = QIcon(":/images/yellow.ico");
            tooltip = "Day, not observing";
            break;
        case StationState::OBSERVING:
            icon = QIcon(":/images/blue.ico");
            tooltip = "Observation in progress";
            break;
        case StationState::NOT_OBSERVING:
            icon = QIcon(":/images/grey.ico");
            tooltip = "Not observing";
            break;
        case StationState::DOME_UNREACHABLE:
            icon = QIcon(":/images/red.ico");
            tooltip = "Dome is not responding";
            break;
    }

    this->tray_icon->setIcon(icon);
    this->tray_icon->setToolTip(QString("AMOS controller\n%1").arg(tooltip));
}

void MainWindow::icon_activated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
        case QSystemTrayIcon::DoubleClick:
            if (this->isVisible()) {
                this->hide();
            } else {
                this->show();
                this->activateWindow();
            }
            break;
        case QSystemTrayIcon::MiddleClick:
            break;
        default:
            ;
    }
}

void MainWindow::show_message(void) {
    switch (this->station->state()) {
        case StationState::DAY:
            this->tray_icon->showMessage("AMOS controller", "AMOS is working", QIcon(":/images/images/blue.ico"), 5000);
            break;
        default:
            break;
    }
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

void MainWindow::display_time(void) {
    statusBar()->showMessage(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    ui->label_heartbeat->setText(QString("%1 s").arg((double) this->timer_heartbeat->remainingTime() / 1000, 3, 'f', 1));
}

void MainWindow::display_serial_port_info(void) {
    this->ui->lb_serial_port->setText(this->station->dome()->serial_port_info());
    this->ui->lb_serial_data->setText(this->station->dome()->state_S().is_valid() ? "valid data" : "no data");
}

void MainWindow::display_sun_properties(void) {
    auto hor = this->station->sun_position();
    this->ui->lb_hor_altitude->setText(QString("%1°").arg(hor.theta * Deg, 3, 'f', 3));
    this->ui->lb_hor_altitude->setStyleSheet(QString("QLabel { color: %1; }").arg(Universe::altitude_colour(hor.theta * Deg)));
    this->ui->lb_hor_azimuth->setText(QString("%1°").arg(fmod(hor.phi * Deg + 360.0, 360.0), 3, 'f', 3));

    if (hor.theta * Deg > 0) {
        this->ui->lb_sun_status->setText("day");
        this->ui->lb_sun_status->setToolTip("Sun is above the horizon");
        this->ui->lb_sun_status->setStyleSheet(QString("QLabel { color: %1; }").arg(Universe::altitude_colour(hor.theta * Deg)));
    } else {
        if (this->station->is_dark()) {
            this->ui->lb_sun_status->setText("dark enough");
            this->ui->lb_sun_status->setToolTip("Sun is below the horizon and below the darkness limit");
            this->ui->lb_sun_status->setStyleSheet("QLabel { color: black; }");
        } else {
            this->ui->lb_sun_status->setText("too much light");
            this->ui->lb_sun_status->setToolTip("Sun is below the horizon, but above the darkness limit");
            this->ui->lb_sun_status->setStyleSheet("QLabel { color: blue; }");
        }
    }

    auto equ = Universe::compute_sun_equ();
    this->ui->lb_eq_latitude->setText(QString("%1°").arg(equ[theta] * Deg, 3, 'f', 3));
    this->ui->lb_eq_longitude->setText(QString("%1°").arg(equ[phi] * Deg, 3, 'f', 3));

    auto ecl = Universe::compute_sun_ecl();
    this->ui->lb_ecl_longitude->setText(QString("%1°").arg(ecl[phi] * Deg, 3, 'f', 3));
}

void MainWindow::display_basic_data(void) {
    const DomeStateS &state = this->station->dome()->state_S();

    if (state.is_valid()) {
        unsigned int time_alive = state.time_alive();
        unsigned int days = time_alive / 86400;
        unsigned int hours = (time_alive % 86400) / 3600;
        unsigned int minutes = (time_alive % 3600) / 60;
        unsigned int seconds = time_alive % 60;
        this->ui->lb_time_alive->setText(QString("%1d %2:%3:%4")
                                         .arg(days)
                                         .arg(hours, 2, 10, QChar('0'))
                                         .arg(minutes, 2, 10, QChar('0'))
                                         .arg(seconds, 2, 10, QChar('0')));

        this->ui->lb_dome_moving->setText(state.servo_moving() ? "moving" : "not moving");
        this->ui->lb_dome_cover_direction->setText(state.servo_direction() ? "opening" : "closing");
        this->ui->lb_dome_open_sensor->setText(state.dome_open_sensor_active() ? "active" : "not active");
        this->ui->lb_dome_closed_sensor->setText(state.dome_closed_sensor_active() ? "active" : "not active");

        this->ui->bt_cover_open->setEnabled(!state.dome_open_sensor_active() && this->station->is_manual());
        this->ui->bt_cover_close->setEnabled(!state.dome_closed_sensor_active() && this->station->is_manual());

        this->ui->lb_dome_safety_position->setText(state.cover_safety_position() ? "yes" : "no");
        this->ui->lb_dome_blocked->setText(state.servo_blocked() ? "yes" : "no");

        this->ui->lb_lens_heating->setText(state.lens_heating_active() ? "on" : "off");
        this->ui->bt_lens_heating->setText(state.lens_heating_active() ? "turn off" : "turn on");

        this->ui->lb_camera_heating->setText(state.camera_heating_active() ? "on" : "off");
        this->ui->bt_camera_heating->setText(state.camera_heating_active() ? "turn off" : "turn on");

        this->ui->lb_fan->setText(state.fan_active() ? "on" : "off");
        this->ui->bt_fan->setText(state.fan_active() ? "turn off" : "turn on");

        if (state.intensifier_active()) {
            this->ui->lb_intensifier->setText("on");
            this->ui->lb_intensifier->setStyleSheet("QLabel { color: red; }");
            this->ui->bt_intensifier->setText("turn off");
        } else {
            this->ui->lb_intensifier->setText("off");
            this->ui->lb_intensifier->setStyleSheet("QLabel { color: black; }");
            this->ui->bt_intensifier->setText("turn on");
        }

        if (state.rain_sensor_active()) {
            this->ui->lb_rain_sensor->setText("raining");
            this->ui->lb_rain_sensor->setStyleSheet("QLabel { color: blue; }");
        } else {
            this->ui->lb_rain_sensor->setText("not raining");
            this->ui->lb_rain_sensor->setStyleSheet("QLabel { color: black; }");
        }

        if (state.light_sensor_active()) {
            this->ui->lb_light_sensor->setText("light");
            this->ui->lb_light_sensor->setStyleSheet("QLabel { color: red; }");
        } else {
            this->ui->lb_light_sensor->setText("dark");
            this->ui->lb_light_sensor->setStyleSheet("QLabel { color: black; }");
        }

        this->ui->lb_master_power->setText(state.computer_power_sensor_active() ? "powered" : "not powered");
    } else {
        this->ui->lb_time_alive->setText("???");

        this->ui->lb_dome_moving->setText("?");
        this->ui->lb_dome_cover_direction->setText("?");
        this->ui->lb_dome_open_sensor->setText("?");
        this->ui->lb_dome_closed_sensor->setText("?");

        this->ui->bt_cover_open->setEnabled(false);
        this->ui->bt_cover_close->setEnabled(false);

        this->ui->lb_dome_safety_position->setText("?");
        this->ui->lb_dome_blocked->setText("?");

        this->ui->lb_lens_heating->setText("?");
        this->ui->bt_lens_heating->setText("no connection");

        this->ui->lb_camera_heating->setText("?");
        this->ui->bt_camera_heating->setText("no connection");

        this->ui->lb_fan->setText("?");
        this->ui->bt_fan->setText("no connection");

        this->ui->lb_intensifier->setText("?");
        this->ui->lb_intensifier->setStyleSheet("QLabel { color: red; }");
        this->ui->bt_intensifier->setText("no connection");

        this->ui->lb_rain_sensor->setText("?");
        this->ui->lb_rain_sensor->setStyleSheet("QLabel { color: red; }");

        this->ui->lb_light_sensor->setText("?");
        this->ui->lb_light_sensor->setStyleSheet("QLabel { color: red; }");

        this->ui->lb_master_power->setText("?");
    }
}

void MainWindow::display_env_data(void) {
    const DomeStateT &state = this->station->dome()->state_T();
    this->ui->group_environment->setTitle(QString("Environment (%1 s)").arg(state.age(), 3, 'f', 1));
    this->ui->lb_t_lens->setText(state.is_valid() ? QString("%1 °C").arg(state.temperature_lens(), 4, 'f', 1) : "? °C");
    this->ui->lb_t_cpu->setText(state.is_valid() ? QString("%1 °C").arg(state.temperature_cpu(), 4, 'f', 1) : "? °C");
    this->ui->lb_t_sht->setText(state.is_valid() ? QString("%1 °C").arg(state.temperature_sht(), 4, 'f', 1) : "? °C");
    this->ui->lb_h_sht->setText(state.is_valid() ? QString("%1 %").arg(state.humidity_sht(), 4, 'f', 1) : "? %");
}

void MainWindow::display_shaft_position(void) {
    const DomeStateZ &state = this->station->dome()->state_Z();
    this->ui->progress_cover->setValue(state.shaft_position());
}

void MainWindow::display_storage_status(const Storage& storage, QProgressBar *pb, QLineEdit *le) {
    QStorageInfo info = storage.info();
    unsigned int total = (int) ((double) info.bytesTotal() / (1 << 30));
    unsigned int used = (int) ((double) info.bytesAvailable() / (1 << 30));
    pb->setMaximum(total);
    pb->setValue(total - used);
    le->setText(storage.get_directory().path());
}

void MainWindow::display_storage_status(void) {
    this->display_storage_status(this->station->primary_storage(), this->ui->pb_primary, this->ui->le_primary);
    this->display_storage_status(this->station->permanent_storage(), this->ui->pb_permanent, this->ui->le_permanent);
}

void MainWindow::display_station_config(void) {
    this->ui->le_ip->setText(this->station->server()->address().toString());
    this->ui->sb_port->setValue(this->station->server()->port());
    this->ui->le_station_id->setText(this->station->get_id());

    this->ui->dsb_latitude->setValue(this->station->latitude());
    this->ui->dsb_longitude->setValue(this->station->longitude());
    this->ui->dsb_altitude->setValue(this->station->altitude());

    this->ui->dsb_darkness_limit->setValue(this->station->darkness_limit());
    this->ui->dsb_humidity_limit_lower->setValue(this->station->humidity_limit_lower());
    this->ui->dsb_humidity_limit_upper->setValue(this->station->humidity_limit_upper());
}

void MainWindow::display_ufo_state(void) {
    this->ui->le_ufo_path->setText(this->ufo->path());
    this->ui->lb_ufo_state->setText(this->ufo->state_string());
}

void MainWindow::send_heartbeat(void) {
    this->station->log_state();
    this->display_storage_status();
    this->station->send_heartbeat();
}

void MainWindow::display_cover_status(void) {
    // Set cover state label
    const DomeStateS &stateS = this->station->dome()->state_S();
    QString text;

    if (stateS.is_valid()) {
        if (stateS.servo_moving()) {
            text = stateS.servo_direction() ? "opening..." : "closing";
        } else {
            text = stateS.cover_safety_position() ? "peeking" :
                 stateS.dome_open_sensor_active() ? "open" :
                 stateS.dome_closed_sensor_active() ? "closed" : "inconsistent!";
        }
    } else {
        text = "???";
    }
    this->ui->label_cover_state->setText(text);

    // Set cover shaft position
    const DomeStateZ &stateZ = this->station->dome()->state_Z();
    if (stateZ.is_valid()) {
        this->ui->progress_cover->setEnabled(true);
        this->ui->progress_cover->setValue(stateZ.shaft_position());
    } else {
        this->ui->progress_cover->setEnabled(false);
        this->ui->progress_cover->setValue(0);
    }
}

void MainWindow::on_button_send_heartbeat_pressed() {
    this->send_heartbeat();
}

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
    if (enable) {
        this->ui->bt_station_apply->setText("Apply changes");
        this->ui->bt_station_apply->setEnabled(true);
    } else {
        this->ui->bt_station_apply->setText("No changes");
        this->ui->bt_station_apply->setEnabled(false);
    }
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
        logger.info(QString("Serial port changed to %1").arg(this->serial_ports[index].portName()));
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
    logger.info(QString("State changed: %1").arg(state));
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
