#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <random>
#include <chrono>

#include <QTimer>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageLogger>


MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    this->station = new Station();
    this->universe = new Universe();


    this->network_manager = new QNetworkAccessManager(this);
    connect(this->network_manager, &QNetworkAccessManager::finished, this, &MainWindow::heartbeat_ok);

    this->settings = new QSettings("settings.ini", QSettings::IniFormat, this);
    this->settings->setValue("run/last_run", QDateTime::currentDateTimeUtc());
    this->load_settings();

    this->server = new Server(QHostAddress(this->ip), this->port);

    this->create_timers();
    this->display_cover_status();

    this->ui->le_ip->setText(this->ip);
    this->ui->spinbox_port->setValue(this->port);
    this->ui->lineedit_station_id->setText(this->station_id);

    this->ui->dsb_latitude->setValue(this->station->latitude);
    this->ui->dsb_longitude->setValue(this->station->longitude);
    this->ui->dsb_altitude->setValue(this->station->altitude);

    connect(this->ui->dsb_latitude, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::station_position_edited);
    connect(this->ui->dsb_longitude, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::station_position_edited);
    connect(this->ui->dsb_altitude, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::station_position_edited);
}

void MainWindow::load_settings(void) {
    this->ip = this->settings->value("server/ip").toString();
    this->port = this->settings->value("server/port").toInt();
    this->station_id = this->settings->value("station/id").toString();

    this->station->latitude = this->settings->value("station/latitude").toDouble();
    this->station->longitude = this->settings->value("station/longitude").toDouble();
    this->station->altitude = this->settings->value("station/altitude").toDouble();
}

void MainWindow::create_timers(void) {
    this->request_telegram();
    this->timer_telegram = new QTimer(this);
    this->timer_telegram->setInterval(3000);
    connect(timer_telegram, &QTimer::timeout, this, &MainWindow::request_telegram);
    this->timer_telegram->start();

    this->timer_heartbeat = new QTimer(this);
    this->timer_heartbeat->setInterval(15 * 1000);
    connect(this->timer_heartbeat, &QTimer::timeout, this, &MainWindow::send_heartbeat);
    this->timer_heartbeat->start();

    this->timer_operation = new QTimer(this);
    this->timer_operation->setInterval(100);
    connect(this->timer_operation, &QTimer::timeout, this, &MainWindow::process_timer);
    this->timer_operation->start();

    this->timer_cover = new QTimer(this);
    this->timer_cover->setInterval(10);
    connect(this->timer_cover, &QTimer::timeout, this, &MainWindow::move_cover);}

MainWindow::~MainWindow() {
    delete ui;
    delete timer_operation;
}

void MainWindow::on_actionExit_triggered() {
    QApplication::quit();
}

