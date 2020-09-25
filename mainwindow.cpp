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
    connect(this->timer_cover, &QTimer::timeout, this, &MainWindow::move_cover);

    this->network_manager = new QNetworkAccessManager(this);
    connect(this->network_manager, &QNetworkAccessManager::finished, this, &MainWindow::heartbeat_ok);

    this->display_cover_status();

    this->ui->lineedit_IP->setText(this->ip);
    this->ui->lineedit_station_id->setText(this->station_id);
}

MainWindow::~MainWindow() {
    delete ui;
    delete timer_operation;
}

void MainWindow::on_actionExit_triggered() {
    QApplication::quit();
}

void MainWindow::on_cbManual_stateChanged(int enable) {
}

void MainWindow::process_timer(void) {
    statusBar()->showMessage(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    ui->label_heartbeat->setText(QString::asprintf("%.0f s", (double) this->timer_heartbeat->remainingTime() / 1000));
    this->display_env_data();
}

void MainWindow::display_sun_properties(void) {
    ui->label_hor_az_value->setText(QString::asprintf("%4.1f°", this->sun_azimuth));
    ui->label_hor_alt_value->setText(QString::asprintf("%4.1f°", this->sun_altitude));
}

void MainWindow::display_env_data(void) {
    ui->group_environment->setTitle(QString::asprintf("Environment (%.3f s)", (double) this->status.dome_manager.last_received.msecsTo(QDateTime::currentDateTimeUtc()) / 1000));
    ui->label_temp->setText(QString::asprintf("%2.1f °C", this->status.dome_manager.temperature));
    ui->label_press->setText(QString::asprintf("%3.2f kPa", this->status.dome_manager.pressure / 1000));
    ui->label_hum->setText(QString::asprintf("%2.1f%%", this->status.dome_manager.humidity));
}

void MainWindow::send_heartbeat(void) {
    QString address = QString("http://%1:%2/station/%3/heartbeat/").arg(this->ip).arg(this->port).arg(this->station_id);
    this->log_debug("About to send a heartbeat to " + address);

    QUrl url(address);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QJsonDocument document = this->status.message();
    QByteArray message = document.toJson(QJsonDocument::Compact);

    this->log_debug(message);
    QNetworkReply *reply = this->network_manager->post(request, message);

    connect(reply, &QNetworkReply::errorOccurred, this, &MainWindow::heartbeat_error);
}

void MainWindow::heartbeat_error(QNetworkReply::NetworkError error) {
    this->log_debug(QString::asprintf("Heartbeat could not be sent: error %d", error));
}

void MainWindow::heartbeat_ok(QNetworkReply* reply) {
    log_debug("Heartbeat received (HTTP code " + reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString() + "), response: \"" + reply->readAll() + "\"");
    reply->deleteLater();
}

void MainWindow::heartbeat_response(void) {
    this->log_debug("Heartbeat response");
}

void MainWindow::log_debug(const QString& message) {
    ui->log->addItem("[" + QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + "] " + message);
}

void MainWindow::move_cover(void) {
    switch (this->status.dome_manager.cover_state) {
        case COVER_OPENING: {
            if (this->status.dome_manager.cover_position < 400) {
                this->status.dome_manager.cover_position += 1;
            } else {
                this->status.dome_manager.cover_state = COVER_OPEN;
                ui->button_cover->setEnabled(true);
                this->timer_cover->stop();
            }
            break;
        }
        case COVER_CLOSING: {
            if (this->status.dome_manager.cover_position > 0) {
                this->status.dome_manager.cover_position -= 1;
            } else {
                this->status.dome_manager.cover_state = COVER_CLOSED;
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
    this->status.dome_manager.fake_env_data();
}

void MainWindow::display_cover_status(void) {
    QString status;
    switch (this->status.dome_manager.cover_state) {
        case COVER_OPEN: {
            status = "Open";
            ui->button_cover->setText("Close");
            break;
        }
        case COVER_OPENING: {
            status = "Opening...";
            break;
        }
        case COVER_CLOSED: {
            status = "Closed";
            ui->button_cover->setText("Open");
            break;
        }
        case COVER_CLOSING: {
            status = "Closing...";
            break;
        }
        default: {
            status = "undefined";
            break;
        }
    }
    ui->progress_cover->setValue(this->status.dome_manager.cover_position);
    ui->label_cover_state->setText(status);
}

void MainWindow::on_button_send_heartbeat_pressed() {
    this->send_heartbeat();
}

void MainWindow::on_button_cover_clicked() {
    if (this->status.dome_manager.cover_state == COVER_CLOSED) {
        this->status.dome_manager.cover_state = COVER_OPENING;
    }
    if (this->status.dome_manager.cover_state == COVER_OPEN) {
        this->status.dome_manager.cover_state = COVER_CLOSING;
    }
    this->display_cover_status();
    this->timer_cover->start();
    ui->button_cover->setEnabled(false);
}

void MainWindow::on_button_station_accept_clicked() {
    this->ip = ui->lineedit_IP->text();
    this->station_id = ui->lineedit_station_id->text();
}

void MainWindow::on_checkbox_manual_stateChanged(int enable) {
    if (enable) {
        this->setWindowTitle("AMOS [manual mode]");
    } else {
        this->setWindowTitle("AMOS [automatic mode]");
    }
}
