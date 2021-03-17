#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "include.h"

#include "logging/loggingdialog.h"

extern EventLogger logger;
extern QSettings *settings;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {
    this->ui->setupUi(this);
    this->ui->storage_primary->set_name("primary");
    this->ui->storage_permanent->set_name("permanent");

    this->universe = new Universe();

    this->ui->tb_log->setColumnWidth(0, 140);
    this->ui->tb_log->setColumnWidth(1, 72);
    this->ui->tb_log->setColumnWidth(2, 72);

    logger.set_display_widget(this->ui->tb_log);
    logger.info(Concern::Operation, "Client initialized");

    // connect signals for handling of edits of station position
    settings = new QSettings(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/settings.ini", QSettings::IniFormat, this);
    settings->setValue("run/last_run", QDateTime::currentDateTimeUtc());
    this->load_settings();

    this->ui->sun_info->set_station(this->station);

    this->create_timers();

    this->connect(this->ui->dsb_latitude, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::slot_station_edited);
    this->connect(this->ui->dsb_longitude, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::slot_station_edited);
    this->connect(this->ui->dsb_altitude, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::slot_station_edited);

    // connect signals for handling of edits of server address
    this->connect(this->ui->le_station_id, QOverload<const QString&>::of(&QLineEdit::textChanged), this, &MainWindow::slot_station_edited);
    this->connect(this->ui->le_ip, QOverload<const QString&>::of(&QLineEdit::textChanged), this, &MainWindow::slot_station_edited);
    this->connect(this->ui->sb_port, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::slot_station_edited);

    // connect signals for handling of edits of safety limits
    this->connect(this->ui->dsb_darkness_limit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::slot_station_edited);

    this->create_actions();
    this->create_tray_icon();
    this->icons = {
        {Icon::Daylight, QIcon(":/images/yellow.ico")},
        {Icon::Failure, QIcon(":/images/red.ico")},
        {Icon::Manual, QIcon(":/images/green.ico")},
        {Icon::Observing, QIcon(":/images/blue.ico")},
        {Icon::NotObserving, QIcon(":/images/grey.ico")},
    };
    this->set_icon(Station::Manual);
    this->tray_icon->show();

    this->connect(this->tray_icon, &QSystemTrayIcon::messageClicked, this, &MainWindow::message_clicked);
    this->connect(this->tray_icon, &QSystemTrayIcon::activated, this, &MainWindow::icon_activated);

    this->connect(this->station, &Station::state_changed, this, &MainWindow::show_message);
    this->connect(this->ui->cb_manual, QOverload<int>::of(&QCheckBox::stateChanged), this, &MainWindow::process_watchdog_timer);

    this->display_cover_status();
    this->display_station_config();
    this->display_ufo_state();
    this->display_window_title();

    this->connect(this->station, &Station::state_changed, this, &MainWindow::set_icon);
    this->set_icon(this->station->state());

    this->connect(this->ui->scanner, &QScannerBox::sightings_found, this->station, &Station::process_sightings);

    this->connect(this->station, &Station::humidity_limits_changed, this->ui->dome_info, &QDome::set_formatters);

    this->ui->dome_info->list_serial_ports();

    this->ui->sun_info->update_short_term();
    this->ui->sun_info->update_long_term();
    this->ui->dome_info->initialize(this->station);
#ifdef OLD_PROTOCOL
    this->ui->progress_cover->setMaximum(26);
    this->ui->dome_widget->set_cover_maximum(26);
#endif
}

MainWindow::~MainWindow() {
    logger.info(Concern::Operation, "Terminating normally");

    delete this->ui;
    delete this->timer_display;
    delete this->timer_heartbeat;
    delete this->timer_watchdog;

    delete this->universe;
    delete this->station;
    delete settings;
}

QString MainWindow::format_duration(unsigned int duration) {
    unsigned int days = duration / 86400;
    unsigned int hours = (duration % 86400) / 3600;
    unsigned int minutes = (duration % 3600) / 60;
    unsigned int seconds = duration % 60;

    return QString("%1d %2:%3:%4")
        .arg(days)
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

void MainWindow::message_clicked() {
}

void MainWindow::on_action_exit_triggered() {
    QApplication::quit();
}

void MainWindow::on_button_send_heartbeat_pressed() {
    this->heartbeat();
}

void MainWindow::on_cb_debug_stateChanged(int debug) {
    settings->setValue("debug", bool(debug));

    logger.set_level(debug ? Level::Debug : Level::Info);
    logger.warning(Concern::Operation, QString("Logging of debug information %1").arg(debug ? "ON" : "OFF"));

    this->ui->action_debug->setChecked(debug);
}

void MainWindow::on_cb_manual_stateChanged(int manual) {
    logger.warning(Concern::Operation, QString("Switched to %1 mode").arg(manual ? "manual" : "automatic"));

    this->station->set_manual_control((bool) manual);
    settings->setValue("manual", this->station->is_manual());

    this->ui->action_manual->setChecked(this->station->is_manual());
    this->ui->cb_safety_override->setEnabled(this->station->is_manual());
    this->ui->cb_safety_override->setCheckState(Qt::CheckState::Unchecked);

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
            settings->setValue("ufo/path", filename);
            this->station->ufo_manager()->set_path(filename);
            this->display_ufo_settings();
        }
    }
}

void MainWindow::on_cb_ufo_auto_stateChanged(int enable) {
    this->station->ufo_manager()->set_autostart((bool) enable);
    settings->setValue("ufo/autostart", this->station->ufo_manager()->autostart());
}

void MainWindow::on_bt_ufo_clicked() {
    if (this->station->ufo_manager()->is_running()) {
        this->station->ufo_manager()->start_ufo();
    } else {
        this->station->ufo_manager()->stop_ufo();
    }
}

void MainWindow::on_action_manual_triggered() {
    this->ui->cb_manual->click();
}

void MainWindow::on_action_logging_options_triggered() {
    LoggingDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_action_open_log_triggered() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(logger.filename()));
}

void MainWindow::on_action_open_stat_triggered() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(this->station->state_logger_filename()));
}

void MainWindow::on_action_open_config_triggered() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(settings->fileName()));
}

void MainWindow::on_action_debug_triggered() {
    this->ui->cb_debug->click();
}
