#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "logging/loggingdialog.h"
#include "widgets/qaboutdialog.h"
#include "models/qsightingmodel.h"

extern EventLogger logger;
extern QSettings * settings;

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_terminate(false)
{
    this->ui->setupUi(this);

    this->ui->tb_log->setColumnWidth(0, 140);
    this->ui->tb_log->setColumnWidth(1, 72);
    this->ui->tb_log->setColumnWidth(2, 80);

    logger.set_display_widget(this->ui->tb_log);
    logger.info(Concern::Operation, QString("------------ Initializing AMOS client %1 ------------").arg(VERSION_STRING));

    // connect signals for handling of edits of station position
    settings = new QSettings("./settings.ini", QSettings::IniFormat, this);
    settings->setValue("run/last_run", this->ui->station->start_time());
    this->load_settings();

    this->amos_widgets = {this->ui->dome, this->ui->server, this->ui->station, this->ui->camera_allsky, this->ui->camera_spectral};

    this->ui->dome->initialize(settings);
    this->ui->server->initialize(settings);
    this->ui->station->initialize(settings);
    this->ui->camera_allsky->initialize(settings, "allsky", this->ui->station, false);
    this->ui->camera_spectral->initialize(settings, "spectral", this->ui->station, true);

    this->ui->dome->set_station(this->ui->station);
    this->ui->sun_info->set_station(this->ui->station);

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
    this->set_icon(QStation::NotObserving);
    this->tray_icon->show();

    this->connect(this->tray_icon, &QSystemTrayIcon::activated, this, &MainWindow::icon_activated);

    this->connect(this->ui->station, &QStation::manual_mode_changed, this, &MainWindow::display_window_title);
    this->connect(this->ui->station, &QStation::manual_mode_changed, this->ui->action_manual, &QAction::setChecked);
    this->connect(this->ui->station, &QStation::safety_override_changed, this, &MainWindow::display_window_title);
    this->connect(this->ui->station, &QStation::state_changed, this, &MainWindow::show_message);
    this->connect(this->ui->station, &QStation::state_changed, this, &MainWindow::set_icon);

    this->connect(this->ui->station, &QStation::automatic_action_allsky, this->ui->station, &QStation::automatic_cover);
    this->connect(this->ui->station, &QStation::automatic_action_allsky, this->ui->camera_allsky, &QCamera::auto_action);
    this->connect(this->ui->station, &QStation::automatic_action_spectral, this->ui->camera_spectral, &QCamera::auto_action);
    this->connect(this->ui->station, &QStation::position_changed, this->ui->sun_info, &QSunInfo::update_long_term);
    this->connect(this->ui->station, &QStation::position_changed, this->ui->camera_allsky, &QCamera::update_clocks);
    this->connect(this->ui->station, &QStation::position_changed, this->ui->camera_spectral, &QCamera::update_clocks);

    auto model = this->ui->sb_sightings->model();
    this->connect(this->ui->camera_allsky,   &QCamera::sighting_found,      model, &QSightingModel::insert_sighting);
    this->connect(this->ui->camera_spectral, &QCamera::sighting_found,      model, &QSightingModel::insert_sighting);
    this->connect(this->ui->camera_allsky,   &QCamera::sighting_stored,     model, &QSightingModel::mark_stored);
    this->connect(this->ui->camera_spectral, &QCamera::sighting_stored,     model, &QSightingModel::mark_stored);
    this->connect(this->ui->camera_allsky,   &QCamera::sighting_discarded,  model, &QSightingModel::mark_discarded);
    this->connect(this->ui->camera_spectral, &QCamera::sighting_discarded,  model, &QSightingModel::mark_discarded);
    this->connect(model, &QSightingModel::sighting_to_send, this->ui->server, &QServer::send_sighting);
    this->connect(this->ui->server, &QServer::sighting_sent,     model, &QSightingModel::mark_sent);
    this->connect(this->ui->server, &QServer::sighting_accepted, model, &QSightingModel::store_sighting);
    this->connect(this->ui->server, &QServer::sighting_conflict, model, &QSightingModel::discard_sighting);
    this->connect(this->ui->server, &QServer::sighting_error,    model, &QSightingModel::defer_sighting);
    this->connect(model, &QSightingModel::sighting_accepted, this->ui->camera_allsky,   &QCamera::store_sighting);
    this->connect(model, &QSightingModel::sighting_accepted, this->ui->camera_spectral, &QCamera::store_sighting);
    this->connect(model, &QSightingModel::sighting_rejected, this->ui->camera_allsky,   &QCamera::discard_sighting);
    this->connect(model, &QSightingModel::sighting_rejected, this->ui->camera_spectral, &QCamera::discard_sighting);

    this->connect(this->ui->dome, &QAmosWidget::settings_changed, this, &MainWindow::slot_settings_changed);
    this->connect(this->ui->station, &QAmosWidget::settings_changed, this, &MainWindow::slot_settings_changed);
    this->connect(this->ui->server, &QAmosWidget::settings_changed, this, &MainWindow::slot_settings_changed);
    this->connect(this->ui->camera_allsky, &QAmosWidget::settings_changed, this, &MainWindow::slot_settings_changed);
    this->connect(this->ui->camera_spectral, &QAmosWidget::settings_changed, this, &MainWindow::slot_settings_changed);

    this->connect(qApp, &QApplication::commitDataRequest, this, &MainWindow::set_terminate);

    this->on_bt_discard_clicked();
    this->slot_settings_changed();

    this->ui->station->send_heartbeat();

    this->set_icon(this->ui->station->state());
    this->display_window_title();
    this->ui->sun_info->update_short_term();
    this->ui->sun_info->update_long_term();
    this->ui->camera_allsky->update_clocks();
    this->ui->camera_spectral->update_clocks();
#if OLD_PROTOCOL
//    this->ui->dome->set_cover_minimum(-26);
//    this->ui->dome->set_cover_maximum(26);
#endif
    logger.info(Concern::Operation, "Initialization complete");
}

MainWindow::~MainWindow() {
    logger.info(Concern::Operation, "Terminating normally");

    delete this->ui;
    delete this->m_timer_display;
    delete settings;
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

void MainWindow::on_action_about_triggered() {
    QAboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_pb_logging_options_clicked() {
    this->on_action_logging_options_triggered();
}

