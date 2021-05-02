#include "include.h"
#include "ui_mainwindow.h"

void MainWindow::create_timers(void) {
    this->timer_heartbeat = new QTimer(this);
    this->timer_heartbeat->setInterval(60 * 1000);
    this->connect(this->timer_heartbeat, &QTimer::timeout, this, &MainWindow::heartbeat);
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
}

void MainWindow::process_watchdog_timer(void) {
    this->display_ufo_state();
}

void MainWindow::heartbeat(void) {
    this->station->log_state();
    this->station->send_heartbeat();
}
