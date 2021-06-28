#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "settings.h"
#include "include.h"

#include "logging/loggingdialog.h"
#include "widgets/aboutdialog.h"

extern EventLogger logger;
extern QSettings * settings;

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_start_time(QDateTime::currentDateTimeUtc()),
    m_terminate(false)
{
    this->ui->setupUi(this);

    this->ui->tb_log->setColumnWidth(0, 140);
    this->ui->tb_log->setColumnWidth(1, 72);
    this->ui->tb_log->setColumnWidth(2, 80);

    logger.set_display_widget(this->ui->tb_log);
    logger.info(Concern::Operation, "------------ Initializing AMOS client ------------");

    // connect signals for handling of edits of station position
    settings = new QSettings(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/settings.ini", QSettings::IniFormat, this);
    settings->setValue("run/last_run", this->m_start_time);
    this->load_settings();

    this->ui->dome->initialize(this->ui->station);
    this->ui->server->initialize();
    this->ui->station->initialize();
    this->ui->camera_allsky->initialize("allsky");
    this->ui->camera_spectral->initialize("spectral");

    this->ui->sun_info->set_station(this->ui->station);
    this->ui->sun_info->set_allsky_camera(this->ui->camera_allsky);

    this->create_timers();
    this->create_actions();
    this->create_tray_icon();
    this->icons = {
        {Icon::Daylight, QIcon(":/images/yellow.ico")},
        {Icon::Failure, QIcon(":/images/red.ico")},
        {Icon::Manual, QIcon(":/images/green.ico")},
        {Icon::Observing, QIcon(":/images/blue.ico")},
        {Icon::NotObserving, QIcon(":/images/grey.ico")},
    };
    this->set_icon(QStation::Manual);
    this->tray_icon->show();

    this->connect(this->tray_icon, &QSystemTrayIcon::activated, this, &MainWindow::icon_activated);

    this->connect(this->ui->station, &QStation::manual_mode_changed, this, &MainWindow::display_window_title);
    this->connect(this->ui->station, &QStation::manual_mode_changed, this->ui->action_manual, &QAction::setChecked);
    this->connect(this->ui->station, &QStation::safety_override_changed, this, &MainWindow::display_window_title);
    this->connect(this->ui->station, &QStation::state_changed, this, &MainWindow::show_message);
    this->connect(this->ui->station, &QStation::state_changed, this->ui->dome, &QDome::display_serial_port_info);
    this->connect(this->ui->station, &QStation::state_changed, this, &MainWindow::set_icon);

    this->connect(this->ui->station, &QStation::position_changed, this->ui->sun_info, &QSunInfo::update_long_term);

    this->connect(this->ui->camera_allsky, &QCamera::sightings_found, this->ui->server, &QServer::send_sightings);

    this->connect(qApp, &QApplication::commitDataRequest, this, &MainWindow::set_terminate);

    this->ui->station->send_heartbeat();

    this->set_icon(this->ui->station->state());
    this->display_window_title();
    this->ui->sun_info->update_short_term();
    this->ui->sun_info->update_long_term();
#if OLD_PROTOCOL
//    this->ui->dome->set_cover_minimum(-26);
//    this->ui->dome->set_cover_maximum(26);
#endif
    logger.info(Concern::Operation, "Initialization complete");
}

MainWindow::~MainWindow() {
    logger.info(Concern::Operation, "Terminating normally");

    delete this->ui;
    delete this->timer_display;
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

void MainWindow::on_cb_debug_stateChanged(int debug) {
    settings->setValue("debug", bool(debug));

    logger.set_level(debug ? Level::Debug : Level::Info);
    logger.warning(Concern::Operation, QString("Logging of debug information %1").arg(debug ? "ON" : "OFF"));

    this->ui->action_debug->setChecked(debug);
}

/**
 * @brief MainWindow::set_terminate
 * Once called, the application is scheduled for termination and will not ask for exit confirmation
 * Primarily used to prevent blocking system shutdown
 */
void MainWindow::set_terminate(void) {
    this->m_terminate = true;
}

void MainWindow::on_action_manual_triggered() {
    this->ui->station->set_manual_control(!this->ui->station->is_manual());
}

void MainWindow::on_action_logging_options_triggered() {
    LoggingDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_action_exit_triggered() {
    QApplication::quit();
}

void MainWindow::on_action_open_log_triggered() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(logger.filename()));
}

void MainWindow::on_action_open_stat_triggered() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(this->ui->station->state_logger_filename()));
}

void MainWindow::on_action_open_config_triggered() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(settings->fileName()));
}

void MainWindow::on_action_debug_triggered() {
    this->ui->cb_debug->click();
}

void MainWindow::on_actionAbout_triggered() {
    AboutDialog dialog(this);
    dialog.exec();
}
