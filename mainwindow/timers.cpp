#include "include.h"
#include "ui_mainwindow.h"

void MainWindow::create_timers(void) {
    this->timer_display = new QTimer(this);
    this->timer_display->setInterval(100);
    this->connect(this->timer_display, &QTimer::timeout, this, &MainWindow::process_display_timer);
    this->timer_display->start();
}

void MainWindow::process_display_timer(void) {
    this->display_time();
}
