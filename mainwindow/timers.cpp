#include "include.h"
#include "ui_mainwindow.h"

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

    this->set_icon(this->station->determine_state());
    this->display_ufo_state();
}

void MainWindow::send_heartbeat(void) {
    this->station->log_state();
    this->station->send_heartbeat();
}
