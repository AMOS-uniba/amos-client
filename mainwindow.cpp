#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <random>

#include <QTimer>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonDocument>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    this->request_telegram();
    this->timer_telegram = new QTimer(this);
    this->timer_telegram->setInterval(3000);
    connect(timer_telegram, SIGNAL(timeout()), this, SLOT(request_telegram()));
    this->timer_telegram->start();

    this->get_housekeeping_data();
    this->timer_operation = new QTimer(this);
    this->timer_operation->setInterval(200);
    connect(timer_operation, SIGNAL(timeout()), this, SLOT(process_timer()));
    this->timer_operation->start();

    this->timer_cover = new QTimer(this);
    this->timer_cover->setInterval(10);
    connect(timer_cover, SIGNAL(timeout()), this, SLOT(move_cover()));

    this->network_manager = new QNetworkAccessManager(this);
    this->last_received = QDateTime::currentDateTimeUtc();
    this->display_cover_status();
}

MainWindow::~MainWindow() {
    delete ui;
    delete timer_operation;
}

void MainWindow::on_actionExit_triggered() {
    QApplication::quit();
}

void MainWindow::on_cbManual_stateChanged(int enable) {
    if (enable) {
        this->setWindowTitle("AMOS [manual mode]");
    } else {
        this->setWindowTitle("AMOS [automatic mode]");
    }
}

void MainWindow::process_timer(void) {
    statusBar()->showMessage(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    this->display_housekeeping_data();
}

void MainWindow::display_housekeeping_data(void) {
    ui->group_environment->setTitle(QString::asprintf("Environment (%.3f s)", (double) this->last_received.msecsTo(QDateTime::currentDateTimeUtc()) / 1000));
    ui->lbTemperature->setText(QString::asprintf("Temperature: %2.1f °C", this->temperature));
    ui->lbPressure->setText(QString::asprintf("Pressure: %5.0f Pa", this->pressure));
    ui->lbHumidity->setText(QString::asprintf("Humidity: %2.1f%%", this->humidity));
}

void MainWindow::get_housekeeping_data(void) {
    std::uniform_real_distribution<double> td(-20, 30);
    std::normal_distribution<double> pd(100000, 1000);
    std::uniform_real_distribution<double> hd(0, 100);

    this->temperature = td(this->generator);
    this->pressure = pd(this->generator);
    this->humidity = hd(this->generator);

    this->last_received = QDateTime::currentDateTimeUtc();
}

void MainWindow::send_heartbeat(void) {
    QUrl url = QUrl("http://192.168.153.128:4805/station/AGO/heartbeat/");
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    connect(this->network_manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(handleEnd(QNetworkReply *)));

    QJsonObject json;

    json["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    json["temperature"] = this->temperature;
    json["pressure"] = this->pressure;
    json["humidity"] = this->humidity;

    QJsonDocument document(json);

    QByteArray message = document.toJson(QJsonDocument::Compact);

    ui->log->addItem(message);
    this->network_manager->post(request, message);
}


void MainWindow::handleEnd(QNetworkReply *a) {
    statusBar()->showMessage("POST request sent");
}

void MainWindow::move_cover(void) {
    switch (this->cover_state) {
        case COVER_OPENING: {
            if (this->cover_position < 400) {
                this->cover_position += 1;
            } else {
                this->cover_state = COVER_OPEN;
                ui->button_cover->setEnabled(true);
                this->timer_cover->stop();
            }
            break;
        }
        case COVER_CLOSING: {
            if (this->cover_position > 0) {
                this->cover_position -= 1;
            } else {
                this->cover_state = COVER_CLOSED;
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
    this->get_housekeeping_data();
}

void MainWindow::display_cover_status(void) {
    QString status;
    switch (this->cover_state) {
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
    ui->progress_cover->setValue(this->cover_position);
    ui->label_cover_state->setText(status);
}

void MainWindow::on_button_send_heartbeat_pressed() {
    this->send_heartbeat();
}

void MainWindow::on_button_cover_clicked() {
    if (this->cover_state == COVER_CLOSED) {
        this->cover_state = COVER_OPENING;
    }
    if (this->cover_state == COVER_OPEN) {
        this->cover_state = COVER_CLOSING;
    }
    this->display_cover_status();
    this->timer_cover->start();
    ui->button_cover->setEnabled(false);
}
