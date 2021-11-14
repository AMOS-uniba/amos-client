#include "ui_mainwindow.h"

void MainWindow::create_timers(void) {
    this->m_timer_display = new QTimer(this);
    this->m_timer_display->setInterval(100);
    this->connect(this->m_timer_display, &QTimer::timeout, this, &MainWindow::process_display_timer);
    this->m_timer_display->start();

    this->m_timer_long = new QTimer(this);
    this->m_timer_long->setInterval(300000);
    this->connect(this->m_timer_long, &QTimer::timeout, this->ui->camera_allsky, &QCamera::update_clocks);
    this->connect(this->m_timer_long, &QTimer::timeout, this->ui->camera_spectral, &QCamera::update_clocks);
    this->m_timer_long->start();
}

void MainWindow::process_display_timer(void) {
    this->display_time();
}