void MainWindow::process_timer(void) {
    statusBar()->showMessage(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    ui->label_heartbeat->setText(QString("%1 s").arg((double) this->timer_heartbeat->remainingTime() / 1000, 3, 'f', 1));
    this->display_env_data();
    this->display_sun_properties();

    this->setWindowTitle(this->settings->fileName());
    this->settings->sync();
}

void MainWindow::display_sun_properties(void) {
    ui->lb_hor_altitude->setText(QString("%1°").arg(this->station->get_sun_altitude(), 3, 'f', 3));
    ui->lb_hor_azimuth->setText(QString("%1°").arg(this->station->get_sun_azimuth(), 3, 'f', 3));

    ui->lb_eq_latitude->setText(QString("%1°").arg(Universe::compute_sun_equ()[theta] * Deg, 3, 'f', 3));
    ui->lb_eq_longitude->setText(QString("%1°").arg(Universe::compute_sun_equ()[phi] * Deg, 3, 'f', 3));

    ui->lb_ecl_longitude->setText(QString("%1°").arg(Universe::compute_sun_ecl()[phi] * Deg, 3, 'f', 3));
}

void MainWindow::display_env_data(void) {
    ui->group_environment->setTitle(QString("Environment (%1 s)").arg((double) this->station->dome_manager.get_last_received().msecsTo(QDateTime::currentDateTimeUtc()) / 1000, 3, 'f', 1));
    ui->label_temp->setText(QString("%1 °C").arg(this->station->dome_manager.temperature, 3, 'f', 1));
    ui->label_press->setText(QString("%1 kPa").arg(this->station->dome_manager.pressure / 1000, 5, 'f', 3));
    ui->label_hum->setText(QString("%1 %").arg(this->station->dome_manager.humidity, 3, 'f', 1));
}

void MainWindow::send_heartbeat(void) {
    this->server->send_heartbeat();

}

void MainWindow::heartbeat_error(QNetworkReply::NetworkError error) {
    this->log_error(QString("Heartbeat could not be sent: error %1").arg(error));
}

void MainWindow::heartbeat_ok(QNetworkReply* reply) {
    log_debug("Heartbeat received (HTTP code " + reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString() + "), response: \"" + reply->readAll() + "\"");
    reply->deleteLater();
}

void MainWindow::heartbeat_response(void) {
    this->log_debug("Heartbeat response");
}

QString MainWindow::format_message(const QString& message) const {
    return "[" + QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + "] " + message;
}

void MainWindow::log_debug(const QString& message) {
    qDebug("%s", qUtf8Printable(message));
    ui->log->addItem(this->format_message(message));
}

void MainWindow::log_error(const QString& message) {
    QListWidgetItem *item = new QListWidgetItem(this->format_message(message));
    item->setForeground(Qt::red);
    ui->log->addItem(item);
}

void MainWindow::move_cover(void) {
    switch (this->station->dome_manager.cover_state) {
        case CoverState::OPENING: {
            if (this->station->dome_manager.cover_position < 400) {
                this->station->dome_manager.cover_position += 1;
            } else {
                this->station->dome_manager.cover_state = CoverState::OPEN;
                ui->button_cover->setEnabled(true);
                this->timer_cover->stop();
            }
            break;
        }
        case CoverState::CLOSING: {
            if (this->station->dome_manager.cover_position > 0) {
                this->station->dome_manager.cover_position -= 1;
            } else {
                this->station->dome_manager.cover_state = CoverState::CLOSED;
                ui->button_cover->setEnabled(true);
                this->timer_cover->stop();
            }
            break;
        }
        default:
            break;
    }
    this->display_cover_status();
}

void MainWindow::request_telegram(void) {
    /* Currently a mockup */
    this->station->dome_manager.fake_env_data();
}

void MainWindow::display_cover_status(void) {
    QString station;
    switch (this->station->dome_manager.cover_state) {
        case CoverState::OPEN: {
            station = "Open";
            ui->button_cover->setText("Close");
            break;
        }
        case CoverState::OPENING: {
            station = "Opening...";
            break;
        }
        case CoverState::CLOSED: {
            station = "Closed";
            ui->button_cover->setText("Open");
            break;
        }
        case CoverState::CLOSING: {
            station = "Closing...";
            break;
        }
        default: {
            station = "undefined";
            break;
        }
    }
    ui->progress_cover->setValue(this->station->dome_manager.cover_position);
    ui->label_cover_state->setText(station);
}

void MainWindow::on_button_send_heartbeat_pressed() {
    this->send_heartbeat();
}

void MainWindow::on_button_cover_clicked() {
    if (this->station->dome_manager.cover_state == CoverState::CLOSED) {
        this->station->dome_manager.cover_state = CoverState::OPENING;
    }
    if (this->station->dome_manager.cover_state == CoverState::OPEN) {
        this->station->dome_manager.cover_state = CoverState::CLOSING;
    }
    this->display_cover_status();
    this->timer_cover->start();
    ui->button_cover->setEnabled(false);

}

void MainWindow::on_button_station_accept_clicked() {
    this->station_id = ui->lineedit_station_id->text();
    this->ip = ui->le_ip->text();
    this->port = ui->spinbox_port->value();

    this->station->latitude = ui->dsb_latitude->value();
    this->station->longitude = ui->dsb_longitude->value();
    this->station->altitude = ui->dsb_altitude->value();

    this->settings->setValue("server/ip", this->ip);
    this->settings->setValue("server/port", this->port);
    this->settings->setValue("station/id", this->station_id);
    this->settings->setValue("station/latitude", this->station->latitude);
    this->settings->setValue("station/longitude", this->station->longitude);
    this->settings->setValue("station/altitude", this->station->altitude);

    this->ui->button_station_accept->setText("No changes");
}

void MainWindow::on_checkbox_manual_stateChanged(int enable) {
    if (enable) {
        this->setWindowTitle("AMOS [manual mode]");
        this->log_debug("Switched to manual mode");
    } else {
        this->setWindowTitle("AMOS [automatic mode]");
        this->log_debug("Switched to automatic mode");
    }
    this->station->automatic = (bool) enable;
    ui->group_control->setEnabled(this->station->automatic);
    ui->button_send_heartbeat->setEnabled(this->station->automatic);
}

void MainWindow::station_position_edited(void) {
    this->log_debug(QString("%1 %2 %3").arg(this->station->latitude, 8, 'f', 8).arg(this->station->longitude, 8, 'f', 8).arg(this->station->altitude, 8, 'f', 8));
    if (
        (ui->dsb_latitude->value() != this->station->latitude) ||
        (ui->dsb_longitude->value() != this->station->longitude) ||
        (ui->dsb_altitude->value() != this->station->altitude) ||
        (ui->le_ip->text() != this->server->ip)
    ) {
        ui->button_station_accept->setText("Apply changes");
    } else {
        ui->button_station_accept->setText("No changes");
    }
}
